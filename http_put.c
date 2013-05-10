/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "http.h"
#include "digest.h"

static __thread struct {
	bool ready;
	struct random_data data;
	char state[128];
} rnd;

static void file_close_null(struct mog_http *http)
{
	struct mog_file *file;

	if (http->forward == NULL)
		return;

	file = &http->forward->as.file;

	if (file->tmppath) {
		if (mog_unlink(http->svc, file->tmppath) != 0)
			syslog(LOG_ERR, "Failed to unlink %s (in %s): %m",
			       file->tmppath, http->svc->docroot);
	}
	mog_file_close(http->forward);
	http->forward = NULL;
}

bool mog_http_write_full(struct mog_fd *file_mfd, char *buf, size_t buf_len)
{
	struct mog_file *file = &file_mfd->as.file;
	ssize_t w;
	const char *errpath;

	if (file->digest.ctx)
		gc_hash_write(file->digest.ctx, buf_len, buf);
	if (buf_len == 0)
		return true;

	errno = 0;
	for (;;) {
		w = pwrite(file_mfd->fd, buf, buf_len, file->foff);

		if (w > 0) {
			file->foff += w;
			if (w == buf_len)
				return true;

			buf_len -= w;
			buf += w;
			continue;
		}
		if (w < 0 && errno == EINTR)
			continue;
		if (w == 0 && errno == 0)
			errno = ENOSPC;

		break;
	}

	errpath = file->tmppath ? file->tmppath : file->path;

	PRESERVE_ERRNO(do {
		if (w == 0)
			syslog(LOG_ERR,
			       "pwrite() to %s wrote zero bytes of "
			       "%llu at offset: %lld: assuming %m",
			       errpath, (unsigned long long)buf_len,
			       (long long)file->foff);
		else
			syslog(LOG_ERR,
			       "pwrite() to %s failed at offset: %lld: %m",
			       errpath, (long long)file->foff);
	} while (0));

	return false;
}

#define stop(http,status) stop0((http),(status),sizeof(status)-1);

MOG_NOINLINE static enum mog_next
stop0(struct mog_http *http, const char *status, size_t status_len)
{
	if (status) {
		struct iovec iov;
		union { const char *in; char *out; } deconst;

		deconst.in = status;
		iov.iov_base = deconst.out;
		iov.iov_len = status_len;

		mog_http_resp0(http, &iov, false);
	}
	file_close_null(http);
	return MOG_NEXT_CLOSE;
}

MOG_NOINLINE static enum mog_next
write_err(struct mog_http *http, const char *default_msg)
{
	switch (errno) {
	case ERANGE:
	case ENOSPC:
	case EFBIG:
		return stop(http, "507 Insufficient Storage");
	}

	if (default_msg == NULL)
		default_msg = "500 Internal Server Error";

	return stop0(http, default_msg, strlen(default_msg));
}

static bool md5_ok(struct mog_http *http)
{
	gc_hash_handle ctx = http->forward->as.file.digest.ctx;
	const char *result;

	/* PUT requests don't _require_ Content-MD5 header/trailer */
	if (ctx == NULL)
		return true;

	result = gc_hash_read(ctx);

	return (memcmp(http->expect_md5, result, 16) == 0);
}

static bool set_perms_commit(struct mog_http *http)
{
	struct mog_file *file = &http->forward->as.file;

	if (fchmod(http->forward->fd, http->svc->put_perms) != 0) {
		syslog(LOG_ERR, "fchmod() failed: %m");
		return false;
	}

	if (file->tmppath == NULL)
		return true;
	assert(file->path && "file->path NULL when file->tmppath set");
	if (mog_rename(http->svc, file->tmppath, file->path) == 0) {
		mog_free_and_null(&file->tmppath);
		return true;
	}

	syslog(LOG_ERR, "renameat(%s => %s) failed: %m",
	       file->tmppath, file->path);
	return false;
}

