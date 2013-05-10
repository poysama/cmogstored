/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
bool mog_cloexec_atomic;

/*
 * The presence of O_CLOEXEC in headers doesn't mean the kernel supports it
 */
#if defined(O_CLOEXEC) && (O_CLOEXEC != 0) && \
    defined(SOCK_CLOEXEC) && \
    defined(HAVE_ACCEPT4)
__attribute__((constructor)) static void cloexec_detect(void)
{
	int flags;
	int fd = open("/dev/null", O_RDONLY|O_CLOEXEC);

	if (fd < 0) {
		if (errno == EINVAL) goto out;
		die_errno("open(/dev/null) failed");
	}

	flags = fcntl(fd, F_GETFD);
	if (flags != -1)
		mog_cloexec_atomic = ((flags & FD_CLOEXEC) == FD_CLOEXEC);
	mog_close(fd);

	if (! mog_cloexec_atomic) goto out;

	/* try to ensure sockets are sane, too */
	fd = socket(AF_INET, SOCK_STREAM|SOCK_CLOEXEC, 0);
	if (fd < 0) {
		if (errno == EINVAL) goto out;
		die_errno("socket(AF_INET, ...) failed");
	}
	flags = fcntl(fd, F_GETFD);
	if (flags != -1)
		mog_cloexec_atomic = ((flags & FD_CLOEXEC) == FD_CLOEXEC);
	mog_close(fd);

out:
	if (mog_cloexec_atomic)
		mog_cloexec_works();
	else
		warn("close-on-exec is NOT atomic");
}
#endif /* no O_CLOEXEC at all */
