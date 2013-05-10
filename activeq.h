/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */

static inline void mog_activeq_add(struct mog_queue *q, struct mog_fd *mfd)
{
	mog_idleq_add(q, mfd, MOG_QEV_RW);
}

static inline void mog_activeq_push(struct mog_queue *q, struct mog_fd *mfd)
{
	mog_idleq_push(q, mfd, MOG_QEV_RW);
}
