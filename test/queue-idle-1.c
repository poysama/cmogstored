/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "check.h"
static int fds[2];
static char buf[128];
static struct mog_queue *q;
static struct mog_fd *mfd;

static void setup(void)
{
	q = mog_queue_new();
	pipe_or_die(fds);
	mfd = mog_fd_get(fds[0]);
	mfd->fd = fds[0];

	mog_set_nonblocking(fds[0], true);
	assert(read(fds[0], buf, sizeof(buf)) == -1 &&
	       errno == EAGAIN && "read() should EAGAIN");
}

static void teardown(void)
{
	close_pipe(fds);
}

static void test_nonblocking(void)
{
	setup();

	mog_idleq_add(q, mfd, MOG_QEV_RD);
	assert(NULL == mog_idleq_wait(q, 0) && "q wait should return NULL");
	assert(1 == write(fds[1], ".", 1) && "couldn't write");
	assert(mfd == mog_idleq_wait(q, 0) && "q wait should return mfd");

	teardown();
}

static void * wait_then_write(void *arg)
{
	sleep(1);
	assert(1 == write(fds[1], "B", 1) && "couldn't write");

	return NULL;
}

static void test_blocking(void)
{
	pthread_t thr;

	setup();

	mog_idleq_add(q, mfd, MOG_QEV_RD);
	CHECK(int, 0, pthread_create(&thr, NULL, wait_then_write, NULL));
	printf("start wait: %d\n", (int)time(NULL));
	mog_cancel_disable();
	assert(mfd == mog_idleq_wait(q, -1));
	printf("  end wait: %d\n", (int)time(NULL));
	assert(1 == read(fds[0], buf, 1) && "read failed");
	assert(buf[0] == 'B' && "didn't read expected 'B'");

	teardown();
}

int main(void)
{
	test_nonblocking();
	test_blocking();

	return 0;
}
