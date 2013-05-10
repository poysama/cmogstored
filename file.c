/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "digest.h"

bool mog_open_expire_retry(struct mog_svc *svc)
{
	switch (errno) {
	case ENFILE:
	case EMFILE:
	case ENOMEM:
		if (mog_fdmap_expire(svc->idle_timeout) > 0)
			return true;
	}
	return false;
}

/* path must be a free()-able pointer */
struct mog_fd *
mog_file_open_read(struct mog_svc *svc, char *path)
{
	struct mog_fd *mfd;
	struct mog_file *mfile;
	int fd = mog_open_read(svc, path);

	if (fd < 0 && mog_open_expire_retry(svc))
		fd = mog_open_read(svc, path);

	if (fd < 0) return NULL;

	mfd = mog_fd_init(fd, MOG_FD_TYPE_FILE);

	mfile = &mfd->as.file;
	memset(mfile, 0, sizeof(struct mog_file));
	mfile->fsize = -1;
	mfile->svc = svc;

	return mfd;
}

static int mkpath_open_put(struct mog_svc *svc, char *path, int flags)
{
	int fd = mog_open_put(svc, path, flags);

	if (fd < 0 && errno == ENOENT) {
		/* directory does not exist, create it and retry open */
		if (mog_mkpath_for(svc, path) == 0)
			return mog_open_put(svc, path, flags);

		syslog(LOG_ERR, "failed to create directory for path=%s (%m)",
		       path);
		errno = ENOENT; /* restore open() errno */
	}

	return fd;
}

/* path must be a free()-able pointer */
struct mog_fd *
mog_file_open_put(struct mog_svc *svc, char *path, int flags)
{
	struct mog_fd *mfd;
	struct mog_file *mfile;
	int fd = mkpath_open_put(svc, path, flags);

	if (fd < 0 && mog_open_expire_retry(svc))
		fd = mkpath_open_put(svc, path, flags);

	if (fd < 0) return NULL;

	mfd = mog_fd_init(fd, MOG_FD_TYPE_FILE);

	mfile = &mfd->as.file;
	memset(mfile, 0, sizeof(struct mog_file));
	mfile->svc = svc;

	return mfd;
}

void mog_file_close(struct mog_fd *mfd)
{
	struct mog_file *mfile = &mfd->as.file;

	assert(mfd->fd_type == MOG_FD_TYPE_FILE && "mog_fd is not a file");

	/* all of these may already be NULL */
	free(mfile->path);
	free(mfile->tmppath);
	mog_digest_destroy(&mfile->digest);

	mog_fd_put(mfd);
}
