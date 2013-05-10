/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

static void queue_loop_cleanup(void *arg)
{
	unsigned long self = (unsigned long)pthread_self();

	syslog(LOG_DEBUG, "mog_queue_loop[%lx] thread shutting down...", self);
	mog_alloc_quit();
	syslog(LOG_DEBUG, "mog_queue_loop[%lx] thread done", self);
}

static struct mog_fd *queue_xchg_maybe(struct mog_queue *q, struct mog_fd *mfd)
{
	/*
	 * idle, just-ready clients are the most important
	 * We use a zero timeout here since epoll_wait() is
	 * optimizes for the non-blocking case.
	 */
	struct mog_fd *recent_mfd = mog_idleq_wait(q, 0);

	if (recent_mfd) {
		/*
		 * We got a more important client, push
		 * active_mfd into the active queue for another
		 * thread to service while we service a more
		 * recently-active client.
		 */
		mog_activeq_push(q, mfd);
		return recent_mfd;
	}

	/*
	 * keep processing the currently-active mfd in this thread
	 * if no new work came up
	 */
	return mfd;
}

/* passed as a start_routine to pthread_create */
void * mog_queue_loop(void *arg)
{
	struct mog_queue *q = arg;
	struct mog_fd *mfd = NULL;

	pthread_cleanup_push(queue_loop_cleanup, NULL);
	mog_cancel_disable();
	syslog(LOG_DEBUG, "mog_queue_loop[%lx] thread ready",
	       (unsigned long)pthread_self());

	for (;;) {
		while (mfd == NULL)
			mfd = mog_idleq_wait(q, -1);
		switch (mog_queue_step(mfd)) {
		case MOG_NEXT_ACTIVE:
			mfd = queue_xchg_maybe(q, mfd);
			break;
		case MOG_NEXT_WAIT_RD:
			mfd = mog_queue_xchg(q, mfd, MOG_QEV_RD);
			break;
		case MOG_NEXT_WAIT_WR:
			mfd = mog_queue_xchg(q, mfd, MOG_QEV_WR);
			break;
		case MOG_NEXT_IGNORE:
		case MOG_NEXT_CLOSE:
			/* already hanndled */
			mfd = mog_idleq_wait(q, -1);
		}
	}

	pthread_cleanup_pop(1);

	return NULL;
}

static void queue_quit_step(struct mog_fd *mfd)
{
	switch (mfd->fd_type) {
	case MOG_FD_TYPE_MGMT: mog_mgmt_quit_step(mfd); return;
	case MOG_FD_TYPE_HTTP:
	case MOG_FD_TYPE_HTTPGET:
		mog_http_quit_step(mfd); return;
	case MOG_FD_TYPE_FILE:
	case MOG_FD_TYPE_QUEUE:
	case MOG_FD_TYPE_SVC:
		assert(0 && "invalid fd_type in queue_quit_step");
	default:
		break;
	}
}

/* called at shutdown when only one thread is active */
void mog_queue_quit_loop(struct mog_queue *queue)
{
	struct mog_fd *mfd;

	while (mog_nr_active_at_quit) {
		assert(mog_nr_active_at_quit <= (size_t)INT_MAX
		       && "mog_nr_active_at_quit underflow");

		if ((mfd = mog_idleq_wait(queue, -1)))
			queue_quit_step(mfd);
	}
}
