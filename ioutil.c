/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

static pthread_mutex_t cleanup_lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t iou_lock = PTHREAD_MUTEX_INITIALIZER;
static Hash_table *dev_iou; /* maps system device IDs to utilization */
struct ioutil;
struct ioutil {
	dev_t st_dev;
	bool in_use;
	struct ioutil *free_next;
	char util[MOG_IOUTIL_LEN];
};

static size_t iou_hash(const void *entry, size_t tablesize)
{
	const struct ioutil *iou = entry;

	return iou->st_dev % tablesize;
}

static bool iou_cmp(const void *_a, const void *_b)
{
	const struct ioutil *a = _a;
	const struct ioutil *b = _b;

	return a->st_dev == b->st_dev;
}

__attribute__((destructor)) static void iou_destructor(void)
{
	hash_free(dev_iou);
}

__attribute__((constructor)) static void iou_constructor(void)
{
	dev_iou = hash_initialize(7, NULL, iou_hash, iou_cmp, free);
	if (!dev_iou)
		mog_oom();
}

static bool cleanup_begin_i(void *ent, void *unused)
{
	struct ioutil *iou = ent;
	iou->in_use = false;
	return true;
}

void mog_iou_cleanup_begin(void)
{
	CHECK(int, 0, pthread_mutex_lock(&cleanup_lock));
	CHECK(int, 0, pthread_mutex_lock(&iou_lock));
	hash_do_for_each(dev_iou, cleanup_begin_i, NULL);
	CHECK(int, 0, pthread_mutex_unlock(&iou_lock));
}

static bool freelist_append(void *ent, void *f)
{
	struct ioutil *iou = ent;
	struct ioutil **free_head = f;

	if (iou->in_use)
		return true;

	assert(iou->free_next == NULL && "free_next set");

	/* prepend current item to the free list */
	iou->free_next = *free_head;
	*free_head = iou;

	return true;
}

void mog_iou_cleanup_finish(void)
{
	struct ioutil *fl = NULL;

	CHECK(int, 0, pthread_mutex_lock(&iou_lock));

	/* build up the free list */
	hash_do_for_each(dev_iou, freelist_append, &fl);

	/* release items in the free list */
	while (fl) {
		struct ioutil *next = fl->free_next;
		struct ioutil *found = hash_delete(dev_iou, fl);
		assert(found == fl && "freelist found does not match");
		free(fl);
		fl = next;
	}

	CHECK(int, 0, pthread_mutex_unlock(&iou_lock));
	CHECK(int, 0, pthread_mutex_unlock(&cleanup_lock));
}

static struct ioutil * iou_vivify(dev_t st_dev)
{
	struct ioutil lookup = { .st_dev = st_dev };
	struct ioutil *iou = hash_lookup(dev_iou, &lookup);

	if (!iou) {
		iou = xmalloc(sizeof(*iou));
		iou->st_dev = st_dev;
		iou->util[0] = '-';
		iou->util[1] = 0;
		iou->free_next = NULL;
		CHECK(int, 1, hash_insert_if_absent(dev_iou, iou, NULL));
	}
	iou->in_use = true;

	return iou;
}

void mog_iou_read(dev_t st_dev, char buf[MOG_IOUTIL_LEN])
{
	struct ioutil *iou;

	CHECK(int, 0, pthread_mutex_lock(&iou_lock));
	iou = iou_vivify(st_dev);
	memcpy(buf, iou->util, MOG_IOUTIL_LEN);
	CHECK(int, 0, pthread_mutex_unlock(&iou_lock));
}

void mog_iou_write(dev_t st_dev, const char buf[MOG_IOUTIL_LEN])
{
	struct ioutil *iou;

	CHECK(int, 0, pthread_mutex_lock(&iou_lock));
	iou = iou_vivify(st_dev);
	memcpy(iou->util, buf, MOG_IOUTIL_LEN);
	CHECK(int, 0, pthread_mutex_unlock(&iou_lock));
}

/* marks the given device as in-use */
void mog_iou_active(dev_t st_dev)
{
	CHECK(int, 0, pthread_mutex_lock(&iou_lock));
	(void)iou_vivify(st_dev);
	CHECK(int, 0, pthread_mutex_unlock(&iou_lock));
}
