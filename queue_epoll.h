/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#if defined(HAVE_EPOLL_WAIT) && ! MOG_LIBKQUEUE
#include <sys/epoll.h>

/*
 * EPOLLERR and EPOLLHUP are always set by default,
 * but there's no harm in setting them here
 */
#define MY_EP_FLAGS (EPOLLONESHOT|EPOLLHUP|EPOLLERR)
enum mog_qev {
	MOG_QEV_RD = EPOLLIN | MY_EP_FLAGS | EPOLLET,
	MOG_QEV_WR = EPOLLOUT | MY_EP_FLAGS | EPOLLET,
	MOG_QEV_RW = EPOLLIN | EPOLLOUT | MY_EP_FLAGS
};
#endif /* HAVE_EPOLL_WAIT */
