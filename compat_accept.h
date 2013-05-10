/*
 * accept()/accept4() wrappers
 *
 * It's tricky for gnulib to work with SOCK_CLOEXEC/SOCK_NONBLOCK,
 * and any emulation would not be thread-safe.  So we'll handle
 * accept4()-compatibility for ourselves.
 *
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */

#if defined(HAVE_ACCEPT4) && defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)
#define MOG_ACCEPT_FN "accept4()"
static inline int
mog_accept_fn(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	return accept4(sockfd, addr, addrlen, SOCK_CLOEXEC|SOCK_NONBLOCK);
}
#else
#define MOG_ACCEPT_FN "accept()"
static inline int
mog_accept_fn(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	int fd = accept(sockfd, addr, addrlen);

	/*
	 * If we don't have a real accept4() syscall, don't
	 * bother setting O_CLOEXEC here.  Systems without
	 * accept4() will end up calling mog_cloexec_from()
	 * anyways in a fork()-ed child
	 */
	if (fd >= 0)
		CHECK(int, 0, mog_set_nonblocking(fd, true));
	return fd;
}
#endif /* HAVE_ACCEPT4 */
