/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

/*
 * access to this should only be called in the main thread, this
 * is currently not thread safe as there's no need for it.
 */
static LIST_HEAD(queue_head, mog_queue) all_queues;

struct mog_queue *mog_queue_init(int queue_fd)
{
	struct mog_fd *mfd;
	struct mog_queue *q;

	/*
	 * Do not bother with epoll_create1(EPOLL_CLOEXEC),
	 * there's no kqueue version of it.  We only create epoll/kqueue
	 * descriptors before we'd ever fork anything
	 */
	CHECK(int, 0, mog_set_cloexec(queue_fd, true));

	mfd = mog_fd_init(queue_fd, MOG_FD_TYPE_QUEUE);
	q = &mfd->as.queue;
	q->queue_fd = queue_fd;
	memset(&q->thrpool, 0, sizeof(struct mog_thrpool));
	LIST_INSERT_HEAD(&all_queues, q, qbuddies);

	return q;
}

void mog_queue_stop(struct mog_queue *keep)
{
	struct mog_queue *queue, *tmp;
	struct mog_fd *mfd;

	LIST_FOREACH_SAFE(queue, &all_queues, qbuddies, tmp) {
		/* keep is usually mog_notify_queue */
		if (queue == keep)
			continue;
		LIST_REMOVE(queue, qbuddies);
		mog_thrpool_quit(&queue->thrpool, queue);
		mfd = mog_fd_of(queue);
		mog_fd_put(mfd);
	}
}
