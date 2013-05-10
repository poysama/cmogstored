/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
/* epoll-specific parts see queue_common.c and activeq.c for the rest */
/*
 * a poll/select/libev/libevent-based implementation would have a hard time
 * migrating clients between threads
 */
#if defined(HAVE_EPOLL_WAIT) && ! MOG_LIBKQUEUE
#include "compat_epoll_pwait.h"
#include <sys/utsname.h>

/*
 * Detect old kernels with buggy EPOLL_CTL_MOD on SMP
 * This issue is fixed by Linux commit 128dd1759d96ad36c379240f8b9463e8acfd37a1
 * Remove this workaround around 2020 - 2023
 */
static bool epoll_ctl_mod_buggy;

__attribute__((constructor)) static void epoll_ctl_mod_buggy_detect(void)
{
	struct utsname buf;
	unsigned version, patchlevel, sublevel, extra;
	int rc;

	/*
	 * Online/current processors for this process is not enough,
	 * we need all processors since events may be triggered
	 * by interrupt handlers on any CPU in the system
	 */
	unsigned long nproc = num_processors(NPROC_ALL);

	/* Eric Wong's personal machines are ancient and weak: */
	if (nproc == 1)
		return;

	CHECK(int, 0, uname(&buf));

	/* who knows, maybe there'll be an epoll on other OSes one day */
	if (strcmp(buf.sysname, "Linux"))
		return;

	rc = sscanf(buf.release, "%u.%u.%u", &version, &patchlevel, &sublevel);
	if (rc != 3) {
		warn("sscanf failed to parse kernel version: %s (rc=%d), "
		     "assuming EPOLL_CTL_MOD is buggy on SMP",
		     buf.release, rc);
		epoll_ctl_mod_buggy = true;
		return;
	}

	/* TODO: whitelist vendor kernels as fixes are backported */
	if (version <= 2)
		epoll_ctl_mod_buggy = true;
	if (version != 3)
		return;

	/* v3.8-rc2+ has this fix (don't care about v3.8-rc1) */
	if (patchlevel >= 8)
		return;

	switch (patchlevel) {
	case 0: /* v3.0.59+ are good */
		epoll_ctl_mod_buggy = sublevel < 59;
		return;
	case 2: /* v3.2.37+ are good */
		epoll_ctl_mod_buggy = sublevel < 37;
		return;
	case 4: /* v3.4.26+ are good */
		epoll_ctl_mod_buggy = sublevel < 26;
		return;
	case 5: /* v3.5.7.3+ are good */
		/* (extended stable) git://kernel.ubuntu.com/ubuntu/linux.git */
		if (sublevel == 7) {
			rc = sscanf(buf.release, "%u.%u.%u.%u",
				    &version, &patchlevel, &sublevel, &extra);
			epoll_ctl_mod_buggy = (rc == 4) && (extra < 3);
		} else {
			epoll_ctl_mod_buggy = true;
		}
		/* v3.5.8 probably will not happen ... */
		return;
	case 7: /* v3.7.3+ are good */
		epoll_ctl_mod_buggy = sublevel < 3;
		return;
	case 1: /* v3.1 seems abandoned */
	case 3: /* v3.3 seems abandoned */
	case 6: /* v3.6 seems abandoned */
		epoll_ctl_mod_buggy = true;
	}
}

struct mog_queue * mog_queue_new(void)
{
	int size_hint = 666; /* hint, ignored in new kernels */
	int epoll_fd = epoll_create(size_hint);
	if (epoll_fd < 0) die_errno("epoll_create() failed");

	return mog_queue_init(epoll_fd);
}

static struct mog_fd * epoll_event_check(int rc, struct epoll_event *event)
{
	struct mog_fd *mfd;

	switch (rc) {
	case 1:
		mfd = event->data.ptr;
		mog_fd_check_out(mfd);
		return mfd;
	case 0:
		return NULL;
	}

