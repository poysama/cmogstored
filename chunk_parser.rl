/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "http_util.h"
static inline off_t hexchar2off(int xdigit)
{
	if (xdigit >= '0' && xdigit <= '9')
		return xdigit - '0';
	if (xdigit >= 'a' && xdigit <= 'f')
		return xdigit - 'a' + 10;
	if (xdigit >= 'A' && xdigit <= 'F')
		return xdigit - 'A' + 10;

	/* Ragel already does runtime range checking for us  */
	assert(0 && "invalid digit character");
	return (off_t)LLONG_MIN;
}

%%{
	machine chunk_parser;
	include http_common "http_common.rl";

	chunk_data = (any*) > {
		off_t buf_remain;
		size_t wr_len;

		if (http->content_len == 0) { /* final chunk */
			http->chunk_state = MOG_CHUNK_STATE_TRAILER;
			fhold;

			/* XXX this feels wrong ... */
			if (fpc >= buf) {
				assert(fc == '\n' && "bad chunk end");
				http->line_end = to_u16(fpc - buf);
			}
			fgoto more_trailers;
		}

		assert(http->content_len > 0 && "impossible content_len");

		buf_remain = len - (fpc - buf);
		if (buf_remain == 0)
			fbreak;

		assert(buf_remain > 0 && "impossible buf_remain");
		wr_len = MIN((size_t)http->content_len, (size_t)buf_remain);
		assert(wr_len != 0 && "invalid wr_len");
		if (! mog_http_write_full(http->forward, fpc, wr_len))
			fbreak;

		http->content_len -= wr_len;
		p += wr_len - 1;
		assert(p < pe && "buffer overrun");

		if (http->content_len > 0) {
			really_done = 1;
			/* let caller handle reading the rest of the body */
			fbreak;
		}

		/* next chunk header */
		http->chunk_state = MOG_CHUNK_STATE_SIZE;
		if (wr_len == buf_remain) {
			if (http->content_len == 0)
				fgoto main;
			really_done = 1;
			fbreak;
		}

		/* more chunks in this buffer */
		assert(http->content_len == 0 &&
		       "bad content_len at chunk end");

		fgoto main;
	};
	chunk = "\r\n"? # account for trailing CRLF in previous chunk
		(xdigit+) $ {
			off_t prev = http->content_len;

			http->content_len *= 16;
			http->content_len += hexchar2off(fc);
			if (http->content_len < prev) {
				errno = ERANGE;
				http->content_len = -1;
				fbreak;
			}
		}
		(any -- [\r\n])*
		'\r' '\n' @ { http->chunk_state = MOG_CHUNK_STATE_DATA; }
		chunk_data;
	main := chunk+;
}%%

%% write data;

void mog_chunk_init(struct mog_http *http)
{
	int cs;

	%% write init;
	assert(http->chunked && "not chunked");
	http->cs = cs;
	http->line_end = 0;
	http->content_len = 0;
	http->offset = 0;
	http->chunk_state = MOG_CHUNK_STATE_SIZE;
}

enum mog_parser_state
mog_chunk_parse(struct mog_http *http, char *buf, size_t len)
{
	char *p, *pe, *eof = NULL;
	int cs = http->cs;
	int really_done = 0;
	size_t off = http->offset;

	assert(http->wbuf == NULL && "unwritten data in buffer");
	assert(off <= len && "http offset past end of buffer");

	p = buf + off;
	pe = buf + len;

	assert((void *)(pe - p) == (void *)(len - off) &&
	       "pointers aren't same distance");

	errno = 0;
	%% write exec;

	if (really_done)
		cs = chunk_parser_first_final;

	http->cs = cs;
	http->offset = p - buf;

	if (cs == chunk_parser_error || errno)
		return MOG_PARSER_ERROR;

	assert(p <= pe && "buffer overflow after chunk parse");
	assert(http->offset <= len && "offset longer than len");

	if (http->cs == chunk_parser_first_final) return MOG_PARSER_DONE;
	return MOG_PARSER_CONTINUE;
}
