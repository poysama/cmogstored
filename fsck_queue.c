/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
static pthread_mutex_t fsck_queue_lock = PTHREAD_MUTEX_INITIALIZER;
static Hash_table *fsck_queues;

struct fsck_queue {
	dev_t st_dev;
	SIMPLEQ_HEAD(fsck_fd, mog_mgmt) qhead;
	size_t active;
	size_t max_active;
	pthread_mutex_t qlock;
};

static bool fq_cmp(const void *a, const void *b)
{
	const struct fsck_queue *fqa = a;
	const struct fsck_queue *fqb = b;

	return fqa->st_dev == fqb->st_dev;
}

static size_t fq_hash(const void *x, size_t tablesize)
{
	const struct fsck_queue *fq = x;

	return fq->st_dev % tablesize;
}

static void fsck_queue_atexit(void)
{
	hash_free(fsck_queues);
}

MOG_NOINLINE static void fsck_queue_once(void)
{
	fsck_queues = hash_initialize(7, NULL, fq_hash, fq_cmp, free);
	atexit(fsck_queue_atexit);
}

static struct fsck_queue * fsck_queue_for(struct mog_mgmt *mgmt)
{
	struct fsck_queue tmpq;
	struct fsck_queue *fq;
	struct stat sb;

	assert(mgmt->forward && "no file open for fsck MD5");

	if (fstat(mgmt->forward->fd, &sb) < 0) {
		assert(errno != EBADF && "fstat on closed fd");
		syslog(LOG_ERR, "fstat() failed for MD5 req: %m");
		/* continue to a better error handling path */
		return NULL;
	}

	tmpq.st_dev = sb.st_dev;

	CHECK(int, 0, pthread_mutex_lock(&fsck_queue_lock));

	if (fsck_queues) {
		fq = hash_lookup(fsck_queues, &tmpq);
		if (fq)
			goto out;
	} else {
		fsck_queue_once();
	}

	fq = mog_cachealign(sizeof(struct fsck_queue));
	fq->st_dev = sb.st_dev;
	SIMPLEQ_INIT(&fq->qhead);
	fq->active = 0;
	fq->max_active = 1; /* TODO: tunable */
	CHECK(int, 0, pthread_mutex_init(&fq->qlock, NULL));

	CHECK(int, 1, hash_insert_if_absent(fsck_queues, fq, NULL));
out:
	CHECK(int, 0, pthread_mutex_unlock(&fsck_queue_lock));

	return fq;
}

bool mog_fsck_queue_ready(struct mog_fd *mfd)
{
	struct mog_mgmt *mgmt = &mfd->as.mgmt;
	struct fsck_queue *fq = fsck_queue_for(mgmt);
	bool rv;

	/* hopefully continue to a better error handling path on error */
	if (fq == NULL)
		return true;

	assert(mgmt->prio == MOG_PRIO_FSCK && "bad prio");

	CHECK(int, 0, pthread_mutex_lock(&fq->qlock));
	if (fq->active < fq->max_active) {
		fq->active++;
		rv = true;
	} else {
		SIMPLEQ_INSERT_TAIL(&fq->qhead, mgmt, fsckq);
		rv = false;
	}
	CHECK(int, 0, pthread_mutex_unlock(&fq->qlock));

	return rv;
}

void mog_fsck_queue_next(struct mog_fd *mfd)
{
	struct mog_mgmt *mgmt = &mfd->as.mgmt;
	struct fsck_queue *fq = fsck_queue_for(mgmt);
	struct mog_mgmt *next_mgmt = NULL;

	if (fq == NULL)
		return; /* hopefully continue to a better error handling path */

	assert(mgmt->prio == MOG_PRIO_FSCK && "bad prio");

	CHECK(int, 0, pthread_mutex_lock(&fq->qlock));
	fq->active--;
	if (fq->active < fq->max_active) {
		next_mgmt = SIMPLEQ_FIRST(&fq->qhead);
		if (next_mgmt)
			SIMPLEQ_REMOVE_HEAD(&fq->qhead, fsckq);
	}
	CHECK(int, 0, pthread_mutex_unlock(&fq->qlock));

	if (next_mgmt) {
		assert(next_mgmt->prio == MOG_PRIO_FSCK && "bad prio");
		mog_activeq_push(next_mgmt->svc->queue, mog_fd_of(next_mgmt));
	}
}
