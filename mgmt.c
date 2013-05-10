/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "mgmt.h"
#include "digest.h"
#include "ioprio.h"

static void mgmt_digest_step(struct mog_fd *mfd)
{
	struct mog_mgmt *mgmt = &mfd->as.mgmt;
	struct mog_fd *fmfd = mgmt->forward;
	enum mog_digest_next next;

	/*
	 * MOG_PRIO_FSCK means we're likely the _only_ thread handling
	 * MD5, so run it as fast as possible.
	 */
	if (mgmt->prio == MOG_PRIO_FSCK) {
		int ioprio = mog_ioprio_drop();

		do {
			next = mog_digest_read(&fmfd->as.file.digest, fmfd->fd);
		} while (next == MOG_DIGEST_CONTINUE);

		if (ioprio != -1)
			mog_ioprio_restore(ioprio);
	} else {
		next = mog_digest_read(&fmfd->as.file.digest, fmfd->fd);
	}

	assert(mgmt->wbuf == NULL && "wbuf should be NULL here");

	switch (next) {
	case MOG_DIGEST_CONTINUE: return;
	case MOG_DIGEST_EOF:
		mog_mgmt_fn_digest_emit(mgmt);
		break;
	case MOG_DIGEST_ERROR:
		mog_mgmt_fn_digest_err(mgmt);
	}

	if (mgmt->prio == MOG_PRIO_FSCK)
		mog_fsck_queue_next(mfd);

	mog_file_close(mgmt->forward);
	mgmt->prio = MOG_PRIO_NONE;
	mgmt->forward = NULL;
}

static enum mog_next mgmt_digest_in_progress(struct mog_fd *mfd)
{
	struct mog_mgmt *mgmt = &mfd->as.mgmt;

	assert(mgmt->forward && mgmt->forward != MOG_IOSTAT && "bad forward");

	if (mgmt->prio == MOG_PRIO_FSCK && !mog_fsck_queue_ready(mfd))
		return MOG_NEXT_IGNORE;

	mgmt_digest_step(mfd);

	if (mgmt->wbuf == MOG_WR_ERROR) return MOG_NEXT_CLOSE;
	if (mgmt->wbuf) return MOG_NEXT_WAIT_WR;

	/*
	 * we can error on the MD5 but continue if we didn't
	 * have a socket error (from wbuf == MOG_WR_ERROR)
	 */
	return MOG_NEXT_ACTIVE;
}

MOG_NOINLINE static void mgmt_close(struct mog_fd *mfd)
{
	struct mog_mgmt *mgmt = &mfd->as.mgmt;

	mog_rbuf_free(mgmt->rbuf);
	assert((mgmt->wbuf == NULL || mgmt->wbuf == MOG_WR_ERROR) &&
	       "would leak mgmt->wbuf on close");

	mog_fd_put(mfd);
}

void mog_mgmt_writev(struct mog_mgmt *mgmt, struct iovec *iov, int iovcnt)
{
	struct mog_fd *mfd = mog_fd_of(mgmt);

	assert(mgmt->wbuf == NULL && "tried to write while busy");
	mgmt->wbuf = mog_trywritev(mfd->fd, iov, iovcnt);
}

static enum mog_next mgmt_iostat_forever(struct mog_mgmt *mgmt)
{
	mog_rbuf_free_and_null(&mgmt->rbuf); /* no coming back from this */
	mog_notify(MOG_NOTIFY_DEVICE_REFRESH);
	mog_svc_devstats_subscribe(mgmt);

	return MOG_NEXT_IGNORE;
}

/* returns true if we can continue queue step, false if not */
static enum mog_next mgmt_wbuf_in_progress(struct mog_mgmt *mgmt)
{
	assert(mgmt->wbuf != MOG_WR_ERROR && "still active after write error");
	switch (mog_tryflush(mog_fd_of(mgmt)->fd, &mgmt->wbuf)) {
	case MOG_WRSTATE_ERR: return MOG_NEXT_CLOSE;
	case MOG_WRSTATE_DONE:
		if (mgmt->forward == MOG_IOSTAT)
			return mgmt_iostat_forever(mgmt);
		return MOG_NEXT_ACTIVE;
	case MOG_WRSTATE_BUSY:
		/* unlikely, we never put anything big in wbuf */
		return MOG_NEXT_WAIT_WR;
	}
	assert(0 && "compiler bug?");
	return MOG_NEXT_CLOSE;
}

/* stash any pipelined data for the next round */
static void
mgmt_defer_rbuf(struct mog_mgmt *mgmt, struct mog_rbuf *rbuf, size_t buf_len)
{
	struct mog_rbuf *old = mgmt->rbuf;
	size_t defer_bytes = buf_len - mgmt->offset;
	char *src = rbuf->rptr + mgmt->offset;

	assert(mgmt->offset >= 0 && "mgmt->offset negative");
	assert(defer_bytes <= MOG_RBUF_BASE_SIZE && "defer bytes overflow");

	if (defer_bytes == 0) {
		mog_rbuf_free_and_null(&mgmt->rbuf);
	} else if (old) { /* no allocation needed, reuse existing */
		assert(old == rbuf && "mgmt->rbuf not reused properly");
		memmove(old->rptr, src, defer_bytes);
		old->rsize = defer_bytes;
	} else {
		mgmt->rbuf = mog_rbuf_new(MOG_RBUF_BASE_SIZE);
		memcpy(mgmt->rbuf->rptr, src, defer_bytes);
		mgmt->rbuf->rsize = defer_bytes;
	}
	mgmt->offset = 0;
}

