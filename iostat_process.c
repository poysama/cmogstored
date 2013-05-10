/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
/*
 * process management for iostat(1)
 * Since iostat(1) watches the entire system, we only spawn it once
 * regardless of the number of mog_svc objects we have.
 */
#include "cmogstored.h"

static pid_t iostat_pid;
static time_t iostat_last_fail;
static struct mog_iostat *iostat;
static time_t iostat_fail_timeout = 10;

static void iostat_atexit(void)
{
	if (iostat_pid > 0)
		kill(iostat_pid, SIGTERM);
}

static int iostat_pipe_init(int *fds)
{
	if (pipe2(fds, O_CLOEXEC) < 0) {
		PRESERVE_ERRNO( syslog(LOG_ERR, "pipe2() failed: %m") );

		/*
		 * don't retry here, MFS can deal with not getting iostat
		 * data for a while
		 */
		if (errno == ENFILE || errno == EMFILE)
			PRESERVE_ERRNO( (void)mog_fdmap_expire(5) );
		return -1;
	}

	CHECK(int, 0, mog_set_nonblocking(fds[0], true));
	/* fds[1] (write end) stays _blocking_ */

	return 0;
}

/* only called in the child process */
static const char * exec_cmd(const char *cmd)
{
	time_t last_fail = time(NULL) - iostat_last_fail;
	time_t delay = iostat_fail_timeout - last_fail;

	if (delay <= 0)
		return xasprintf("exec %s", cmd);

	syslog(LOG_DEBUG,
	       "delaying exec of `%s' for %ds due to previous failure",
	       cmd, (int)delay);
	return xasprintf("sleep %d; exec %s", (int)delay, cmd);
}

static void dup2_or_die(int oldfd, int newfd, const char *errdesc)
{
	int rc;

	do
		rc = dup2(oldfd, newfd);
	while (rc < 0 && (errno == EINTR || errno == EBUSY));

	if (rc < 0) {
		syslog(LOG_CRIT, "dup2(%s) failed: %m", errdesc);
		abort();
	}
}

static void preexec_redirect(int out_fd)
{
	int null_fd;

	dup2_or_die(out_fd, STDOUT_FILENO, "iostat_pipe[1],STDOUT");
	mog_close(out_fd);

	null_fd = open("/dev/null", O_RDONLY);
	if (null_fd < 0) {
		syslog(LOG_CRIT, "open(/dev/null) failed: %m");
		abort();
	}
	dup2_or_die(null_fd, STDIN_FILENO, "/dev/null,STDIN");
	mog_close(null_fd);

	/* don't touch stderr */
}

static pid_t iostat_fork_exec(int out_fd)
{
	/* rely on /bin/sh to parse iostat command-line args */
	const char *cmd = getenv("MOG_IOSTAT_CMD");
	if (!cmd)
		cmd = "iostat -dx 1 30";

	cmd = exec_cmd(cmd);

	iostat_pid = fork();
	if (iostat_pid < 0) {
		syslog(LOG_ERR, "fork() for iostat failed: %m");
	} else if (iostat_pid > 0) {
		mog_process_register(iostat_pid, MOG_PROC_IOSTAT);
		mog_close(out_fd);
	} else {
		/* child */
		preexec_redirect(out_fd);
		if (! mog_cloexec_atomic)
			mog_cloexec_from(STDERR_FILENO + 1);

		mog_intr_enable();
		execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
		syslog(LOG_CRIT, "execl(%s) failed: %m", cmd);
		abort();
	}
	mog_free(cmd);
	return iostat_pid;
}

bool mog_iostat_respawn(int oldstatus)
{
	int fds[2];
	struct mog_fd *mfd;

	if (WIFEXITED(oldstatus) && WEXITSTATUS(oldstatus) == 0) {
		/* syslog(LOG_DEBUG, "iostat done, restarting"); */
	} else {
		iostat_last_fail = time(NULL);
		syslog(LOG_WARNING,
		       "iostat done (pid=%d, status=%d), will retry in %ds",
		       (int)iostat_pid, oldstatus, (int)iostat_fail_timeout);
	}
	iostat_pid = 0;

	if (iostat_pipe_init(fds) < 0)
		return false; /* EMFILE || ENFILE */
	if (iostat_fork_exec(fds[1]) < 0)
		return false; /* fork() failure */

	assert(fds[0] >= 0 && "invalid FD");

	mfd = mog_fd_init(fds[0], MOG_FD_TYPE_IOSTAT);

	if (iostat == NULL)
		atexit(iostat_atexit);
	iostat = &mfd->as.iostat;
	iostat->queue = mog_notify_queue;
	mog_iostat_init(iostat);
	mog_idleq_add(iostat->queue, mfd, MOG_QEV_RD);

	return true;
}
