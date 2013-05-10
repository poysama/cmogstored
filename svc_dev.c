/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "compat_memstream.h"

/*
 * maps multiple "devXXX" directories to the device.
 * This is uncommon in real world deployments (multiple mogdevs sharing
 * the same system device), but happens frequently in testing
 */
struct mog_devlist {
	dev_t st_dev;
	Hash_table *by_mogdevid;
};

static size_t devlist_hash(const void *x, size_t tablesize)
{
	const struct mog_devlist *devlist = x;

	return devlist->st_dev % tablesize;
}

static bool devlist_cmp(const void *a, const void *b)
{
	const struct mog_devlist *devlist_a = a;
	const struct mog_devlist *devlist_b = b;

	return devlist_a->st_dev == devlist_b->st_dev;
}

static void devlist_free(void *x)
{
	struct mog_devlist *devlist = x;

	hash_free(devlist->by_mogdevid);
	free(devlist);
}

static size_t devid_hash(const void *x, size_t tablesize)
{
	const struct mog_dev *dev = x;

	return dev->devid % tablesize;
}

static bool devid_cmp(const void *a, const void *b)
{
	const struct mog_dev *dev_a = a;
	const struct mog_dev *dev_b = b;

	return dev_a->devid == dev_b->devid;
}

static struct mog_devlist * mog_devlist_new(dev_t st_dev)
{
	struct mog_devlist *devlist = xmalloc(sizeof(struct mog_devlist));

	devlist->st_dev = st_dev;
	devlist->by_mogdevid = hash_initialize(7, NULL,
	                                       devid_hash, devid_cmp, free);

	return devlist;
}

/* ensures svc has a devlist, this must be called with devstats_lock held */
static struct mog_devlist * svc_devlist(struct mog_svc *svc, dev_t st_dev)
{
	struct mog_devlist *devlist;
	struct mog_devlist finder;

	assert(svc->by_st_dev && "by_st_dev unintialized in svc");

	finder.st_dev = st_dev;
	devlist = hash_lookup(svc->by_st_dev, &finder);

	if (devlist == NULL) {
		devlist = mog_devlist_new(st_dev);
		switch (hash_insert_if_absent(svc->by_st_dev, devlist, NULL)) {
		case 0:
			assert(0 && "race condition, devlist should insert "
			       "without error");
			abort();
			break;
		case 1: break; /* OK, inserted */
		default: mog_oom(); /* -1 */
		}
	}
	return devlist;
}

static void svc_init_dev_hash(struct mog_svc *svc)
{
	if (svc->by_st_dev) {
		hash_clear(svc->by_st_dev);
		return;
	}

	svc->by_st_dev = hash_initialize(7, NULL, devlist_hash,
	                                 devlist_cmp, devlist_free);
	if (!svc->by_st_dev)
		mog_oom();
}

static int svc_scandev(struct mog_svc *svc, size_t *nr, mog_scandev_cb cb)
{
	struct dirent *ent;
	int rc = 0;

	if (svc->mgmt_fd < 0)
		return 0;

	CHECK(int, 0, pthread_mutex_lock(&svc->devstats_lock));
	svc_init_dev_hash(svc);
	rewinddir(svc->dir);
	while ((ent = readdir(svc->dir))) {
		unsigned long mog_devid;
		char *end;
		size_t len = strlen(ent->d_name);
		struct mog_dev *dev;
		struct mog_devlist *devlist;
		Hash_table *devhash;

		if (len <= 3) continue;
		if (memcmp("dev", ent->d_name, 3) != 0) continue;

		mog_devid = strtoul(ent->d_name + 3, &end, 10);
		if (*end != 0) continue;
		if (mog_devid > 0xffffff) continue; /* MEDIUMINT in DB */

		dev = mog_dev_new(svc, (uint32_t)mog_devid);
		if (!dev) continue;

		devlist = svc_devlist(svc, dev->st_dev);
		devhash = devlist->by_mogdevid;

		if (cb) rc |= cb(dev, svc); /* mog_dev_mkusage */
		switch (hash_insert_if_absent(devhash, dev, NULL)) {
		case 0:
			free(dev);
			break;
		case 1:
			(*nr)++;
			break;
		default: mog_oom(); /* -1 */
		}
	}
	CHECK(int, 0, pthread_mutex_unlock(&svc->devstats_lock));

	return rc;
}

