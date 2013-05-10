/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

struct mog_accept *
mog_accept_init(int fd, struct mog_svc *svc, post_accept_fn fn)
{
	struct mog_fd *mfd = mog_fd_get(fd);
	struct mog_accept *ac = &mfd->as.accept;

	mfd->fd = fd;
	ac->post_accept_fn = fn;
	ac->svc = svc;
	memset(&ac->thrpool, 0, sizeof(struct mog_thrpool));

	return ac;
}
