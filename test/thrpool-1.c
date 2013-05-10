/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
/* ensure we can start and stop thread pools properly */
#include "check.h"
#include <sys/time.h>

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static struct timeval tv;

void *fn(void *xarg)
{
	const char *s = xarg;
	assert(strcmp("whazzup", s) == 0 && "arg passed wrong");

	for (;;) {
		struct timespec t;
		t.tv_nsec = tv.tv_usec * 1000;
		t.tv_sec = tv.tv_sec + 1;
		if (0 && t.tv_nsec >= 1000000000) {
			t.tv_nsec -= 1000000000;
			t.tv_sec++;
		}

		mog_cancel_disable();
		CHECK(int, 0, pthread_mutex_lock(&lock));
		pthread_cond_timedwait(&cond, &lock, &t);
		CHECK(int, 0, pthread_mutex_unlock(&lock));
		mog_cancel_enable();
		pthread_testcancel();
	}
	assert(strcmp("whazzup", s) == 0 && "arg changed");

	return NULL;
}

int main(void)
{
	static struct mog_thrpool tp;
	char *tmp = xstrdup("whazzup");
	struct timespec t;

	CHECK(int, 0, gettimeofday(&tv, NULL));
	t.tv_nsec = tv.tv_usec * 1000;
	t.tv_sec = tv.tv_sec + 1;

	mog_thrpool_start(&tp, 6, fn, (void *)tmp);

	CHECK(int, 0, pthread_mutex_lock(&lock));
	CHECK(int, ETIMEDOUT, pthread_cond_timedwait(&cond, &lock, &t));
	CHECK(int, 0, pthread_mutex_unlock(&lock));

	mog_thrpool_quit(&tp, NULL);

	free(tmp);

	return 0;
}