static bool write_dev_stats(void *entry, void *filep)
{
	struct mog_dev *dev = entry;
	FILE **fp = filep;
	char util[MOG_IOUTIL_LEN];

	mog_iou_read(dev->st_dev, util);

	if (fprintf(*fp, "%u\t%s\n", dev->devid, util) > 0)
		return true;

	/* stop iteration in case we get EIO/ENOSPC on systems w/o memstream */
	my_memstream_errclose(*fp);
	*fp = NULL;
	return false;
}

static bool write_devlist_stats(void *entry, void *filep)
{
	struct mog_devlist *devlist = entry;
	FILE **fp ;

	hash_do_for_each(devlist->by_mogdevid, write_dev_stats, filep);
	fp = filep;

	/* *filep becomes NULL on errors */
	return !!*fp;
}

/* updates per-svc device stats from the global mount list */
static ssize_t devstats_stringify(struct mog_svc *svc, char **dst)
{
	FILE *fp;
	size_t bytes;

	assert(svc->by_st_dev && "need to scan devices first");

	/* open_memstream() may fail on EIO/EMFILE/ENFILE on fake memstream */
	fp = open_memstream(dst, &bytes);
	if (!fp)
		return -1;

	/*
	 * write_devlist_stats->write_dev_stats may fclose and NULL fp
	 * to indicate error:
	 */
	hash_do_for_each(svc->by_st_dev, write_devlist_stats, &fp);
	if (!fp)
		return -1;

	if (fprintf(fp, ".\n") == 2) {
		CHECK(int, 0, my_memstream_close(fp, dst, &bytes));
		return bytes;
	}

	my_memstream_errclose(fp);
	return -1;
}

void mog_svc_devstats_subscribe(struct mog_mgmt *mgmt)
{
	struct mog_svc *svc = mgmt->svc;

	CHECK(int, 0, pthread_mutex_lock(&svc->devstats_lock));
	LIST_INSERT_HEAD(&svc->devstats_subscribers, mgmt, subscribed);
	CHECK(int, 0, pthread_mutex_unlock(&svc->devstats_lock));
}

/* called while iterating through all mog_svc objects */
bool mog_svc_devstats_broadcast(void *ent, void *ignored)
{
	struct mog_svc *svc = ent;
	struct mog_mgmt *mgmt, *tmp;
	struct iovec iov;
	char *buf = NULL;
	ssize_t len;
	struct mog_fd *mfd;

	CHECK(int, 0, pthread_mutex_lock(&svc->devstats_lock));

	len = devstats_stringify(svc, &buf);
	if (len < 0)
		goto out;

	LIST_FOREACH_SAFE(mgmt, &svc->devstats_subscribers, subscribed, tmp) {
		assert(mgmt->wbuf == NULL && "wbuf not null");
		iov.iov_base = buf;
		iov.iov_len = (size_t)len;
		mog_mgmt_writev(mgmt, &iov, 1);

		if (mgmt->wbuf == NULL) continue; /* success */

		LIST_REMOVE(mgmt, subscribed);
		mfd = mog_fd_of(mgmt);
		if (mgmt->wbuf == MOG_WR_ERROR) {
			assert(mgmt->rbuf == NULL && "would leak rbuf");
			mog_fd_put(mfd);
		} else { /* blocked on write */
			mog_idleq_push(mgmt->svc->queue, mfd, MOG_QEV_WR);
		}
	}
out:
	free(buf);

	CHECK(int, 0, pthread_mutex_unlock(&svc->devstats_lock));

	return true;
}

static bool devstats_shutdown_i(void *svcptr, void *ignored)
{
	struct mog_svc *svc = svcptr;
	struct mog_mgmt *mgmt, *tmp;
	struct mog_fd *mfd;

	CHECK(int, 0, pthread_mutex_lock(&svc->devstats_lock));
	LIST_FOREACH_SAFE(mgmt, &svc->devstats_subscribers, subscribed, tmp) {
		assert(mgmt->wbuf == NULL && "wbuf not null");
		assert(mgmt->rbuf == NULL && "would leak rbuf");
		LIST_REMOVE(mgmt, subscribed);
		mfd = mog_fd_of(mgmt);
		mog_fd_put(mfd);
	}
	CHECK(int, 0, pthread_mutex_unlock(&svc->devstats_lock));

	return true;
}

void mog_svc_dev_shutdown(void)
{
	mog_svc_each(devstats_shutdown_i, NULL);
}

static bool svc_mkusage_each(void *svc, void *nr)
{
	svc_scandev((struct mog_svc *)svc, nr, mog_dev_mkusage);

	return true;
}

size_t mog_mkusage_all(void)
{
	size_t nr = 0;

	mog_svc_each(svc_mkusage_each, &nr);

	return nr;
}
