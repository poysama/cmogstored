/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

/*
 * statvfs() on GNU/Linux may call stat() internally, and stat() is bad
 * for network mounts which may be stalled/slow, so favor the non-portable
 * statfs() on GNU/Linux
 */
#ifdef __linux__
#include <sys/vfs.h>
#define MY_STATFS statfs
#else /* statvfs() is POSIX */
#define MY_STATFS statvfs
#endif

static bool resolve_symlink(char **orig)
{
	char *p = canonicalize_filename_mode(*orig, CAN_EXISTING);

	if (p) {
		free(*orig);
		*orig = p;
		return true;
	}
	return false;
}

static bool stat_harder(struct mount_entry *me)
{
	struct stat sb;

	/* the device number may not have been populated, do it */
	if (me->me_dev == (dev_t)-1) {
		if (stat(me->me_mountdir, &sb) != 0)
			return false;
		me->me_dev = sb.st_dev;
	}

	/*
	 * resolve symlinks for things that look like paths
	 * and skip dead symlinks
	 */
	if (me->me_devname[0] == '/') {
		if (lstat(me->me_devname, &sb) == 0
		    && S_ISLNK(sb.st_mode)
		    && ! resolve_symlink(&me->me_devname))
			return false;
	}
	return true;
}

/*
 * prevents us from using filesystems of unknown size, since those could
 * be stalled/dead network mounts
 */
bool mog_mnt_usable(struct mount_entry *me)
{
	struct MY_STATFS buf;
	const char *path = me->me_mountdir;

	if (me->me_dummy)
		return false;

retry:
	errno = 0;
	if (MY_STATFS(path, &buf) == 0)
		return (buf.f_blocks > 0) ? stat_harder(me) : false;

	/* unknown */
	assert(errno != EFAULT && "BUG: EFAULT from statfs/statvfs");
	switch (errno) {
	case EINTR: goto retry;
	case EIO: /* this is important enough to log: */
	case EOVERFLOW: /* this is important enough to log: */
		syslog(LOG_ERR, MOG_STR(MY_STATFS) "(%s) failed: %m", path);
	case ENOTDIR: /* race between read_file_system_list and statvfs */
	case ELOOP: /* race between read_file_system_list and statvfs? */
	case ENOSYS: /* if statvfs() doesn't work, fstatvfs() won't, either */
	case EACCES: /* this is common */
		return false;
	}
	/*
	 * assume other errors are recoverable (or fail elsewhere)
	 * ENAMETOOLONG - would fail anyways if we need to stat(2)
	 * ENOMEM - maybe the kernel will get more memory back, soon
	 */
	return true;
}
