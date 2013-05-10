/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#ifdef HAVE_KQUEUE
#include <sys/types.h>
#include <sys/event.h>
enum mog_qev {
	MOG_QEV_RD = EVFILT_READ,
	MOG_QEV_WR = EVFILT_WRITE,
	MOG_QEV_RW = 0
};
struct mog_queue;
struct mog_fd;
#endif /* HAVE_KQUEUE */

#if defined(LIBKQUEUE) && (LIBKQUEUE == 1)
#  define MOG_LIBKQUEUE (true)
#else
#  define MOG_LIBKQUEUE (false)
#endif