/*
 * this is the main event callback and called whenever mgmt
 * is pulled out of a queue (either idle or active)
 */
static enum mog_next mgmt_queue_step(struct mog_fd *mfd)
{
	struct mog_mgmt *mgmt = &mfd->as.mgmt;
	struct mog_rbuf *rbuf;
	char *buf;
	ssize_t r;
	off_t off;
	size_t buf_len = 0;
	enum mog_parser_state state;

	assert(mfd->fd >= 0 && "mgmt fd is invalid");

	if (mgmt->wbuf) return mgmt_wbuf_in_progress(mgmt);
	if (mgmt->forward) return mgmt_digest_in_progress(mfd);

	/* we may have pipelined data in mgmt->rbuf */
	rbuf = mgmt->rbuf ? mgmt->rbuf : mog_rbuf_get(MOG_RBUF_BASE_SIZE);
	buf = rbuf->rptr;
	off = mgmt->offset;
	assert(off >= 0 && "offset is negative");
	assert(off < MOG_RBUF_BASE_SIZE && "offset is too big");
	if (mgmt->rbuf && off == 0) {
		/* request got "pipelined", resuming now */
		buf_len = mgmt->rbuf->rsize;
		goto parse;
	}
reread:
	r = read(mfd->fd, buf + off, MOG_RBUF_BASE_SIZE - off);
	if (r > 0) {
		buf_len = r + off;
parse:
		state = mog_mgmt_parse(mgmt, buf, buf_len);
		if (mgmt->wbuf == MOG_WR_ERROR) return MOG_NEXT_CLOSE;

		switch (state) {
		case MOG_PARSER_ERROR:
			syslog(LOG_ERR, "mgmt parser error");
			return MOG_NEXT_CLOSE;
		case MOG_PARSER_CONTINUE:
			assert(mgmt->wbuf == NULL &&
			       "tried to write (and failed) with partial req");
			if (mgmt->offset == MOG_RBUF_BASE_SIZE) {
				assert(buf_len == MOG_RBUF_BASE_SIZE &&
				       "bad rbuf");
				syslog(LOG_ERR, "mgmt request too large");
				return MOG_NEXT_CLOSE;
			}
			off = mgmt->offset;
			goto reread;
		case MOG_PARSER_DONE:
			if (mgmt->forward == MOG_IOSTAT)
				return mgmt_iostat_forever(mgmt);

			/* stash unread portion in a new buffer */
			mgmt_defer_rbuf(mgmt, rbuf, buf_len);
			mog_mgmt_reset_parser(mgmt);
			assert(mgmt->wbuf != MOG_WR_ERROR);
			return mgmt->wbuf ? MOG_NEXT_WAIT_WR : MOG_NEXT_ACTIVE;
		}
	} else if (r == 0) { /* client shut down */
		return MOG_NEXT_CLOSE;
	} else {
		switch (errno) {
		case_EAGAIN:
			if ((buf_len > 0) && (mgmt->rbuf == NULL))
				mgmt->rbuf = mog_rbuf_detach(rbuf);
			return MOG_NEXT_WAIT_RD;
		case EINTR: goto reread;
		case ECONNRESET:
		case ENOTCONN:
			return MOG_NEXT_CLOSE;
		default:
			syslog(LOG_NOTICE, "mgmt client died: %m");
			return MOG_NEXT_CLOSE;
		}
	}

	assert(0 && "compiler bug?");
	return MOG_NEXT_CLOSE;
}

/*
 * this function is called whenever a mgmt client is pulled out of
 * _any_ queue (listen/idle/active).  Our queueing model should be
 * designed to prevent this function from executing concurrently
 * for any fd.
 */
enum mog_next mog_mgmt_queue_step(struct mog_fd *mfd)
{
	enum mog_next rv = mgmt_queue_step(mfd);

	if (rv == MOG_NEXT_CLOSE)
		mgmt_close(mfd);
	return rv;
}

/* called during graceful shutdown instead of mog_mgmt_queue_step */
void mog_mgmt_quit_step(struct mog_fd *mfd)
{
	struct mog_mgmt *mgmt = &mfd->as.mgmt;
	struct mog_queue *q = mgmt->svc->queue;

	/* centralize all queue transitions here: */
	switch (mgmt_queue_step(mfd)) {
	case MOG_NEXT_WAIT_RD:
		if (mgmt->forward || mgmt->rbuf) {
			/* something is in progress, do not drop it */
			mog_idleq_push(q, mfd, MOG_QEV_RD);
			return;
		}
		/* fall-through */
	case MOG_NEXT_IGNORE: /* no new iostat watchers during shutdown */
		assert(mgmt->prio == MOG_PRIO_NONE && "bad prio");
		/* fall-through */
	case MOG_NEXT_CLOSE:
		mog_nr_active_at_quit--;
		mgmt_close(mfd);
		return;
	case MOG_NEXT_ACTIVE: mog_activeq_push(q, mfd); return;
	case MOG_NEXT_WAIT_WR: mog_idleq_push(q, mfd, MOG_QEV_WR); return;
	}
}

/* called immediately after accept(), this initializes the mfd (once) */
void mog_mgmt_post_accept(int fd, struct mog_svc *svc)
{
	struct mog_fd *mfd = mog_fd_init(fd, MOG_FD_TYPE_MGMT);
	struct mog_mgmt *mgmt = &mfd->as.mgmt;

	mog_mgmt_init(mgmt, svc);
	mog_idleq_add(svc->queue, mfd, MOG_QEV_RD);
}
