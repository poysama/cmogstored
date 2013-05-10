/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

/*
 * this function is only called in a forked child (for iostat)
 * if O_CLOEXEC/SOCK_CLOEXEC is unsupported, or if mog_cloexec_detect()
 * detects those flags are broken.
 */
void mog_cloexec_from(int lowfd)
{
	int fd;
	int last_good = lowfd;

	for (fd = lowfd; fd < INT_MAX; fd++) {
		if (mog_set_cloexec(fd, true) == 0)
			last_good = fd;
		if ((last_good + 1024) < fd)
			break;
	}
}
