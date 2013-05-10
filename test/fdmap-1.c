/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "check.h"

int main(void)
{
	struct mog_fd *mfd;
	int open_max = (int)sysconf(_SC_OPEN_MAX);
	int i;

	mfd = mog_fd_get(0);
	{
		struct mog_mgmt *mgmt = &mfd->as.mgmt;

		assert(mog_fd_of(mgmt) == mfd);
	}

	for (i = 0; i < open_max; i++) {
		mfd = mog_fd_get(i);
		assert(mfd && "mfd unset");
	}

	return 0;
}
