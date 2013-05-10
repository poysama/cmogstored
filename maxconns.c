/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include <sys/time.h>
#include <sys/resource.h>

#ifndef RLIM_INFINITY
#  define RLIM_INFINITY ((rlim_t)(-1))
#endif

void mog_set_maxconns(unsigned long maxconns)
{
	struct rlimit r;
	rlim_t want;
	struct rlimit orig;

	if (getrlimit(RLIMIT_NOFILE, &r) != 0)
		die_errno("getrlimit(RLIMIT_NOFILE) failed");

	memcpy(&orig, &r, sizeof(struct rlimit));

	if (maxconns == 0)
		maxconns = MOG_DEFAULT_MAXCONNS;
	want = maxconns;

	if ((int)want < 0 || want > MOG_FD_MAX)
		want = MOG_FD_MAX; /* LOL :D */
	if (r.rlim_cur >= want)
		return;

	if (r.rlim_max == RLIM_INFINITY || r.rlim_cur == RLIM_INFINITY) {
		/* insane? maybe... */
		r.rlim_max = r.rlim_cur = want;
	} else if (r.rlim_max == 0) {
		warn("RLIMIT_NOFILE max=0, trying %ld anyways", (long)want);
		r.rlim_max = r.rlim_cur = want;
	} else if (r.rlim_max < want) {
		warn("RLIMIT_NOFILE max=%ld less than wanted value=%ld",
		     (long)r.rlim_max, (long)want);
		r.rlim_cur = r.rlim_max;
	} else {
		r.rlim_max = r.rlim_cur = want;
	}

	if (setrlimit(RLIMIT_NOFILE, &r) == 0) return;

	warn("failed to set RLIMIT_NOFILE max=%ld cur=%ld (maxconns=%lu)",
	     (long)r.rlim_max, (long)r.rlim_cur, maxconns);

	while ((want -= 64) >= maxconns) {
		r.rlim_max = r.rlim_cur = want;
		if (setrlimit(RLIMIT_NOFILE, &r) == 0)
			goto eventual_success;
	}

	warn("RLIMIT_NOFILE stuck at  max=%ld cur=%ld (maxconns=%lu)",
	     (long)orig.rlim_max, (long)orig.rlim_cur, maxconns);
	return;

eventual_success:
	warn("set RLIMIT_NOFILE max=%ld cur=%ld (maxconns=%lu)",
	     (long)r.rlim_max, (long)r.rlim_cur, maxconns);
}
