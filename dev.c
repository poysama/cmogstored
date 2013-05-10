/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

struct mog_dev * mog_dev_new(struct mog_svc *svc, uint32_t mog_devid)
{
	struct mog_dev *dev;
	struct stat sb;
	char *devprefix = xasprintf("/dev%u/", mog_devid);
	size_t len = strlen(devprefix);

	if (mog_stat(svc, devprefix, &sb) < 0) {
		PRESERVE_ERRNO( free(devprefix) );
		return NULL;
	}

	dev = xmalloc(sizeof(struct mog_dev) + len);

	assert(devprefix[len - 1] == '/' && "not trailing slash");
	devprefix[len - 1] = '\0';
	memcpy(dev->prefix, devprefix, len);
	free(devprefix);

	dev->devid = mog_devid;
	dev->st_dev = sb.st_dev;

	return dev;
}

static int
emit_usage(
const struct mog_dev *dev, struct mog_svc *svc, int fd, struct statvfs *v)
{
	int rc = -1;
	unsigned long long available = v->f_bavail;
	unsigned long long total = v->f_blocks - (v->f_bfree - v->f_bavail);
	unsigned long long used = v->f_blocks - v->f_bfree;
	unsigned use = (used * 100) / total + !!((used * 100) % total);
	long long now = (long long)time(NULL);
	long double mb = v->f_frsize / (long double)1024.0;
	const struct mount_entry *me;

	available *= mb;
	total *= mb;
	used *= mb;

	if (use > 100)
		use = 100;

	me = mog_mnt_acquire(dev->st_dev);
	if (me) {
		static const char usage_fmt[] =
			"available: %llu\n"
			"device: %s\n"
			"disk: %s%s\n"
			"time: %lld\n"
			"total: %llu\n"
			"use: %u%%\n"
			"used: %llu\n";

		errno = 0;
		rc = dprintf(fd, usage_fmt,
			     available, me->me_devname, svc->docroot,
			     dev->prefix, now, total, use, used);

		PRESERVE_ERRNO( mog_mnt_release(me) );
		if (rc < 0 || errno == ENOSPC) {
			PRESERVE_ERRNO(do {
				syslog(LOG_ERR, "dprintf(%s%s/usage): %m",
				       svc->docroot, dev->prefix);
			} while (0));
		}
	} else {
		syslog(LOG_ERR, "mount entry not found for %s%s",
		       svc->docroot, dev->prefix);
		errno = ENODEV;
	}

	return rc;
}

int mog_dev_mkusage(const struct mog_dev *dev, struct mog_svc *svc)
{
	struct statvfs v;
	char *usage_path = xasprintf("%s/usage", dev->prefix);
	char *tmp_path = xasprintf("%s.%x", usage_path, (unsigned)getpid());
	int fd = -1;

	if (mog_unlink(svc, tmp_path) < 0 && errno != ENOENT) goto out;

	errno = 0;
	fd = mog_open_put(svc, tmp_path, O_EXCL|O_CREAT);
	if (fd < 0) {
		if (mog_open_expire_retry(svc))
			fd = mog_open_put(svc, tmp_path, O_EXCL|O_CREAT);
	}
	if (fd < 0) {
		PRESERVE_ERRNO(do {
			syslog(LOG_ERR, "open(%s%s): %m",
			       svc->docroot, tmp_path);
		} while (0));
		goto out;
	}
	if (fstatvfs(fd, &v) < 0) {
		PRESERVE_ERRNO(do {
			syslog(LOG_ERR, "fstatvfs(%s%s): %m",
			       svc->docroot, tmp_path);
		} while (0));
		goto out;
	}
	if (emit_usage(dev, svc, fd, &v) < 0) goto out;
	if (fchmod(fd, svc->put_perms) < 0) {
		PRESERVE_ERRNO(do {
			syslog(LOG_ERR, "fchmod(%s%s): %m",
			       svc->docroot, tmp_path);
		} while (0));
		goto out;
	}

	/* skip rename on EIO if close() fails */
	if (close(fd) != 0) {
		assert(errno != EBADF && "attempted to close bad FD");
		fd = -1;
		if (errno != EINTR) {
			/* possible EIO */
			PRESERVE_ERRNO(do {
				syslog(LOG_ERR, "close(%s%s) failed: %m",
			               svc->docroot, tmp_path);
			} while (0));
			goto out;
		}
	}

	fd = -1;
	errno = 0;
	if (mog_rename(svc, tmp_path, usage_path) != 0) {
		PRESERVE_ERRNO(do {
			syslog(LOG_ERR, "rename(%s(%s => %s))",
			       svc->docroot, tmp_path, usage_path);
		} while (0));
	}
out:
	PRESERVE_ERRNO(do {
		if (errno)
			(void)mog_unlink(svc, tmp_path);
		if (fd >= 0)
			(void)mog_close(fd);
		free(tmp_path);
		free(usage_path);
	} while (0));
	return errno ? -1 : 0;
}
