/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "compat_accept.h"

#define ENOSYS_msg \
  MOG_ACCEPT_FN" missing, rebuild on the same platform this runs on"

/* don't spam syslog on accept flood */
static void do_expire(struct mog_accept *ac)
{
	int err = errno;
	time_t now;
	static time_t last_expire;
	static pthread_mutex_t err_lock = PTHREAD_MUTEX_INITIALIZER;

	CHECK(int, 0, pthread_mutex_lock(&err_lock));

	now = time(NULL);
	if (last_expire == now)
		err = 0;
	else
		last_expire = now;

	CHECK(int, 0, pthread_mutex_unlock(&err_lock));

	if (err) {
		errno = err;
		syslog(LOG_ERR, MOG_ACCEPT_FN" failed with: %m");
	}

	mog_fdmap_expire(ac->svc->idle_timeout);
}

MOG_NOINLINE static void accept_error_check(struct mog_accept *ac)
{
	int fd;

	switch (errno) {
	case ECONNABORTED:
	case EINTR:
		return; /* common errors, nothing we can do about it */
	case EBADF:
		assert(0 && "BUG, called accept on bad FD");
	case ENOTSOCK:
	case EOPNOTSUPP:
		return pthread_exit(NULL);
	case_EAGAIN:
		/*
		 * listen socket could've been inherited from another process,
		 * we'll support that in the near future (like nginx/unicorn)
		 */
		fd = mog_fd_of(ac)->fd;
		if (mog_set_nonblocking(fd, false) != 0) {
			assert(errno != EBADF && "unexpected EBADF");
			syslog(LOG_ERR,
			       "failed to make fd=%d blocking: %m", fd);
		}
		syslog(LOG_DEBUG, "made fd=%d blocking", fd);
		return;
	case EMFILE:
	case ENFILE:
	case ENOBUFS:
	case ENOMEM:
		do_expire(ac);
		return;
	case ENOSYS:
		syslog(LOG_CRIT, ENOSYS_msg);
		die(ENOSYS_msg);
	default:
		syslog(LOG_ERR, MOG_ACCEPT_FN" failed with: %m");
	}
}

static void accept_loop_cleanup(void *ignored)
{
	mog_alloc_quit();
}

/*
 * passed as the start_routine argument to pthread_create.
 * This function may run concurrently in multiple threads.
 * The design of cmogstored assumes "wake-one" behavior for blocking
 * accept()/accept4() callers.  We will force accept_fd into blocking
 * state if O_NONBLOCK is ever set (e.g. listen socket was inherited).
 */
void *mog_accept_loop(void *arg)
{
	struct mog_accept *ac = arg;
	int accept_fd = mog_fd_of(ac)->fd;

	pthread_cleanup_push(accept_loop_cleanup, NULL);

	for (;;) {
		/* pthread cancellation point */
		int client_fd = mog_accept_fn(accept_fd, NULL, NULL);

		if (client_fd >= 0) {
			mog_cancel_disable();
			ac->post_accept_fn(client_fd, ac->svc);
			mog_cancel_enable();
		} else {
			accept_error_check(ac);
		}
	}

	pthread_cleanup_pop(1);

	return NULL;
}
