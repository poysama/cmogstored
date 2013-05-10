/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "http.h"

/*
 * On FreeBSD, this disables TCP_NOPUSH momentarily to flush out corked data.
 * This immediately recorks again if we're handling persistent connections,
 * otherwise we leave it uncorked if we're going to close the socket anyways
 */
static void tcp_push(struct mog_fd *mfd, bool recork)
{
	socklen_t len = (socklen_t)sizeof(int);
	int val = 0;
	int rv;

	if (MOG_TCP_NOPUSH == 0)
		return;

	rv = setsockopt(mfd->fd, IPPROTO_TCP, MOG_TCP_NOPUSH, &val, len);

	if (rv == 0 && recork) {
		val = 1;
		setsockopt(mfd->fd, IPPROTO_TCP, MOG_TCP_NOPUSH, &val, len);
	}
	/* deal with errors elsewhere */
}

/* stash any pipelined data for the next round */
static void
http_defer_rbuf(struct mog_http *http, struct mog_rbuf *rbuf, size_t buf_len)
{
	struct mog_rbuf *old = http->rbuf;
	size_t defer_bytes = buf_len - http->offset;
	char *src = rbuf->rptr + http->offset;

	if (http->skip_rbuf_defer) {
		http->skip_rbuf_defer = 0;
		return;
	}

	assert(http->offset >= 0 && "http->offset negative");
	assert(defer_bytes <= MOG_RBUF_MAX_SIZE && "defer bytes overflow");

	if (defer_bytes == 0) {
		mog_rbuf_free_and_null(&http->rbuf);
	} else if (old) { /* no allocation needed, reuse existing */
		assert(old == rbuf && "http->rbuf not reused properly");
		memmove(old->rptr, src, defer_bytes);
		old->rsize = defer_bytes;
	} else {
		http->rbuf = mog_rbuf_new(defer_bytes);
		memcpy(http->rbuf->rptr, src, defer_bytes);
		http->rbuf->rsize = defer_bytes;
	}
	http->offset = 0;
}

static void
http_process_client(struct mog_http *http, char *buf, size_t buf_len)
{
	switch (http->http_method) {
	case MOG_HTTP_METHOD_NONE: assert(0 && "BUG: unset HTTP method");
	case MOG_HTTP_METHOD_GET: mog_http_get_open(http, buf); break;
	case MOG_HTTP_METHOD_HEAD: mog_http_get_open(http, buf); break;
	case MOG_HTTP_METHOD_DELETE: mog_http_delete(http, buf); break;
	case MOG_HTTP_METHOD_MKCOL: mog_http_mkcol(http, buf); break;
	case MOG_HTTP_METHOD_PUT: mog_http_put(http, buf, buf_len); break;
	}
}

MOG_NOINLINE static void http_close(struct mog_fd *mfd)
{
	struct mog_http *http = &mfd->as.http;

	mog_rbuf_free(http->rbuf);
	assert((http->wbuf == NULL || http->wbuf == MOG_WR_ERROR) &&
	       "would leak http->wbuf on close");

	/*
	 * uncork to avoid ECONNCRESET if we have unread data
	 * (but already wrote a response).  This can help get
	 * the proper error sent to the client if the client is
	 * writing a request that's too big to read and we reset
	 * their connection to save ourselves bandwidth/cycles
	 */
	tcp_push(mfd, false);

	mog_fd_put(mfd);
}

/* returns true if we can continue queue step, false if not */
static enum mog_next http_wbuf_in_progress(struct mog_http *http)
{
	assert(http->wbuf != MOG_WR_ERROR && "still active after write error");
	switch (mog_tryflush(mog_fd_of(http)->fd, &http->wbuf)) {
	case MOG_WRSTATE_ERR:
		return MOG_NEXT_CLOSE;
	case MOG_WRSTATE_DONE:
		if (!http->persistent) return MOG_NEXT_CLOSE;
		if (http->forward == NULL)
			mog_http_reset(http);
		assert(http->offset == 0 && "bad offset");
		return MOG_NEXT_ACTIVE;
	case MOG_WRSTATE_BUSY:
		/* unlikely, we never put anything big in wbuf */
		return MOG_NEXT_WAIT_WR;
	}
	assert(0 && "compiler bug?");
	return MOG_NEXT_CLOSE;
}

