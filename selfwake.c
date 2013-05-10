/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

#if MOG_SELFPIPE
struct mog_fd * mog_selfwake_new(void)
{
	struct mog_fd *reader, *writer;
	struct mog_selfwake *selfwake;
	int self_pipe[2];

	if (pipe2(self_pipe, O_NONBLOCK | O_CLOEXEC) < 0)
		die_errno("failed to init self-pipe");

	reader = mog_fd_init(self_pipe[0], MOG_FD_TYPE_SELFWAKE);
	selfwake = &reader->as.selfwake;

	writer = mog_fd_init(self_pipe[1], MOG_FD_TYPE_SELFPIPE);
	writer->as.selfpipe.reader = reader;
	selfwake->writer = writer;

	return reader;
}

static ssize_t selfwake_drain(struct mog_fd *mfd)
{
	char buf[64];
	return read(mfd->fd, buf, sizeof(buf));
}

/* this allows interrupts */
void mog_selfwake_wait(struct mog_fd *mfd)
{
	/* poll() on a pipe does not work on some *BSDs, so just block */
	mog_set_nonblocking(mfd->fd, false);
	mog_intr_enable();
	(void)selfwake_drain(mfd);
	mog_intr_disable();
	mog_set_nonblocking(mfd->fd, true);
	mog_selfwake_drain(mfd);
}

/* this is async-signal safe (except in the case of bugs) */
void mog_selfwake_trigger(struct mog_fd *mfd)
{
	ssize_t w;

retry:
	w = write(mfd->as.selfwake.writer->fd, "", 1);
	if (w >= 0) return;

	switch (errno) {
	case_EAGAIN: return; /* somebody already woke this up */
	case EINTR: goto retry; /* just in case... */
	}

	/*
	 * we're screwed anyways, at least try this even though syslog()
	 * isn't safe inside a signal handler.
	 */
	syslog(LOG_CRIT, "mog_selfwake_trigger write() failed: %m");
	abort();
}

void mog_selfwake_drain(struct mog_fd *mfd)
{
	ssize_t r;

	do {
		r = selfwake_drain(mfd);
	} while (r > 0);
	assert(r < 0 && (errno == EAGAIN || errno == EINTR)
	       && "selfwake read failed");
}
#endif /* MOG_SELFPIPE */
