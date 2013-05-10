/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */

#define _GNU_SOURCE 1 /* needed for _ATFILE_SOURCE on glibc 2.5 - 2.9 */
#include <dirent.h> /* needed for FreeBSD */
#include "cmogstored.h"

/* same default as MogileFS upstream */
static pthread_mutex_t svc_lock = PTHREAD_MUTEX_INITIALIZER;
static Hash_table *by_docroot; /* enforce one mog_svc per docroot: */
static mode_t mog_umask;

static void svc_free(void *ptr)
{
	struct mog_svc *svc = ptr;

	if (closedir(svc->dir) < 0)
		syslog(LOG_ERR, "closedir(%s) failed with: %m", svc->docroot);
	CHECK(int, 0, pthread_mutex_destroy(&svc->devstats_lock));
	mog_free(svc->docroot);
	if (svc->by_st_dev)
		hash_free(svc->by_st_dev);
	free(svc);
}

static size_t svc_hash(const void *x, size_t tablesize)
{
	const struct mog_svc *svc = x;

	return hash_string(svc->docroot, tablesize);
}

static bool svc_cmp(const void *a, const void *b)
{
	const struct mog_svc *svc_a = a;
	const struct mog_svc *svc_b = b;

	return strcmp(svc_a->docroot, svc_b->docroot) == 0;
}

static void svc_atexit(void) /* called atexit */
{
	hash_free(by_docroot);
}

static void svc_once(void)
{
	by_docroot = hash_initialize(7, NULL, svc_hash, svc_cmp, svc_free);
	if (!by_docroot)
		mog_oom();

	mog_umask = umask(0);
	umask(mog_umask);
	atexit(svc_atexit);
}

struct mog_svc * mog_svc_new(const char *docroot)
{
	struct mog_svc *svc;
	DIR *dir;
	int fd;

	if (!docroot) docroot = MOG_DEFAULT_DOCROOT;

	docroot = mog_canonpath_die(docroot, CAN_EXISTING);

	dir = opendir(docroot);
	if (dir == NULL) {
		syslog(LOG_ERR, "opendir(%s) failed with: %m", docroot);
		mog_free(docroot);
		return NULL;
	}

	fd = dirfd(dir);
	if (fd < 0) {
		syslog(LOG_ERR, "dirfd(%s) failed with: %m", docroot);
		mog_free(docroot);
		return NULL;
	}

	CHECK(int, 0, pthread_mutex_lock(&svc_lock));

	if (!by_docroot)
		svc_once();

	svc = xzalloc(sizeof(struct mog_svc));
	svc->http_fd = svc->httpget_fd = svc->mgmt_fd = -1;
	svc->docroot = docroot;
	svc->docroot_fd = fd;
	svc->dir = dir;
	svc->put_perms = (~mog_umask) & 0666;
	svc->mkcol_perms = (~mog_umask) & 0777;
	svc->idle_timeout = 5;
	CHECK(int, 0, pthread_mutex_init(&svc->devstats_lock, NULL));

	switch (hash_insert_if_absent(by_docroot, svc, NULL)) {
	case 0:
		svc_free(svc);
		svc = NULL;
	case 1: break;
	default: mog_oom();
	}

	CHECK(int, 0, pthread_mutex_unlock(&svc_lock));

	return svc;
}

size_t mog_svc_each(Hash_processor processor, void *data)
{
	size_t rv;

	CHECK(int, 0, pthread_mutex_lock(&svc_lock));
	rv = hash_do_for_each(by_docroot, processor, data);
	CHECK(int, 0, pthread_mutex_unlock(&svc_lock));

	return rv;
}

static bool cloexec_disable(int fd)
{
	if (fd >= 0)
		CHECK(int, 0, mog_set_cloexec(fd, false));
	return true;
}

static bool svc_cloexec_off_i(void *svcptr, void *unused)
{
	struct mog_svc *svc = svcptr;

	return (cloexec_disable(svc->mgmt_fd)
	        && cloexec_disable(svc->http_fd)
	        && cloexec_disable(svc->httpget_fd));
}

/*
 * Only call this from a freshly forked upgrade child process.
 * This holds no locks to avoid potential deadlocks in post-fork mutexes
 */
void mog_svc_upgrade_prepare(void)
{
	(void)hash_do_for_each(by_docroot, svc_cloexec_off_i, NULL);
}