static void put_commit_resp(struct mog_http *http)
{
	if (md5_ok(http)) { /* true if there's no MD5, too */
		if (set_perms_commit(http)) {
			file_close_null(http);
			mog_http_resp(http, "201 Created", true);
		} else {
			file_close_null(http);
			mog_http_resp(http, "500 Internal Server Error", false);
		}
	} else {
		file_close_null(http);
		mog_http_resp(http, "400 Bad Request", true);
	}
}

static enum mog_next http_put_commit(struct mog_http *http)
{
	put_commit_resp(http);

	if (http->wbuf && http->wbuf != MOG_WR_ERROR)
		return MOG_NEXT_WAIT_WR;
	if (!http->persistent || http->wbuf == MOG_WR_ERROR)
		return MOG_NEXT_CLOSE;
	mog_http_reset(http);
	return MOG_NEXT_ACTIVE;
}

static void stash_advance_rbuf(struct mog_http *http, char *buf, size_t buf_len)
{
	struct mog_rbuf *rbuf = http->rbuf;
	size_t end = http->line_end + 1;

	if (http->line_end == 0 || buf_len <= end) {
		http->offset = 0;
		mog_rbuf_free_and_null(&http->rbuf);
		return;
	}

	assert(buf[http->line_end] == '\n' && "line_end is not LF");
	assert(buf_len <= MOG_RBUF_MAX_SIZE && "bad rbuf size");
	assert(end <= http->offset && "invalid line end");
	if (rbuf == NULL)
		http->rbuf = rbuf = mog_rbuf_new(MOG_RBUF_BASE_SIZE);

	memmove(rbuf->rptr, buf + end, buf_len - end);
	rbuf->rsize = buf_len - end;
	http->offset -= end;
	if (http->tmp_tip >= end)
		http->tmp_tip -= end;
	http->line_end = 0;
}

static void
chunked_body_after_header(struct mog_http *http, char *buf, size_t buf_len)
{
	size_t tmpoff = http->offset;

	mog_chunk_init(http);
	http->offset = tmpoff;

	switch (mog_chunk_parse(http, buf, buf_len)) {
	case MOG_PARSER_ERROR:
		(void)write_err(http, "400 Bad Request");
		return;
	case MOG_PARSER_CONTINUE:
		assert(http->chunk_state != MOG_CHUNK_STATE_DONE);
		/* fall through */
	case MOG_PARSER_DONE:
		switch (http->chunk_state) {
		case MOG_CHUNK_STATE_SIZE:
			assert(http->offset == buf_len
			       && "HTTP chunk parser didn't finish size");
			return;
		case MOG_CHUNK_STATE_DATA:
			assert(http->offset == buf_len
			       && "HTTP chunk parser didn't finish data");
			return;
		case MOG_CHUNK_STATE_TRAILER:
			assert(http->offset > 0 &&
			       "http->offset unset while in trailer");
			stash_advance_rbuf(http, buf, buf_len);
			http->skip_rbuf_defer = 1;
			return;
		case MOG_CHUNK_STATE_DONE:
			put_commit_resp(http);
			assert(http->offset > 0 &&
			       "http->offset unset after chunk body done");
			stash_advance_rbuf(http, buf, buf_len);
			http->skip_rbuf_defer = 1;
		}
	}
}

static void
identity_body_after_header(struct mog_http *http, char *buf, size_t buf_len)
{
	size_t body_len = buf_len - http->offset;
	char *body_ptr = buf + http->offset;

	if (http->content_len < body_len)
		body_len = http->content_len;
	if (body_len == 0)
		return;
	http->offset += body_len;
	if (!mog_http_write_full(http->forward, body_ptr, body_len))
		(void)write_err(http, NULL);
}

static bool lengths_ok(struct mog_http *http)
{
	if (http->content_len < 0)
		return false;	/* ERANGE */

	if (http->has_content_range) {
		if (http->chunked)
			return false;

		if (http->range_end < 0 || http->range_beg < 0)
			return false;	/* ERANGE */

		assert(http->range_end >= 0 && http->range_beg >= 0 &&
		       "bad range, http_parser.rl broken");

		/* can't end after we start */
		if (http->range_end < http->range_beg)
			return false;

		/*
		 * Content-Length should match Content-Range boundaries
		 * WARNING: Eric Wong sucks at arithmetic, check this:
		 */
		if (http->content_len >= 0) {
			off_t expect = http->range_end - http->range_beg + 1;

			if (http->content_len != expect)
				return false;
		}
	}
	return true;
}