static enum mog_next http_forward_in_progress(struct mog_fd *mfd)
{
	enum mog_http_method method = mfd->as.http.http_method;

	if (method == MOG_HTTP_METHOD_GET)
		return mog_http_get_in_progress(mfd);

	assert(method == MOG_HTTP_METHOD_PUT && "bad http_method for forward");

	return mog_http_put_in_progress(mfd);
}

static enum mog_next http_queue_step(struct mog_fd *mfd)
{
	struct mog_http *http = &mfd->as.http;
	struct mog_rbuf *rbuf;
	char *buf;
	ssize_t r;
	off_t off;
	size_t buf_len = 0;
	enum mog_parser_state state;

	assert(mfd->fd >= 0 && "http fd is invalid");

	if (http->wbuf) return http_wbuf_in_progress(http);
	if (http->forward) return http_forward_in_progress(mfd);

	/* we may have pipelined data in http->rbuf */
	rbuf = http->rbuf ? http->rbuf : mog_rbuf_get(MOG_RBUF_BASE_SIZE);
	buf = rbuf->rptr;
	off = http->offset;
	assert(off >= 0 && "offset is negative");
	assert(off < rbuf->rcapa && "offset is too big");
	if (http->rbuf) {
		/* request got pipelined, resuming now */
		buf_len = http->rbuf->rsize;
		assert(http->offset <= buf_len && "bad offset from pipelining");
		assert(buf_len <= http->rbuf->rcapa && "bad rsize stashed");
		if (http->offset < buf_len)
			goto parse;
	}
reread:
	r = read(mfd->fd, buf + off, rbuf->rcapa - off);
	if (r > 0) {
		buf_len = r + off;
parse:
		state = mog_http_parse(http, buf, buf_len);

		switch (state) {
		case MOG_PARSER_ERROR:
			goto err507or400;
		case MOG_PARSER_CONTINUE:
			assert(http->wbuf == NULL &&
			       "tried to write (and failed) with partial req");
			if (http->offset >= rbuf->rcapa) {
				rbuf->rsize = buf_len;
				http->rbuf = rbuf = mog_rbuf_grow(rbuf);
				if (!rbuf)
					goto err400;
				buf = rbuf->rptr;
			}
			off = http->offset;
			goto reread;
		case MOG_PARSER_DONE:
			http_process_client(http, buf, buf_len);
			if (http->wbuf == MOG_WR_ERROR)
				return MOG_NEXT_CLOSE;
			if (http->wbuf) {
				http_defer_rbuf(http, rbuf, buf_len);
				return MOG_NEXT_WAIT_WR;
			} else if (http->forward) {
				http_defer_rbuf(http, rbuf, buf_len);
				return http_forward_in_progress(mfd);
			} else if (!http->persistent) {
				return MOG_NEXT_CLOSE;
			} else {
				http_defer_rbuf(http, rbuf, buf_len);
				mog_http_reset(http);
			}
			return MOG_NEXT_ACTIVE;
		}
	} else if (r == 0) { /* client shut down */
		return MOG_NEXT_CLOSE;
	} else {
		switch (errno) {
		case_EAGAIN:
			if (buf_len > 0) {
				if (http->rbuf == NULL)
					http->rbuf = mog_rbuf_detach(rbuf);
				http->rbuf->rsize = buf_len;
			}
			return MOG_NEXT_WAIT_RD;
		case EINTR: goto reread;
		case ECONNRESET:
		case ENOTCONN:
			return MOG_NEXT_CLOSE;
		default:
			syslog(LOG_NOTICE, "http client died: %m");
			return MOG_NEXT_CLOSE;
		}
	}

	assert(0 && "compiler bug?");

err507or400:
	if (errno == ERANGE) {
		mog_http_resp(http, "507 Insufficient Storage", false);
	} else {
err400:
		mog_http_resp(http, "400 Bad Request", false);
	}
	return MOG_NEXT_CLOSE;
}

enum mog_next mog_http_queue_step(struct mog_fd *mfd)
{
	enum mog_next rv = http_queue_step(mfd);

	if (rv == MOG_NEXT_CLOSE)
		http_close(mfd);
	assert(rv != MOG_NEXT_IGNORE &&
	       "refusing to put HTTP client into ignore state");
	return rv;
}

