/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

char *mog_canonpath(const char *path, enum canonicalize_mode_t canon_mode)
{
	char *p = canonicalize_filename_mode(path, canon_mode);

	if (!p && errno == ENOMEM)
		mog_oom();

	return p; /* may be null if errors */
}

char *mog_canonpath_die(const char *path, enum canonicalize_mode_t canon_mode)
{
	char *p = mog_canonpath(path, canon_mode);

	if (p) return p;

	if (errno)
		die_errno("`%s' failed to resolve", path);
	else
		die("`%s' failed to resolve", path);
}
