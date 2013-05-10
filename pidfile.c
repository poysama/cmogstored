/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
static const char *pidfile;
static bool pidfile_exists;
static const char *old;
static pid_t owner;
#ifndef O_CLOEXEC
#define O_CLOEXEC (0)
#endif

static bool pid_is_running(pid_t pid)
{
	if (pid <= 0)
		return false;
	if (kill(pid, 0) < 0 && errno == ESRCH)
		return false;
	return true;
}

/* sets errno on failure */
static bool pid_write(int fd)
{
	errno = 0;
	return !(dprintf(fd, "%d\n", (int)getpid()) <= 1 || errno == ENOSPC);
}

/* returns 0 if pidfile is empty, -1 on error, pid value on success */
static pid_t pidfile_read(int fd)
{
	pid_t pid = -1;
	char buf[sizeof(pid_t) * 8 / 3 + 1];
	ssize_t r;
	char *end;
	long tmp;

	errno = 0;
	r = pread(fd, buf, sizeof(buf), 0);
	if (r == 0)
		pid = 0; /* empty file */
	if (r > 0) {
		errno = 0;
		tmp = strtol(buf, &end, 10);

		if (*end == '\n' && tmp > 0 && tmp < LONG_MAX)
			pid = (pid_t)tmp;
	}

	return pid;
}

static void pidfile_cleanup(void)
{
	if (pidfile) {
		if (getpid() == owner) {
			if (old)
				unlink(old);
			else if (pidfile_exists)
				unlink(pidfile);
		}
		/* else: don't unlink if it does not belong to us */
		mog_free_and_null(&pidfile);
		mog_free_and_null(&old);
	}
}

/*
 * opens a pid file and returns a file descriptor for it
 * mog_pidfile_commit() should be used on the fd returned by
 * this function (often in a separate process)
 * returns < 0 if there is an error and sets errno=EAGAIN
 * if a pid already exists
 *
 *
 * Example: (error checking is left as an exercise to the reader)
 *
 *	pid_t cur_pid;
 *	int fd = mog_pidfile_open("/path/to/pid", &cur_pid);
 *	daemon(0, 0);
 *	mog_pidfile_commit(fd);
 */
static int mog_pidfile_open(const char *path, pid_t *cur)
{
	int fd = open(path, O_RDWR|O_CREAT, 0666);
	pid_t pid;

	*cur = -1;
	if (fd < 0)
		return fd;

	/* see if existing pidfile is valid */
	pid = pidfile_read(fd);
	if (pid == 0) {
		/*
		 * existing pidfile is empty, FS could've been full earlier,
		 * proceed assuming we can overwrite
		 */
	} else if (pid > 0) {
		/* can't signal it, (likely) safe to overwrite */
		if (!pid_is_running(pid))
			goto out;

		/* old pidfile is still valid */
		errno = EAGAIN;
		*cur = pid;
		goto err;
	}

out:
	assert(pidfile == NULL && "already opened pidfile for process");
	pidfile = canonicalize_filename_mode(path, CAN_EXISTING);
	if (!pidfile)
		goto err;

	pidfile_exists = true;
	return fd;
err:
	PRESERVE_ERRNO( close(fd) );
	return -1;
}

/*
 * commits the pidfile pointed to by the given fd
 * and closes the given fd on success.
 * returns -1 on error and sets errno
 * fd should be the return value of mog_pidfile_open();
 */
int mog_pidfile_commit(int fd)
{
	assert(lseek(fd, 0, SEEK_CUR) == 0 && "pidfile offset != 0");
	assert(pidfile && "mog_pidfile_open not called (or unsuccessful)");

	errno = 0;
	if (!pid_write(fd)) {
		PRESERVE_ERRNO( close(fd) );
		if (errno == ENOSPC)
			PRESERVE_ERRNO( pidfile_cleanup() );
		return -1;
	}
	if (close(fd) < 0 && errno != EINTR)
		return -1;

	owner = getpid();
	atexit(pidfile_cleanup);

	return 0;
}

int mog_pidfile_prepare(const char *path)
{
	pid_t cur_pid = -1;
	int pid_fd = mog_pidfile_open(path, &cur_pid);

	if (pid_fd >= 0)
		return pid_fd;
	if (errno == EAGAIN)
		die("already running on PID: %d", (int)cur_pid);
	else
		die_errno("mog_pidfile_prepare failed");
	return -1;
}

