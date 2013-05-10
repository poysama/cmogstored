/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

%%{
	machine valid_put_path;
	main := "/dev"digit+ ('/'+) [^/] any+;
}%%

%% write data;

bool mog_valid_put_path(const char *buf, size_t len)
{
	const char *p, *pe;
	int cs;

	if (len <= 0)
		return false;
	if (buf[len - 1] == '/')
		return false;

	%% write init;

	p = buf;
	pe = buf + len;
	%% write exec;

	return cs != valid_put_path_error;
}
