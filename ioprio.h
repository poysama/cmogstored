/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */

#include <sys/syscall.h>

#if defined(SYS_ioprio_get) && defined(SYS_ioprio_set)

#ifndef IOPRIO_PRIO_CLASS
#include "ioprio_linux.h"
#endif /* Linux headers */

/*
 * specifying which == IOPRIO_WHO_PROCESS and who == 0
 * means ioprio works on the current task in Linux.
 * This means we can avoid a syscall for gettid()
 */

static inline int mog_ioprio_get(int which, int who)
{
	return syscall(SYS_ioprio_get, which, who);
}

static inline int mog_ioprio_set(int which, int who, int ioprio)
{
	return syscall(SYS_ioprio_set, which, who, ioprio);
}

static int mog_ioprio_drop(void)
{
	int rc;
	int oldprio = mog_ioprio_get(IOPRIO_WHO_PROCESS, 0);
	int newprio;
	static int warned;

	if (oldprio == -1)
		return -1;

	/* don't bother unless it's already best-effort */
	switch (IOPRIO_PRIO_CLASS(oldprio)) {
	case IOPRIO_CLASS_NONE:
		/*
		 * we have to filter out data for ioprio_set to succeed,
		 * NONE means the task gets ioprio based on niceness
		 */
		oldprio = IOPRIO_PRIO_VALUE(IOPRIO_CLASS_NONE, 0);
	case IOPRIO_CLASS_BE:
		break; /* OK */
	default:
		return -1;
	}

	newprio = IOPRIO_PRIO_VALUE(IOPRIO_CLASS_BE, 7);
	rc = mog_ioprio_set(IOPRIO_WHO_PROCESS, 0, newprio);
	if (rc == 0)
		return oldprio;

	if (!warned) {
		warned = 1;
		syslog(LOG_WARNING, "failed to drop IO priority: %m");
	}
	return -1;
}

static void mog_ioprio_restore(int ioprio)
{
	static int warned;
	int rc = mog_ioprio_set(IOPRIO_WHO_PROCESS, 0, ioprio);

	if (rc < 0 && !warned) {
		warned = 1;
		syslog(LOG_WARNING, "failed to restore IO priority: %m");
	}
}

#else /* workalikes */
static inline int mog_ioprio_drop(void)
{
	return 0;
}

static inline void mog_ioprio_restore(int ioprio)
{
}
#endif
