/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 *
 * All path operations we support are relative to mog_svc.docroot.
 * Absolute path lookups are sometimes more expensive, especially
 * for deeper directory structures, so we'll favor AT_FILE functions.
 *
 * However, it's somewhat common for systems to have AT_FILE functions
 * to be available on the system they're built on, but not available
 * on the system the code is deployed on.  We'll provide fallbacks to
 * the absolute path variants for everything.
 */
#include "cmogstored.h"
#define MY_PATHMAX 256
#ifndef O_NOATIME
# define O_NOATIME 0
#endif

static int noatime_flags = O_RDONLY | O_NOATIME;
static int put_flags = O_RDWR;

/*
 * Like AT_FILE functions, we may be built on systems with O_CLOEXEC and
 * run on systems without it.  O_CLOEXEC is still too recent and was
 * broken on some Linux kernels, so we always run cloexec_detect.c at
 * startup.
 */
#ifdef O_CLOEXEC
void mog_cloexec_works(void)
{
	noatime_flags |= O_CLOEXEC;
	put_flags |= O_CLOEXEC;
}
#endif /* O_CLOEXEC */

/* we only use real *at syscalls, Gnulib workalikes aren't thread-safe */

#define GET_FSPATH(DST,SRC) do { \
	int rc = snprintf((DST), sizeof(DST), "%s%s", svc->docroot, (SRC)); \
	if (rc <= 0 || rc >= sizeof(DST)) { \
		errno = ENAMETOOLONG; \
		return -1; \
	} \
} while (0)

#ifndef HAVE_FSTATAT
int mog_stat(struct mog_svc *svc, const char *path, struct stat *sb)
{
	char fspath[MY_PATHMAX];

	GET_FSPATH(fspath, path);
	return stat(fspath, sb);
}
#endif /* !HAVE_FSTATAT */

#ifdef HAVE_OPENAT
static int open_read(struct mog_svc *svc, const char *path)
{
	return openat(svc->docroot_fd, path + 1, noatime_flags);
}

int mog_open_put(struct mog_svc *svc, const char *path, int flags)
{
	return openat(svc->docroot_fd, path + 1, flags | put_flags, 0600);
}
#else /* !HAVE_OPENAT */
MOG_NOINLINE static int open_read(struct mog_svc *svc, const char *path)
{
	char fspath[MY_PATHMAX];

	GET_FSPATH(fspath, path);
	return open(fspath, noatime_flags);
}

int mog_open_put(struct mog_svc *svc, const char *path, int flags)
{
	char fspath[MY_PATHMAX];

	GET_FSPATH(fspath, path);
	return open(fspath, flags | put_flags, 0600);
}
#endif /* !HAVE_OPENAT */

int mog_open_read(struct mog_svc *svc, const char *path)
{
	int fd;

retry:
	fd = open_read(svc, path);
	if (fd < 0 && errno != ENOENT && (noatime_flags & O_NOATIME)) {
		noatime_flags = O_RDONLY;
		goto retry;
	}
	return fd;
}

#ifndef HAVE_UNLINKAT
int mog_unlink(struct mog_svc *svc, const char *path)
{
	char fspath[MY_PATHMAX];

	GET_FSPATH(fspath,path);
	return unlink(fspath);
}
#endif /* !HAVE_UNLINKAT */

#ifndef HAVE_RENAMEAT
int mog_rename(struct mog_svc *svc, const char *old, const char *new)
{
	char fsnew[MY_PATHMAX];
	char fsold[MY_PATHMAX];

	GET_FSPATH(fsold, old);
	GET_FSPATH(fsnew, new);
	return rename(fsold, fsnew);
}
#endif /* !HAVE_RENAMEAT */

#ifndef HAVE_MKDIRAT
int mog_mkdir(struct mog_svc *svc, const char *path, mode_t mode)
{
	char fspath[MY_PATHMAX];

	GET_FSPATH(fspath, path);
	return mkdir(fspath, mode);
}
#endif /* !HAVE_MKDIRAT */
