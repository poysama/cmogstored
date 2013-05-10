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

static void assert_path_equal(const char *str)
{
	size_t slen = strlen(str);

	assert(0 == memcmp(str, buf + http->path_tip, slen));
	assert(http->path_end == http->path_tip + slen);
}

static void reset(void)
{
	free(buf);
	mog_http_init(http, NULL);
}

static void buf_set(const char *s)
{
	reset();
	buf = xstrdup(s);
	len = strlen(s);
}

int main(void)
{
	if ("normal HTTP GET request") {
		buf_set("GET /foo HTTP/1.1\r\nHost: 127.6.6.6\r\n\r\n");
		state = mog_http_parse(http, buf, len);
		assert(http->http_method == MOG_HTTP_METHOD_GET
		       && "http_method should be GET");
		assert(http->persistent && "not persistent");
		assert(state == MOG_PARSER_DONE && "parser not done");
		assert_path_equal("/foo");
	}

	if ("normal HTTP GET request with redundant leading slash") {
		buf_set("GET //foo HTTP/1.1\r\nHost: 127.6.6.6\r\n\r\n");
		state = mog_http_parse(http, buf, len);
		assert(http->http_method == MOG_HTTP_METHOD_GET
		       && "http_method should be GET");
		assert(http->persistent && "not persistent");
		assert(state == MOG_PARSER_DONE && "parser not done");
		assert_path_equal("/foo");
	}

	if ("HTTP/1.1 request with explicit close") {
		buf_set("GET /foo HTTP/1.1\r\n"
		        "Host: 127.6.6.6\r\n"
		        "Connection: close\r\n"
		        "\r\n");
		state = mog_http_parse(http, buf, len);
		assert(http->http_method == MOG_HTTP_METHOD_GET
		       && "http_method should be GET");
		assert(http->persistent == 0 && "should not be persistent");
		assert(state == MOG_PARSER_DONE && "parser not done");
		assert_path_equal("/foo");
	}

	if ("HTTP/1.0 request with keepalive") {
		buf_set("GET /foo HTTP/1.0\r\n"
		        "Connection:\r\n keep-alive\r\n"
		        "\r\n");
		state = mog_http_parse(http, buf, len);
		assert(http->http_method == MOG_HTTP_METHOD_GET
		       && "http_method should be GET");
		assert(http->persistent == 1 && "should be persistent");
		assert(state == MOG_PARSER_DONE && "parser not done");
		assert_path_equal("/foo");
	}

	if ("bogus HTTP/1.0 request") {
		buf_set("GET /foo HTTP/1.0\r\n"
		        "Connection:\r\nkeep-alive\r\n"
		        "\r\n");
		state = mog_http_parse(http, buf, len);
		assert(state == MOG_PARSER_ERROR && "parser not errored");
	}

	if ("bogus request") {
		buf_set("adlkf;asdfj\r\n'GET /foo HTTP/1.0\r\n"
		        "Connection:\r\nkeep-alive\r\n"
		        "\r\n");
		state = mog_http_parse(http, buf, len);
		assert(state == MOG_PARSER_ERROR && "parser not errored");
	}

	if ("HTTP/1.1 HEAD request") {
		buf_set("HEAD /foo HTTP/1.1\r\n"
		        "Host: 127.6.6.6\r\n"
		        "\r\n");
		state = mog_http_parse(http, buf, len);
		assert(http->http_method == MOG_HTTP_METHOD_HEAD
		       && "http_method should be HEAD ");
		assert(http->persistent == 1 && "should be persistent");
		assert(state == MOG_PARSER_DONE && "parser not done");
		assert_path_equal("/foo");
	}

	if ("HTTP/1.1 PUT request") {
		buf_set("PUT /foo HTTP/1.1\r\n"
		        "Host: 127.6.6.6\r\n"
		        "Content-Length: 12345\r\n"
		        "\r\n"
		        "partial body request");
		state = mog_http_parse(http, buf, len);
		assert(http->content_len == 12345);
		assert(http->http_method == MOG_HTTP_METHOD_PUT
		       && "http_method should be PUT");
		assert(http->persistent == 1 && "should be persistent");
		assert(state == MOG_PARSER_DONE && "parser not done");
		assert_path_equal("/foo");
		assert(strcmp(buf + http->offset, "partial body request") == 0
		       && "buffer repositioned to body start");
	}

	if ("HTTP/1.1 PUT chunked request header") {
		buf_set("PUT /foo HTTP/1.1\r\n"
		        "Host: 127.6.6.6\r\n"
		        "Transfer-Encoding: chunked\r\n"
		        "\r\n"
		        "16\r\npartial...");
		state = mog_http_parse(http, buf, len);
		assert(http->chunked);
		assert(http->has_trailer_md5 == 0);
		assert(http->http_method == MOG_HTTP_METHOD_PUT
		       && "http_method should be PUT");
		assert(http->persistent == 1 && "should be persistent");
		assert(state == MOG_PARSER_DONE && "parser not done");
		assert_path_equal("/foo");
		assert(strcmp(buf + http->offset, "16\r\npartial...") == 0
		       && "buffer repositioned to body start");
	}

	if ("HTTP/1.1 PUT with Content-Range") {
		buf_set("PUT /foo HTTP/1.1\r\n"
		        "Host: 127.6.6.6\r\n"
		        "Transfer-Encoding: chunked\r\n"
		        "Content-Range: bytes 666-666666/*\r\n"
		        "\r\n"
		        "16\r\npartial...");
		state = mog_http_parse(http, buf, len);
		assert(http->range_beg == 666);
		assert(http->range_end == 666666);
		assert(http->has_content_range == 1);
		assert(http->has_trailer_md5 == 0);
		assert(http->http_method == MOG_HTTP_METHOD_PUT
		       && "http_method should be PUT");
		assert(http->persistent == 1 && "should be persistent");
		assert(state == MOG_PARSER_DONE && "parser not done");
		assert_path_equal("/foo");
		assert(strcmp(buf + http->offset, "16\r\npartial...") == 0
		       && "buffer repositioned to body start");
	}

	if ("HTTP/1.1 PUT chunked request header w/Trailer") {
		buf_set("PUT /foo HTTP/1.1\r\n"
		        "Host: 127.6.6.6\r\n"
		        "Transfer-Encoding: chunked\r\n"
			"Trailer: Content-MD5\r\n"
		        "\r\n"
		        "16\r\npartial...");
		state = mog_http_parse(http, buf, len);
		assert(http->chunked);
		assert(http->has_trailer_md5 == 1);
		assert(http->http_method == MOG_HTTP_METHOD_PUT
		       && "http_method should be PUT");
		assert(http->persistent == 1 && "should be persistent");
		assert(state == MOG_PARSER_DONE && "parser not done");
		assert_path_equal("/foo");
		assert(strcmp(buf + http->offset, "16\r\npartial...") == 0
		       && "buffer repositioned to body start");
	}

	if ("HTTP/1.1 DELETE request") {
		buf_set("DELETE /foo HTTP/1.1\r\n"
		        "Host: 127.6.6.6\r\n"
		        "\r\n");
		state = mog_http_parse(http, buf, len);
		assert(http->content_len == 0);
		assert(http->has_trailer_md5 == 0);
		assert(http->http_method == MOG_HTTP_METHOD_DELETE
		       && "http_method should be DELETE");
		assert(http->persistent == 1 && "should be persistent");
		assert(state == MOG_PARSER_DONE && "parser not done");
		assert_path_equal("/foo");
	}

	if ("HTTP/1.1 MKCOL request") {
		buf_set("MKCOL /foo HTTP/1.1\r\n"
		        "Host: 127.6.6.6\r\n"
		        "\r\n");
		state = mog_http_parse(http, buf, len);
		assert(http->content_len == 0);
		assert(http->has_trailer_md5 == 0);
		assert(http->http_method == MOG_HTTP_METHOD_MKCOL
		       && "http_method should be MKCOL");
		assert(http->persistent == 1 && "should be persistent");
		assert(state == MOG_PARSER_DONE && "parser not done");
		assert_path_equal("/foo");
	}

	if ("HTTP/1.1 Range (mid) GET request") {
		buf_set("GET /foo HTTP/1.1\r\n"
		        "Host: 127.6.6.6\r\n"
		        "Range: bytes=5-55\r\n"
		        "\r\n");
		state = mog_http_parse(http, buf, len);
		assert(http->has_range == 1);
		assert(http->range_beg == 5 && "range_beg didn't match");
		assert(http->range_end == 55 && "range_end didn't match");
		assert(http->http_method == MOG_HTTP_METHOD_GET
		       && "http_method should be GET");
		assert(http->persistent == 1 && "should be persistent");
		assert(state == MOG_PARSER_DONE && "parser not done");
		assert_path_equal("/foo");
	}

	if ("HTTP/1.1 Range (tip) GET request") {
		buf_set("GET /foo HTTP/1.1\r\n"
		        "Host: 127.6.6.6\r\n"
		        "Range: bytes=-55\r\n"
		        "\r\n");
		state = mog_http_parse(http, buf, len);
		assert(http->has_range == 1);
		assert(http->range_beg == -1 && "range_beg didn't match");
		assert(http->range_end == 55 && "range_end didn't match");
		assert(http->http_method == MOG_HTTP_METHOD_GET
		       && "http_method should be GET");
		assert(http->persistent == 1 && "should be persistent");
		assert(state == MOG_PARSER_DONE && "parser not done");
		assert_path_equal("/foo");
	}

	if ("HTTP/1.1 Range (end) GET request") {
		buf_set("GET /foo HTTP/1.1\r\n"
		        "Host: 127.6.6.6\r\n"
		        "Range: bytes=55-\r\n"
		        "\r\n");
		state = mog_http_parse(http, buf, len);
		assert(http->has_range == 1);
		assert(http->range_beg == 55 && "range_beg didn't match");
		assert(http->range_end == -1 && "range_end didn't match");
		assert(http->http_method == MOG_HTTP_METHOD_GET
		       && "http_method should be GET");
		assert(http->persistent == 1 && "should be persistent");
		assert(state == MOG_PARSER_DONE && "parser not done");
		assert_path_equal("/foo");
	}

	reset();
	return 0;
}
