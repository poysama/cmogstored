/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "http_util.h"

static bool length_incr(off_t *len, unsigned c)
{
	off_t prev = *len;

	*len *= 10;
	*len += c - '0';

	if (*len >= prev)
		return true;

	errno = ERANGE;
	*len = -1;

	return false;
}

%%{
	machine http_parser;
	include http_common "http_common.rl";

	ignored_header := header_name ':' sep header_value eor @ {
		fgoto more_headers;
	};

	mog_path = '/'[a-zA-Z0-9/\.\-]{0,36}; # only stuff MogileFS will use
	GET = "GET "> { http->http_method = MOG_HTTP_METHOD_GET; };
	HEAD = "HEAD "> { http->http_method = MOG_HTTP_METHOD_HEAD; };
	PUT = "PUT "> { http->http_method = MOG_HTTP_METHOD_PUT; };
	DELETE = "DELETE "> { http->http_method = MOG_HTTP_METHOD_DELETE; };
	MKCOL = "MKCOL "> { http->http_method = MOG_HTTP_METHOD_MKCOL; };

	# no HTTP/0.9 for now, sorry (not :P)
	req_line = (HEAD|GET|PUT|DELETE|MKCOL)
		("http://" [^/]+)?
		'/'*(mog_path) > { http->path_tip = to_u8(fpc - buf); }
		# TODO: maybe folks use query string/fragments for logging...
		(" HTTP/1.") > { http->path_end = to_u8(fpc - buf); }
		('0'|'1'> { http->persistent = 1; }) '\r'LF;

	content_length = "Content-Length:"i sep
		(digit+) $ {
			if (!length_incr(&http->content_len, fc))
				fbreak;
		}
		$! { errno = EINVAL; fbreak; }
		eor;
	content_range = "Content-Range:"i sep "bytes"LWS+
		(digit+) $ {
			if (!length_incr(&http->range_beg, fc))
				fbreak;
		}
		$! { errno = EINVAL; fbreak; }
		"-"
		(digit+) $ {
			if (!length_incr(&http->range_end, fc))
				fbreak;
		}
		$! { errno = EINVAL; fbreak; }
		"/*"
		eor > { http->has_content_range = 1; };
	range = "Range:"i sep (
			"bytes=" > {
				http->range_beg = http->range_end = -1;
			}
			(digit*) $ {
				if (http->range_beg < 0)
					http->range_beg = 0;
				if (!length_incr(&http->range_beg, fc))
					fbreak;
			}
			'-'
			(digit*) $ {
				if (http->range_end < 0)
					http->range_end = 0;
				if (!length_incr(&http->range_end, fc))
					fbreak;
			}
                ) $! { errno = EINVAL; fbreak; }
		eor @ { http->has_range = 1; };
	transfer_encoding_chunked = "Transfer-Encoding:"i sep
		"chunked"i eor > { http->chunked = 1; };
	trailer = "Trailer:"i sep
		(("Content-MD5"i @ { http->has_trailer_md5 = 1; })
		 | header_name | ',')
		eor;
	connection = "Connection:"i sep
		(("close"i @ { http->persistent = 0; }) |
		 ("keep-alive"i @ { http->persistent = 1; })) eor;
	header_line =
		( content_length |
		  transfer_encoding_chunked |
		  trailer |
		  range |
		  content_range |
		  content_md5 |
		  connection ) $!
		{
			assert(http->line_end > 0 &&
			       "no previous request/header line");
			assert(buf[http->line_end] == '\n' &&
			       "bad http->line_end");
			p = buf + http->line_end + 1;
			assert(p <= pe && "overflow");
			fgoto ignored_header;
		};
	headers = header_line* '\r''\n' > { really_done = 1; fbreak; };
	more_headers := headers;
	main := req_line headers;
}%%

%% write data;

void mog_http_reset_parser(struct mog_http *http)
{
	int cs;
	struct mog_rbuf *rbuf = http->rbuf;
	struct mog_svc *svc = http->svc;

	%% write init;
	memset(http, 0, sizeof(struct mog_http));
	http->cs = cs;
	http->rbuf = rbuf;
	http->svc = svc;
}

void mog_http_init(struct mog_http *http, struct mog_svc *svc)
{
	http->svc = svc;
	http->rbuf = NULL;
	mog_http_reset_parser(http);
}

enum mog_parser_state
mog_http_parse(struct mog_http *http, char *buf, size_t len)
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
		cs = http_parser_first_final;

	http->cs = cs;
	http->offset = p - buf;

	if (cs == http_parser_error || errno)
		return MOG_PARSER_ERROR;

	assert(p <= pe && "buffer overflow after http parse");
	assert(http->offset <= len && "offset longer than len");

	if (http->cs == http_parser_first_final) return MOG_PARSER_DONE;
	return MOG_PARSER_CONTINUE;
}
