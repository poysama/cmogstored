/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
void mog_close(int fd)
{
	if (close(fd) == 0)
		return;

	switch (errno) {
	case ECONNRESET:
		/*
		 * FreeBSD (and other BSDs) may return ECONNRESET on close(),
		 * but the file descriptor _does_ seem to be released.
		 * Retrying close() will break since we create descriptors
		 * in different threads
		 */
	case EINTR: return; /* nothing we can do */
	case EBADF:
		/* EBADF would be a disaster since we use threads */
		syslog(LOG_CRIT, "BUG: attempted to close(fd=%d)", fd);
		assert(0 && fd && "won't attempt to continue on bad close()");
	default: /* EIO, nothing we can do... */
		syslog(LOG_ERR, "close(fd=%d) failed: %m", fd);
	}
}
