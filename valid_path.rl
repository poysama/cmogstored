/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

/*
 * we could just use strstr(), but it's buggy on some glibc and
 * we can expand this later (to tighten down to non-FIDs, for example)
 */
%%{
	machine path_traversal;
	main := any* ("..") @ { found = true; fbreak; } any*;
}%%

%% write data;

static bool path_traversal_found(const char *buf, size_t len)
{
	const char *p, *pe;
	bool found = false;
	int cs;
	%% write init;

	p = buf;
	pe = buf + len;
	%% write exec;

	return found;
}

int mog_valid_path(const char *buf, size_t len)
{
	/* TODO: update if MogileFS supports FIDs >= 10,000,000,000 */
	if (len >= (sizeof("/dev16777215/0/000/000/0123456789.fid")))
		return 0;

	return ! path_traversal_found(buf, len);
}