/* returns true if successful (or path is non-existent) */
static bool unlink_if_owner_or_unused(const char *path)
{
	pid_t pid;
	int fd = open(path, O_RDONLY|O_CLOEXEC);

	if (fd < 0) {
		/* somebody mistakenly removed path while we were running */
		if (errno == ENOENT)
			return true;
		syslog(LOG_ERR, "open(%s): %m failed", path);
		return false;
	}

	pid = pidfile_read(fd);
	PRESERVE_ERRNO( mog_close(fd) );

	if (pid == 0) {
		/*
		 * existing path is empty, FS could've been full earlier,
		 * proceed assuming we can overwrite
		 */
	} else if (pid > 0) {
		if (pid == getpid())
			goto do_unlink;
		if (!pid_is_running(pid))
			goto do_unlink;
		syslog(LOG_ERR,
		       "cannot unlink %s belongs to running PID:%d",
		       path, (int)pid);
		return false;
	} else {
		/* can't unlink pidfile safely */
		syslog(LOG_ERR, "failed to read/parse %s: %m", path);
		return false;
	}
do_unlink:
	/* ENOENT: maybe somebody else just unlinked it */
	if (unlink(path) == 0 || errno == ENOENT)
		return true;

	syslog(LOG_ERR, "failed to remove %s for upgrade: %m", path);
	return false;
}

/* replaces (non-atomically) current pidfile with pidfile.oldbin */
bool mog_pidfile_upgrade_prepare(void)
{
	pid_t pid = -1;
	int fd;

	if (!pidfile)
		return true;

	assert(owner == getpid() &&
	       "mog_pidfile_upgrade_prepare called by non-owner");

	if (!unlink_if_owner_or_unused(pidfile))
		return false;

	assert(old == NULL && "oldbin already registered");
	old = xasprintf("%s.oldbin", pidfile);
	fd = open(old, O_CREAT|O_RDWR|O_CLOEXEC, 0666);
	if (fd < 0) {
		syslog(LOG_ERR, "failed to open pidfile %s: %m", old);
		mog_free_and_null(&old);
		return false;
	}
	pid = pidfile_read(fd);
	if (pid_is_running(pid)) {
		syslog(LOG_ERR,
		       "upgrade failed, %s belongs to running PID:%d",
		       old, (int)pid);
		mog_free_and_null(&old);
	} else if (pid_write(fd)) {
		/* success writing, don't touch old */
	} else {
		syslog(LOG_ERR, "failed to write pidfile %s: %m", old);
		mog_free_and_null(&old);
	}

	PRESERVE_ERRNO( mog_close(fd) );
	return old ? true : false;
}

static bool upgrade_failed(void)
{
	pid_t pid;
	int fd = open(pidfile, O_RDONLY|O_CLOEXEC);

	/* pidfile no longer exists, good */
	if (fd < 0)
		return true;

	pid = pidfile_read(fd);
	PRESERVE_ERRNO( mog_close(fd) );

	/* save to overwrite */
	if (!pid_is_running(pid))
		return true;

	assert(old && "we are stuck on oldbin");
	syslog(LOG_ERR, "PID:%d of upgrade still running", pid);
	return false;
}

/* removes oldbin file and restores original pidfile */
void mog_pidfile_upgrade_abort(void)
{
	int fd;

	if (!pidfile)
		return;

	assert(owner == getpid() &&
	       "mog_pidfile_upgrade_abort called by non-owner");

	/* ensure the pidfile of the upgraded process is really invalid */
	if (!upgrade_failed())
		return;

	fd = open(pidfile, O_TRUNC|O_CREAT|O_WRONLY|O_CLOEXEC, 0666);
	if (fd >= 0) {
		pidfile_exists = true;
		if (!pid_write(fd))
			syslog(LOG_ERR, "failed to write %s: %m", pidfile);
		mog_close(fd);
		if (unlink_if_owner_or_unused(old))
			mog_free_and_null(&old);
	} else {
		/* we're pidless(!) */
		syslog(LOG_ERR, "failed to open %s for writing: %m", pidfile);
		pidfile_exists = false;
	}
}
