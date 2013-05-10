/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "compat_memstream.h"

static struct {
	char **argv;
	char **envp;
} start;

#define FD_PFX "CMOGSTORED_FD="

MOG_NOINLINE static void free_list(char **head)
{
	char **tmp = head;

	if (tmp) {
		for (; *tmp; tmp++)
			free(*tmp);
		free(head);
	}
}

/* only needed to make valgrind happy */
__attribute__((destructor)) static void upgrade_atexit(void)
{
	free_list(start.argv);
	free_list(start.envp);
}

void mog_upgrade_prepare(int argc, char *argv[], char *envp[])
{
	int i;
	size_t env_count = 2; /* extra for NULL-termination and CMOGSTORED_FD */
	char **e;

	/* duplicate argv */
	start.argv = xmalloc(sizeof(char *) * (argc + 1));
	for (i = 0; i < argc; i++)
		start.argv[i] = xstrdup(argv[i]);
	start.argv[argc] = NULL;

	/* allocate slots for envp */
	for (e = envp; *e; e++)
		env_count++;
	start.envp = xmalloc(sizeof(char *) * env_count);

	/* duplicate envp */
	e = start.envp;
	*e++ = xstrdup(FD_PFX); /* placeholder */
	for (; *envp; envp++) {
		if (strncmp(*envp, FD_PFX, strlen(FD_PFX)))
			*e++ = xstrdup(*envp);
	}
	*e = NULL;
}

/* writes one comma-delimited fd to fp */
static bool emit_fd(FILE *fp, int fd)
{
	int r;

	/* no error, just the FD isn't used */
	if (fd < 0)
		return true;

	errno = 0;
	r = fprintf(fp, "%d,", fd);
	if (r > 0)
		return true;
	if (errno == 0)
		errno = ENOSPC;
	syslog(LOG_ERR, "fprintf() failed: %m");
	return false;
}

static bool svc_emit_fd_i(void *svcptr, void *_fp)
{
	FILE *fp = _fp;
	struct mog_svc *svc = svcptr;

	return (emit_fd(fp, svc->mgmt_fd)
	        && emit_fd(fp, svc->http_fd)
	        && emit_fd(fp, svc->httpget_fd));
}

/* returns the PID of the newly spawned child */
pid_t mog_upgrade_spawn(void)
{
	pid_t pid = -1;
	FILE *fp;
	size_t bytes;
	char *dst = NULL;
	int rc;
	const char *execfile;

	if (!mog_pidfile_upgrade_prepare())
		return pid;

	fp = open_memstream(&dst, &bytes);
	if (fp == NULL) {
		syslog(LOG_ERR, "open_memstream failed for upgrade: %m");
		return pid;
	}

	execfile = find_in_path(start.argv[0]);
	errno = 0;
	rc = fputs(FD_PFX, fp);
	if (rc < 0 || rc == EOF) {
		if (errno == 0)
			errno = ferror(fp);
		PRESERVE_ERRNO( (void)fclose(fp) );
		syslog(LOG_ERR, "fputs returned %d on memstream: %m", rc);
		goto out;
	}

	mog_svc_each(svc_emit_fd_i, fp);
	errno = 0;
	if ((my_memstream_close(fp, &dst, &bytes) != 0) && (errno != EINTR)) {
		syslog(LOG_ERR, "fclose on memstream failed for upgrade: %m");
		goto out;
	}

	assert(dst[bytes - 1] == ',' && "not comma-terminated no listeners?");
	dst[bytes - 1] = '\0'; /* kill the last comma */

	pid = fork();
	if (pid == 0) {
		start.envp[0] = dst;
		mog_svc_upgrade_prepare();
		execve(execfile, start.argv, start.envp);
		die_errno("execve %s", execfile);
	} else if (pid > 0) {
		mog_process_register(pid, MOG_PROC_UPGRADE);
		syslog(LOG_INFO, "upgrade spawned PID:%d", pid);
	} else {
		syslog(LOG_ERR, "fork failed for upgrade: %m");
	}

out:
	/* find_in_path does not malloc if output == input */
	if (execfile != start.argv[0])
		mog_free(execfile);
	free(dst);

	return pid;
}
