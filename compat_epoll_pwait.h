/*
 * fake epoll_pwait() implemented using ppoll + epoll_wait.
 * This is only for Linux 2.6.18 / glibc 2.5 systems (Enterprise distros :P)
 *
 * Copyright (C) 2012-2013 Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */

#if !defined(HAVE_EPOLL_PWAIT) \
    && defined(HAVE_PPOLL) && defined(HAVE_EPOLL_WAIT)
static int my_epoll_pwait(int epfd, struct epoll_event *events,
                          int maxevents, int timeout, const sigset_t *sigmask)
{
	struct pollfd pfds = { .fd = epfd, .events = POLLIN };
	int rc;
	struct timespec ts;
	struct timespec *tsp;

	if (timeout < 0) {
		tsp = NULL;
	} else {
		ts.tv_sec = timeout / 1000;
		ts.tv_nsec = (timeout % 1000) * 1000000;
		tsp = &ts;
	}

	/* wait on just the epoll descriptor itself */
	rc = ppoll(&pfds, 1, tsp, sigmask);
	if (rc < 0)
		assert((errno == EINTR || errno == ENOMEM)
		       && "ppoll usage bug");

	/* no sleep for the actual epoll call */
	return rc > 0 ? epoll_wait(epfd, events, maxevents, 0) : 0;
}
#define epoll_pwait(epfd,events,maxevents,timeout,sigmask) \
        my_epoll_pwait((epfd),(events),(maxevents),(timeout),(sigmask))
#endif /* epoll_pwait */
