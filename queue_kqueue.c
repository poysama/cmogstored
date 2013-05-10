/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
/*
 * a poll/select/libev/libevent-based implementation would have a hard time
 * migrating clients between threads
 */
#ifdef HAVE_KQUEUE

struct mog_queue * mog_queue_new(void)
{
	int kqueue_fd = kqueue();

	if (kqueue_fd < 0)
		die_errno("kqueue() failed");

	return mog_queue_init(kqueue_fd);
}

static void check_cancel(void)
{
	mog_cancel_enable();
	pthread_testcancel();
	mog_cancel_disable();
}

/*
 * grabs one active event off the event queue
 */
struct mog_fd * mog_idleq_wait(struct mog_queue *q, int timeout)
{
	int rc;
	struct mog_fd *mfd;
	struct kevent event;
	struct timespec ts;
	struct timespec *tsp;
	bool cancellable = timeout != 0;

	if (timeout < 0) {
		tsp = NULL;
	} else {
		ts.tv_sec = timeout / 1000;
		ts.tv_nsec = (timeout % 1000) * 1000000;
		tsp = &ts;
	}

	/*
	 * we enable SIGURG from pthread_kill() in thrpool.c when sleeping
	 * in kevent().  This allows us to wake up an respond to a
	 * cancellation request (since kevent() is not a cancellation point).
	 */
	if (cancellable) {
		check_cancel();
		mog_intr_enable();
	}

	rc = kevent(q->queue_fd, NULL, 0, &event, 1, tsp);

	if (cancellable)
		PRESERVE_ERRNO( mog_intr_disable() );

	if (rc > 0) {
		mfd = event.udata;
		mog_fd_check_out(mfd);

		/* ignore pending cancel until the next round */
		return mfd;
	}
	if (cancellable)
		check_cancel();
	if (rc == 0)
		return NULL;

	if (errno != EINTR)
		die_errno("kevent(wait) failed with (%d)", rc);

	return NULL;
}

struct mog_fd * mog_idleq_wait_intr(struct mog_queue *q, int timeout)
{
	struct mog_fd *mfd;

	/* this is racy, using a self-pipe covers the race */
	mog_intr_enable();
	mfd = mog_idleq_wait(q, timeout);
	mog_intr_disable();
	return mfd;
}

MOG_NOINLINE static void
kevent_add_error(struct mog_queue *q, struct mog_fd *mfd)
{
	switch (errno) {
	case ENOMEM:
		syslog(LOG_ERR,
		      "kevent(EV_ADD) out-of-space, dropping file descriptor");
		mog_fd_put(mfd);
		return;
	default:
		syslog(LOG_ERR, "unhandled kevent(EV_ADD) error: %m");
		assert(0 && "BUG in our usage of kevent");
	}
}

static int add_event(int kqueue_fd, struct kevent *event)
{
	int rc;

	do {
		rc = kevent(kqueue_fd, event, 1, NULL, 0, NULL);
	} while (rc < 0 && errno == EINTR);

	return rc;
}

static void qpush(struct mog_queue *q, struct mog_fd *mfd, enum mog_qev ev)
{
	struct kevent event;
	u_short flags = EV_ADD | EV_ONESHOT;

	EV_SET(&event, mfd->fd, (short)ev, flags, 0, 0, mfd);

	mog_fd_check_in(mfd);
	if (add_event(q->queue_fd, &event) != 0) {
		mog_fd_check_out(mfd);
		kevent_add_error(q, mfd);
	}
}

/*
 * Pushes in one mog_fd for kqueue to watch.
 *
 * Only call this with MOG_QEV_RW *or* if EAGAIN/EWOULDBLOCK is
 * encountered in mog_queue_loop.
 */
void mog_idleq_push(struct mog_queue *q, struct mog_fd *mfd, enum mog_qev ev)
{
	if (ev == MOG_QEV_RW) {
		switch (mfd->fd_type) {
		case MOG_FD_TYPE_IOSTAT:
		case MOG_FD_TYPE_SELFWAKE:
			ev = MOG_QEV_RD;
			break;
		case MOG_FD_TYPE_UNUSED:
		case MOG_FD_TYPE_ACCEPT:
		case MOG_FD_TYPE_FILE:
		case MOG_FD_TYPE_QUEUE:
		case MOG_FD_TYPE_SVC:
			assert(0 && "invalid fd_type for mog_idleq_push");
		default:
			ev = MOG_QEV_WR;
			break;
		}
	}
	qpush(q, mfd, ev);
}

void mog_idleq_add(struct mog_queue *q, struct mog_fd *mfd, enum mog_qev ev)
{
	mog_idleq_push(q, mfd, ev);
}

struct mog_fd *
mog_queue_xchg(struct mog_queue *q, struct mog_fd *mfd, enum mog_qev ev)
{
	/*
	 * kqueue() _should_ be able to implement this function with
	 * one syscall, however, we currently rely on mog_idleq_wait()
	 * being a cancellation point.  So we must ensure the mfd is
	 * back in the queue (for other threads to access) before
	 * cancelling this thread...
	 */
	mog_idleq_push(q, mfd, ev);

	return mog_idleq_wait(q, -1);
}
#else /* ! HAVE_KQUEUE */
typedef int avoid_empty_file;
#endif /* ! HAVE_KQUEUE */