	if (errno != EINTR)
		/* rc could be > 1 if the kernel is broken :P */
		die_errno("epoll_wait() failed with (%d)", rc);
	return NULL;
}

/*
 * grabs one active event off the event queue
 * epoll_wait() has "wake-one" behavior (like accept())
 * to avoid thundering herd since 2007
 */
struct mog_fd * mog_idleq_wait(struct mog_queue *q, int timeout)
{
	int rc;
	struct epoll_event event;
	bool cancellable = timeout != 0;

	if (cancellable)
		mog_cancel_enable();

	/* epoll_wait is a cancellation point since glibc 2.4 */
	rc = epoll_wait(q->queue_fd, &event, 1, timeout);

	if (cancellable)
		mog_cancel_disable();
	return epoll_event_check(rc, &event);
}

struct mog_fd * mog_idleq_wait_intr(struct mog_queue *q, int timeout)
{
	int rc;
	struct epoll_event event;
	sigset_t set;

	CHECK(int, 0, sigemptyset(&set));
	rc = epoll_pwait(q->queue_fd, &event, 1, timeout, &set);
	return epoll_event_check(rc, &event);
}

MOG_NOINLINE static void
epoll_ctl_error(struct mog_queue *q, struct mog_fd *mfd)
{
	switch (errno) {
	case ENOMEM:
	case ENOSPC:
		syslog(LOG_ERR, "epoll_ctl: %m, dropping file descriptor");
		mog_fd_put(mfd);
		return;
	default:
		syslog(LOG_ERR, "unhandled epoll_ctl() error: %m");
		assert(0 && "BUG in our usage of epoll");
	}
}

/*
 * Pushes in one mog_fd for epoll to watch.
 *
 * Only call this from the mog_accept_loop *or*
 * if EAGAIN/EWOULDBLOCK is encountered in mog_queue_loop.
 */
static void
idleq_mod(struct mog_queue *q, struct mog_fd *mfd, enum mog_qev ev, int op)
{
	struct epoll_event event;

	event.data.ptr = mfd;
	event.events = (uint32_t)ev;

	mog_fd_check_in(mfd);
	if (epoll_ctl(q->queue_fd, op, mfd->fd, &event) != 0) {
		mog_fd_check_out(mfd);
		epoll_ctl_error(q, mfd);
	}
}

void mog_idleq_add(struct mog_queue *q, struct mog_fd *mfd, enum mog_qev ev)
{
	idleq_mod(q, mfd, ev, EPOLL_CTL_ADD);
}

/*
 * Workaround buggy EPOLL_CTL_MOD race by combining EPOLL_CTL_DEL
 * and EPOLL_CTL_ADD for the same effect (with more syscall overhead)
 */
static void
fake_epoll_ctl_mod(struct mog_queue *q, struct mog_fd *mfd, enum mog_qev ev)
{
	struct epoll_event event;

	if (epoll_ctl(q->queue_fd, EPOLL_CTL_DEL, mfd->fd, &event) == 0)
		idleq_mod(q, mfd, ev, EPOLL_CTL_ADD);
	else
		epoll_ctl_error(q, mfd);
}

void mog_idleq_push(struct mog_queue *q, struct mog_fd *mfd, enum mog_qev ev)
{
	if (epoll_ctl_mod_buggy)
		fake_epoll_ctl_mod(q, mfd, ev);
	else
		idleq_mod(q, mfd, ev, EPOLL_CTL_MOD);
}

struct mog_fd *
mog_queue_xchg(struct mog_queue *q, struct mog_fd *mfd, enum mog_qev ev)
{
	/* epoll need two (or three) syscalls to implement this */
	mog_idleq_push(q, mfd, ev);
	return mog_idleq_wait(q, -1);
}
#else /* ! HAVE_EPOLL_WAIT */
typedef int avoid_empty_file;
#endif /* ! HAVE_EPOLL_WAIT */
