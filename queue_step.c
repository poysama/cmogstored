/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

enum mog_next mog_queue_step(struct mog_fd *mfd)
{
	switch (mfd->fd_type) {
	case MOG_FD_TYPE_MGMT:
		return mog_mgmt_queue_step(mfd);
	case MOG_FD_TYPE_HTTP:
	case MOG_FD_TYPE_HTTPGET:
		return mog_http_queue_step(mfd);
	default:
		assert(0 && "BUG: bad fd_type in thread pool queue iterator");
	}
	return MOG_NEXT_IGNORE; /* should never get here */
}
