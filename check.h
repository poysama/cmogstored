/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */

/* This header is only used by C test programs */
#ifdef NDEBUG
#  undef NDEBUG
#endif
#include "cmogstored.h"
#include <sys/ioctl.h>

static inline void pipe_or_die(int *fds)
{
	int rc = pipe(fds);

	assert(rc == 0 && "pipe failed");
}

static inline void socketpair_or_die(int *fds)
{
	int rc = socketpair(AF_UNIX, SOCK_STREAM, 0, fds);

	assert(rc == 0 && "socketpair failed");
}

static inline void close_pipe(int *fds)
{
	assert(0 == close(fds[0]) && "close(fd[0]) failed");
	assert(0 == close(fds[1]) && "close(fd[1]) failed");
}

/* stub for tests */
void cmogstored_quit(void) {}
