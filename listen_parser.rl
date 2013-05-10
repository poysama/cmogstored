/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "listen_parser.h"
%%{
	machine listen_parser;
	include listen_parser_common "listen_parser_common.rl";

	main := listen '\0'> {
		a = mog_listen_parse_internal(mark_beg, mark_len,
		                              port_beg, port_len);
	};
}%%

%% write data;

static struct mog_addrinfo *listen_parse(char *str)
{
	char *p, *pe, *eof = NULL;
	char *mark_beg = NULL;
	char *port_beg = NULL;
	size_t mark_len = 0;
	size_t port_len = 0;
	struct mog_addrinfo *a = NULL;
	int cs;

	%% write init;

	p = str;
	pe = str + strlen(str) + 1;

	%% write exec;

	if ((cs == listen_parser_error) && a)
		mog_addrinfo_free(&a);

	assert(p <= pe && "buffer overflow after listen parse");
	return a;
}

struct mog_addrinfo *mog_listen_parse(const char *str)
{
	char *tmp = xstrdup(str);
	struct mog_addrinfo *rv = listen_parse(tmp);
	free(tmp);

	return rv;
}