/* called during graceful shutdown instead of mog_http_queue_step */
void mog_http_quit_step(struct mog_fd *mfd)
{
	struct mog_http *http = &mfd->as.http;
	struct mog_queue *q = http->svc->queue;

	/* centralize all queue transitions here: */
	switch (http_queue_step(mfd)) {
	case MOG_NEXT_WAIT_RD:
		if (http->forward || http->rbuf) {
			mog_idleq_push(q, mfd, MOG_QEV_RD);
			return;
		}
		/* fall-through */
	case MOG_NEXT_CLOSE:
		mog_nr_active_at_quit--;
		http_close(mfd);
		return;
	case MOG_NEXT_ACTIVE: mog_activeq_push(q, mfd); return;
	case MOG_NEXT_WAIT_WR: mog_idleq_push(q, mfd, MOG_QEV_WR); return;
	case MOG_NEXT_IGNORE:
		assert(0 && "refused to put HTTP client into ignore state");
	}
}

/* called immediately after accept(), this initializes the mfd (once) */
void mog_http_post_accept(int fd, struct mog_svc *svc)
{
	struct mog_fd *mfd = mog_fd_init(fd, MOG_FD_TYPE_HTTP);
	struct mog_http *http = &mfd->as.http;

	mog_http_init(http, svc);
	mog_idleq_add(svc->queue, mfd, MOG_QEV_RD);
}

/* called immediately after accept(), this initializes the mfd (once) */
void mog_httpget_post_accept(int fd, struct mog_svc *svc)
{
	struct mog_fd *mfd = mog_fd_init(fd, MOG_FD_TYPE_HTTPGET);
	struct mog_http *http = &mfd->as.http;

	mog_http_init(http, svc);
	mog_idleq_add(svc->queue, mfd, MOG_QEV_RD);
}

/*
 * returns a NUL-terminated HTTP path from the rbuf pointer
 * returns NUL if we got a bad path.
 */
char *mog_http_path(struct mog_http *http, char *buf)
{
	char *path = buf + http->path_tip;
	size_t len = http->path_end - http->path_tip;

	assert(http->path_end > http->path_tip && "bad HTTP path from parser");

	if (! mog_valid_path(path, len))
		return NULL;

	if (http->http_method == MOG_HTTP_METHOD_PUT) {
		if (!mog_valid_put_path(path, len)) {
			errno = EINVAL;
			return NULL;
		}
	}

	path[len] = '\0';

	return path;
}


/* TODO: see if the iovec overheads of writev() is even worth it... */
void
mog_http_resp0(
	struct mog_http *http,
	struct iovec *status,
	bool alive)
{
	struct iovec iov;
	struct mog_now *now;
	char *dst = iov.iov_base = mog_fsbuf_get(&iov.iov_len);

	assert(status->iov_len * 2 + 1024 < iov.iov_len && "fsbuf too small");

#define CPY(str) mempcpy(dst, (str),(sizeof(str)-1))
	dst = CPY("HTTP/1.1 ");
	dst = mempcpy(dst, status->iov_base, status->iov_len);
	dst = CPY("\r\nStatus: ");
	dst = mempcpy(dst, status->iov_base, status->iov_len);
	dst = CPY("\r\nDate: ");
	now = mog_now();
	dst = mempcpy(dst, now->httpdate, sizeof(now->httpdate)-1);
	if (alive && http->persistent) {
		dst = CPY("\r\nContent-Length: 0"
			"\r\nContent-Type: text/plain"
			"\r\nConnection: keep-alive\r\n\r\n");
	} else {
		http->persistent = 0;
		dst = CPY("\r\nContent-Length: 0"
			"\r\nContent-Type: text/plain"
			"\r\nConnection: close\r\n\r\n");
	}
	iov.iov_len = dst - (char *)iov.iov_base;
	assert(http->wbuf == NULL && "tried to write with wbuf");

	http->wbuf = mog_trywritev(mog_fd_of(http)->fd, &iov, 1);
}

/* call whenever we're ready to read the next HTTP request */
void mog_http_reset(struct mog_http *http)
{
	tcp_push(mog_fd_of(http), true);
	mog_http_reset_parser(http);
}
