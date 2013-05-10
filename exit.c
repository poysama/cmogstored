/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "nostd/setproctitle.h"

static void acceptor_quit(int *fdp)
{
	int fd = *fdp;

	if (fd >= 0) {
		struct mog_fd *mfd = mog_fd_get(fd);
		struct mog_accept *ac = &mfd->as.accept;

		mog_thrpool_quit(&ac->thrpool, NULL);
		mog_fd_put(mfd);
		*fdp = -1;
	}
}

static bool svc_quit_accept_i(void *svcptr, void *ignored)
{
	struct mog_svc *svc = svcptr;

	acceptor_quit(&svc->mgmt_fd);
	acceptor_quit(&svc->http_fd);
	acceptor_quit(&svc->httpget_fd);

	return true;
}

static bool svc_queue_set(void *svcptr, void *queue)
{
	struct mog_svc *svc = svcptr;

	svc->queue = queue;

	return true;
}

_Noreturn void cmogstored_exit(void)
{
	/* call atexit() handlers and make valgrind happy */
	setproctitle("cmogstored, shutting down");
	mog_svc_each(svc_quit_accept_i, NULL);
	mog_svc_dev_shutdown();
	mog_queue_stop(mog_notify_queue);
	mog_svc_dev_shutdown();
	mog_svc_each(svc_queue_set, mog_notify_queue);
	mog_fdmap_requeue(mog_notify_queue);
	mog_queue_quit_loop(mog_notify_queue);
	exit(EXIT_SUCCESS);
}
