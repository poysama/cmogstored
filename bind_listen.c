/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

/* Under FreeBSD, TCP_NOPUSH is inherited by accepted sockets */
static int tcp_nopush_prepare(int fd)
{
	socklen_t len = (socklen_t)sizeof(int);
	int val = 1;

	if (MOG_TCP_NOPUSH == 0)
		return 0;

	return setsockopt(fd, IPPROTO_TCP, MOG_TCP_NOPUSH, &val, len);
}

/*
 * TODO
 * - configurable socket buffer sizes (where to put config?)
 * - configurable listen() backlog (where to put config?)
 *
 * TCP_DEFER_ACCEPT is probably not worth using on Linux
 * ref:
 *   https://bugs.launchpad.net/ubuntu/+source/apache2/+bug/134274
 *   http://labs.apnic.net/blabs/?p=57
 */

static int set_tcp_opts(int fd, const char *accept_filter)
{
	int val;
	socklen_t len = sizeof(int);
	int rc;

	val = 1;
	rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, len);
	if (rc < 0) return rc;

	val = 1;
	rc = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, len);
	if (rc < 0) return rc;

	val = 1;
	rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, len);
	if (rc < 0) return rc;

	if (accept_filter) {
		if (strcmp(accept_filter, "httpready") == 0)
			rc = tcp_nopush_prepare(fd);
	}

	return rc;
}

int mog_bind_listen(struct addrinfo *r, const char *accept_filter)
{
	/* see if we inherited the socket, first */
	int fd = mog_inherit_get(r->ai_addr, r->ai_addrlen);

	if (fd >= 0)
		return fd;

	for (; r; r = r->ai_next) {
		fd = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
		if (fd < 0)
			continue;

		/*
		 * We'll need to unset FD_CLOEXEC in the child for upgrades
		 * Leave FD_CLOEXEC set because we fork+exec iostat(1)
		 * frequently.  We can't guarantee SOCK_CLOEXEC works
		 * everywhere yet (in 2012).
		 */
		if (mog_set_cloexec(fd, true) == 0 &&
		    set_tcp_opts(fd, accept_filter) == 0 &&
		    bind(fd, r->ai_addr, r->ai_addrlen) == 0 &&
		    listen(fd, 1024) == 0)
			break;

		PRESERVE_ERRNO( mog_close(fd) );
		fd = -1;
	}

	return fd;
}