MOG_NOINLINE static void rnd_init_per_thread(void)
{
	unsigned seed = (unsigned)((size_t)&rnd >> 1);

	CHECK(int, 0,
	      initstate_r(seed, rnd.state, sizeof(rnd.state), &rnd.data));
	rnd.ready = true;
}

static char *tmppath_for(struct mog_http *http, const char *path)
{
	int32_t result;

	if (!rnd.ready)
		rnd_init_per_thread();

	assert(http && "validation later"); /* TODO */
	CHECK(int, 0, random_r(&rnd.data, &result));

	return xasprintf("%s.%08x.%d.tmp",
	                 path, (unsigned)result, (int)getpid());
}

static struct mog_file * open_put(struct mog_http *http, char *path)
{
	struct mog_file *file;

	/*
	 * we can't do an atomic rename(2) on successful PUT
	 * if we have a partial upload
	 */
	if (http->has_content_range) {
		http->forward = mog_file_open_put(http->svc, path, O_CREAT);
		if (http->forward == NULL)
			return NULL;

		file = &http->forward->as.file;
		assert(file->tmppath == NULL && file->path == NULL &&
		       "file->*path should both be NULL after open");
	} else {
		char *tmp = tmppath_for(http, path);
		int fl = O_EXCL | O_TRUNC | O_CREAT;

		http->forward = mog_file_open_put(http->svc, tmp, fl);

		/* retry once on EEXIST, don't inf loop if RNG is broken */
		if (http->forward == NULL && errno == EEXIST) {
			free(tmp);
			tmp = tmppath_for(http, path);
			http->forward = mog_file_open_put(http->svc, tmp, fl);
		}
		if (http->forward == NULL) {
			PRESERVE_ERRNO( free(tmp) );
			return NULL;
		}
		file = &http->forward->as.file;
		file->tmppath = tmp;
	}

	file->path = xstrdup(path);
	assert(file->foff == 0 && "file->foff should be zero");
	if (http->has_content_range)
		file->foff = http->range_beg;
	if (http->has_trailer_md5 || http->has_expect_md5)
		mog_digest_init(&file->digest, GC_MD5);

	return file;
}

void mog_http_put(struct mog_http *http, char *buf, size_t buf_len)
{
	char *path;
	struct mog_file *file;

	if (mog_fd_of(http)->fd_type == MOG_FD_TYPE_HTTPGET) {
		mog_http_resp(http, "405 Method Not Allowed", false);
		return;
	}

	path = mog_http_path(http, buf);
	if (path == NULL)
		goto err;	/* bad path */
	assert(http->forward == NULL && "already have http->forward");
	assert(path[0] == '/' && "bad path");

	if (!lengths_ok(http)) {
		write_err(http, "400 Bad Request");
		return;
	}

	file = open_put(http, path);
	if (file == NULL)
		goto err;

	if (buf_len == http->offset) {
		/* we got the HTTP header in one read() */
		if (http->chunked) {
			mog_rbuf_free_and_null(&http->rbuf);
			mog_chunk_init(http);
			http->offset = buf_len;
		}
		return;
	}
	/*
	 * otherwise we got part of the request body with the header,
	 * write partially read body
	 */
	assert(buf_len > http->offset && http->offset > 0
	       && "http->offset is wrong");

	if (http->chunked)
		chunked_body_after_header(http, buf, buf_len);
	else
		identity_body_after_header(http, buf, buf_len);

	return;
err:
	switch (errno) {
	case EINVAL:
		mog_http_resp(http, "400 Bad Request", false);
		return;
	case ENOENT:
		mog_http_resp(http, "404 Not Found", false);
		return;
	case EACCES:
		mog_http_resp(http, "403 Forbidden", false);
		return;
	}
	syslog(LOG_ERR, "problem starting PUT for path=%s (%m)", path);
	(void)write_err(http, NULL);
}

