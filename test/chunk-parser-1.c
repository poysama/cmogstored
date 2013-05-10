/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "check.h"

static struct mog_http xhttp;
static struct mog_http *http = &xhttp;
static char *buf;
static size_t len;
static enum mog_parser_state state;
static FILE *tmpfp;
static int tmpfd;

static void reset(void)
{
	free(buf);
	assert(0 == fclose(tmpfp));
	mog_chunk_init(http);
}

static void buf_set(const char *s)
{
	struct mog_file *file;

	http->chunked = 1;
	reset();
	tmpfp = tmpfile();
	assert(tmpfp != NULL && "tmpfile(3) failed");
	tmpfd = fileno(tmpfp);
	assert(tmpfd >= 0 && "invalid fd");
	http->forward = mog_fd_get(tmpfd);
	http->forward->fd = tmpfd;
	file = &http->forward->as.file;
	file->foff = 0;
	buf = xstrdup(s);
	len = strlen(s);
}

int main(void)
{
	tmpfp = tmpfile();
	assert(tmpfp != NULL && "tmpfile(3) failed");

	if ("normal chunk") {
		buf_set("666\r\n");
		state = mog_chunk_parse(http, buf, len);
		assert(state == MOG_PARSER_DONE);
		assert(http->content_len == 0x666);
		assert(http->chunk_state == MOG_CHUNK_STATE_DATA);
	}

	if ("incomplete chunk") {
		buf_set("666\r");
		state = mog_chunk_parse(http, buf, len);
		assert(state == MOG_PARSER_CONTINUE);
		assert(http->content_len == 0x666);
		assert(http->chunk_state == MOG_CHUNK_STATE_SIZE);
	}

	if ("bad chunk") {
		buf_set("zzz\r\n");
		state = mog_chunk_parse(http, buf, len);
		assert(state == MOG_PARSER_ERROR);
	}

	if ("normal chunk with extension") {
		buf_set("abcde; foo=bar\r\n");
		state = mog_chunk_parse(http, buf, len);
		assert(state == MOG_PARSER_DONE);
		assert(http->content_len == 0xabcde);
		assert(http->chunk_state == MOG_CHUNK_STATE_DATA);
	}

	if ("chunk with complete header and data") {
		char tmp[5];
		buf_set("5\r\nabcde");
		state = mog_chunk_parse(http, buf, len);
		assert(state == MOG_PARSER_CONTINUE);
		assert(http->content_len == 0);
		assert(http->chunk_state == MOG_CHUNK_STATE_SIZE);
		assert(sizeof(tmp) == pread(tmpfd, tmp, sizeof(tmp), 0));
		assert(0 == memcmp(tmp, "abcde", sizeof(tmp)));
	}

	if ("chunk with complete header and data and incomplete chunk") {
		char tmp[5];
		buf_set("5\r\nabcde\r\n3");
		state = mog_chunk_parse(http, buf, len);
		assert(state == MOG_PARSER_CONTINUE);
		assert(http->content_len == 3);
		assert(http->chunk_state == MOG_CHUNK_STATE_SIZE);
		assert(sizeof(tmp) == pread(tmpfd, tmp, sizeof(tmp), 0));
		assert(0 == memcmp(tmp, "abcde", sizeof(tmp)));
		assert(http->offset == len);
	}
	if ("multiple chunks with end") {
		char tmp[8];
		buf_set("5\r\nabcde\r\n3\r\n123\r\n0\r\n\r\n");
		state = mog_chunk_parse(http, buf, len);
		assert(state == MOG_PARSER_DONE);
		assert(http->chunk_state == MOG_CHUNK_STATE_DONE);
		assert(sizeof(tmp) == pread(tmpfd, tmp, sizeof(tmp), 0));
		assert(0 == memcmp(tmp, "abcde123", sizeof(tmp)));
		assert(http->offset == len);
	}

	if ("multiple chunks with trailer") {
		char tmp[8];
		buf_set("5\r\nabcde\r\n3\r\n123\r\n0\r\nFoo: bar\r\n\r\n");
		state = mog_chunk_parse(http, buf, len);
		assert(http->chunk_state == MOG_CHUNK_STATE_DONE);
		assert(state == MOG_PARSER_DONE);
		assert(http->content_len == 0);
		assert(sizeof(tmp) == pread(tmpfd, tmp, sizeof(tmp), 0));
		assert(0 == memcmp(tmp, "abcde123", sizeof(tmp)));
		assert(http->offset == len);
	}

	if ("multiple chunks with almost end") {
		char tmp[8];
		buf_set("5\r\nabcde\r\n3\r\n123\r\n0\r\n");
		state = mog_chunk_parse(http, buf, len);
		assert(state == MOG_PARSER_DONE);
		assert(http->chunk_state == MOG_CHUNK_STATE_DATA);
		assert(http->content_len == 0);
		assert(sizeof(tmp) == pread(tmpfd, tmp, sizeof(tmp), 0));
		assert(0 == memcmp(tmp, "abcde123", sizeof(tmp)));
		assert(http->offset == len);
	}

	if ("multiple chunks with almost end (more)") {
		char tmp[8];
		buf_set("5\r\nabcde\r\n3\r\n123\r\n0\r\n\r");
		state = mog_chunk_parse(http, buf, len);
		assert(state == MOG_PARSER_CONTINUE);
		assert(http->chunk_state == MOG_CHUNK_STATE_TRAILER);
		assert(http->content_len == 0);
		assert(sizeof(tmp) == pread(tmpfd, tmp, sizeof(tmp), 0));
		assert(0 == memcmp(tmp, "abcde123", sizeof(tmp)));
		assert(http->offset == len);
	}

	if ("multiple chunks with incomplete") {
		char tmp[7];
		buf_set("5\r\nabcde\r\n3\r\n12");
		state = mog_chunk_parse(http, buf, len);
		assert(state == MOG_PARSER_DONE);
		assert(http->chunk_state == MOG_CHUNK_STATE_DATA);
		assert(http->content_len == 1);
		assert(sizeof(tmp) == pread(tmpfd, tmp, sizeof(tmp), 0));
		assert(0 == memcmp(tmp, "abcde12", sizeof(tmp)));
		assert(http->offset == len);
	}

	if ("incomplete data") {
		char tmp[3];
		buf_set("666\r\nabc");
		state = mog_chunk_parse(http, buf, len);
		assert(state == MOG_PARSER_DONE);
		assert(http->chunk_state == MOG_CHUNK_STATE_DATA);
		assert(http->content_len == (0x666 - sizeof(tmp)));
		assert(sizeof(tmp) == pread(tmpfd, tmp, sizeof(tmp), 0));
		assert(0 == memcmp(tmp, "abc", sizeof(tmp)));
		assert(http->offset == len);
	}

	reset();
	return 0;
}
