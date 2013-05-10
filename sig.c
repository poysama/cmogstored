/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

/*
 * we block signals in pool threads, only the main thread receives signals
 */

void mog_intr_disable(void)
{
	sigset_t set;

	CHECK(int, 0, sigfillset(&set));
	CHECK(int, 0, pthread_sigmask(SIG_SETMASK, &set, NULL));
}

void mog_intr_enable(void)
{
	sigset_t set;

	CHECK(int, 0, sigemptyset(&set));
	CHECK(int, 0, pthread_sigmask(SIG_SETMASK, &set, NULL));
}

/*
 * favor ppoll if available since this is our only pselect user and
 * would increase the size of the executable
 */
#ifdef HAVE_PPOLL
static void sleeper(struct timespec *tsp, const sigset_t *sigmask)
{
	if (ppoll(NULL, 0, tsp, sigmask) < 0)
		assert((errno == EINTR || errno == ENOMEM) &&
		       "BUG in ppoll usage");
}
#else /* PSELECT */
static void sleeper(struct timespec *tsp, const sigset_t *sigmask)
{
	if (pselect(0, NULL, NULL, NULL, tsp, sigmask) < 0)
		assert((errno == EINTR || errno == ENOMEM) &&
		       "BUG in pselect usage");
}
#endif /* PSELECT */

/* thread-safe, interruptible sleep, negative seconds -> sleep forever */
void mog_sleep(long seconds)
{
	sigset_t set;
	struct timespec ts;
	struct timespec *tsp;

	if (seconds < 0) {
		tsp = NULL;
	} else {
		ts.tv_sec = seconds;
		ts.tv_nsec = 0;
		tsp = &ts;
	}

	CHECK(int, 0, sigemptyset(&set));
	sleeper(tsp, &set);
}