static unsigned last_data_recv(int fd)
{
#ifdef TCP_INFO
	struct tcp_info info;
	socklen_t len = (socklen_t)sizeof(struct tcp_info);
	int rc = getsockopt(fd, IPPROTO_TCP, TCP_INFO, &info, &len);

	if (rc == 0)
		return (unsigned)info.tcpi_last_data_recv;
#endif /* TCP_INFO */
	return (unsigned)-1;
}

MOG_NOINLINE static void read_err_dbg(struct mog_fd *mfd, ssize_t r)
{
	union mog_sockaddr any;
	char addrbuf[MOG_NI_MAXHOST];
	char portbuf[MOG_NI_MAXSERV];
	const char *addr;
	static const char no_ip[] = "unconnected";
	const char *path = "(unknown)";
	long long bytes = -1;
	const char *errfmt;
	unsigned last;

	PRESERVE_ERRNO(last = last_data_recv(mfd->fd));

	portbuf[0] = 0;
	PRESERVE_ERRNO(do {
		socklen_t len = (socklen_t)sizeof(any);
		socklen_t addrlen = (socklen_t)sizeof(addrbuf);
		socklen_t portlen = (socklen_t)(sizeof(portbuf));
		int rc = getpeername(mfd->fd, &any.sa, &len);

		if (rc == 0) {
			rc = getnameinfo(&any.sa, len, addrbuf, addrlen,
			                 portbuf + 1, portlen - 1,
					 NI_NUMERICHOST|NI_NUMERICSERV);
			if (rc == 0) {
				addr = addrbuf;
				portbuf[0] = ':';
			} else {
				addr = gai_strerror(rc);
			}
		} else {
			syslog(LOG_ERR, "getpeername() failed for fd=%d: %m",
			       mfd->fd);
			addr = no_ip;
		}
	} while (0));

	if (mfd->as.http.forward) {
		path = mfd->as.http.forward->as.file.path;
		bytes = (long long)mfd->as.http.forward->as.file.foff;
	}

#define PFX "PUT %s failed from %s%s after %lld bytes: "
	errfmt = (r == 0) ? PFX"premature EOF" : PFX"%m";
#undef PFX
	syslog(LOG_ERR, errfmt, path, addr, portbuf, bytes);

	if (last != (unsigned)-1)
		syslog(LOG_ERR, "last_data_recv=%ums from %s%s for PUT %s",
		       last, addr, portbuf, path);
}

static enum mog_next identity_put_in_progress(struct mog_fd *mfd)
{
	struct mog_http *http = &mfd->as.http;
	ssize_t r;
	size_t buf_len;
	char *buf;
	off_t need;

	assert(http->wbuf == NULL && "can't receive file with http->wbuf");
	assert(http->forward && http->forward != MOG_IOSTAT && "bad forward");

	need = http->content_len - http->forward->as.file.foff;
	if (http->has_content_range)
		need += http->range_beg;
	if (need == 0)
		return http_put_commit(http);

	buf = mog_fsbuf_get(&buf_len);
again:
	assert(need > 0 && "over-wrote on PUT request");
	if (need < buf_len)
		buf_len = need;
retry:
	r = read(mfd->fd, buf, buf_len);
	if (r > 0) {
		if (!mog_http_write_full(http->forward, buf, r))
			return write_err(http, NULL);
		need -= r;
		if (need == 0)
			return http_put_commit(http);
		goto again;
	}
	if (r != 0) {
		switch (errno) {
		case_EAGAIN: return MOG_NEXT_WAIT_RD;
		case EINTR: goto retry;
		}
	}

	/* assume all read() errors mean socket is unwritable, too */
	read_err_dbg(mfd, r);
	return stop(http, NULL);
}

static enum mog_next chunked_put_in_progress(struct mog_fd *mfd)
{
	struct mog_rbuf *rbuf;
	struct mog_http *http = &mfd->as.http;
	ssize_t r;
	size_t buf_len;
	size_t prev_len;
	char *buf;
	bool in_trailer = false;

again:
	assert(http->wbuf == NULL && "can't receive file with http->wbuf");
	assert(http->forward && http->forward != MOG_IOSTAT && "bad forward");

	switch (http->chunk_state) {
	case MOG_CHUNK_STATE_DATA:
		assert(http->rbuf == NULL && "unexpected http->rbuf");
		if (http->content_len == 0) { /* final chunk */
			http->chunk_state = MOG_CHUNK_STATE_TRAILER;
			http->offset = 0;
			goto chunk_state_trailer;
		}
		assert(http->content_len > 0 && "bad chunk length");
		/* read the chunk into memory */
		buf = mog_fsbuf_get(&buf_len);
		if (buf_len > http->content_len)
			buf_len = http->content_len;
		do {
			r = read(mfd->fd, buf, buf_len);
		} while (r < 0 && errno == EINTR);

		if (r <= 0)
			goto read_err;
		if (!mog_http_write_full(http->forward, buf, r))
			return write_err(http, NULL);

		http->content_len -= r;

		/* chunk is complete */
		if (http->content_len == 0)
			mog_chunk_init(http);
		goto again;
	case MOG_CHUNK_STATE_TRAILER:
chunk_state_trailer:
		in_trailer = true;
		/* fall-through */
	case MOG_CHUNK_STATE_SIZE:
		rbuf = http->rbuf;
		if (rbuf) {
			prev_len = rbuf->rsize;
			buf_len = rbuf->rcapa - prev_len;
			buf = rbuf->rptr + prev_len;
			/*
			 * buf_len == 0 may happen here if client sends
			 * us very bogus data... just 400 it below
			 */
		} else {
			prev_len = 0;
			rbuf = mog_rbuf_get(MOG_RBUF_BASE_SIZE);
			buf_len = rbuf->rcapa;
			buf = rbuf->rptr;
		}
		do {
			r = read(mfd->fd, buf, buf_len);
		} while (r < 0 && errno == EINTR);
		if (r <= 0)
			goto read_err;

		buf = rbuf->rptr;
		buf_len = r + prev_len;

		switch (mog_chunk_parse(http, buf, buf_len)) {
		case MOG_PARSER_ERROR:
			return write_err(http, "400 Bad Request");
		case MOG_PARSER_CONTINUE:
			assert(http->chunk_state != MOG_CHUNK_STATE_DONE);
		case MOG_PARSER_DONE:
			switch (http->chunk_state) {
			case MOG_CHUNK_STATE_SIZE:
				if (in_trailer)
					assert(0 && "bad chunk state: size");
				/* client is trickling chunk size :< */
				mog_rbuf_free_and_null(&http->rbuf);
				http->offset = 0;
				goto again;
			case MOG_CHUNK_STATE_DATA:
				if (in_trailer)
					assert(0 && "bad chunk state: data");
				/* client is trickling final chunk/trailer */
				mog_rbuf_free_and_null(&http->rbuf);
				goto again;
			case MOG_CHUNK_STATE_TRAILER:
				stash_advance_rbuf(http, buf, buf_len);
				goto again;
			case MOG_CHUNK_STATE_DONE:
				stash_advance_rbuf(http, buf, buf_len);

				/* pipelined HTTP request after trailers! */
				if (http->rbuf)
					assert(http->rbuf->rsize > 0
					       && http->offset == 0
					       && "bad rbuf");
				return http_put_commit(http);
			}
		}
		assert(0 && "compiler bug?");
	case MOG_CHUNK_STATE_DONE:
		assert(0 && "invalid state");
	}

read_err:
	if (r < 0) {
		switch (errno) {
		case_EAGAIN: return MOG_NEXT_WAIT_RD;
		}
	}
	read_err_dbg(mfd, r);
	return stop(http, NULL);
}

enum mog_next mog_http_put_in_progress(struct mog_fd *mfd)
{
	if (mfd->as.http.chunked)
		return chunked_put_in_progress(mfd);

	return identity_put_in_progress(mfd);
}
