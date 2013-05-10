/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */

#include "cmogstored.h"
#include "http.h"
#if defined(HAVE_SENDFILE)
#  if defined(__linux__)
#    include <sys/sendfile.h>
#  else
/*
 * make BSD sendfile look like Linux for now...
 * we can support SF_NODISKIO later
 */
static ssize_t linux_sendfile(int sockfd, int filefd, off_t *off, size_t count)
{
	int flags = 0;
	off_t sbytes = 0;
	int rc;

	rc = sendfile(filefd, sockfd, *off, count, NULL, &sbytes, flags);
	if (sbytes > 0) {
		*off += sbytes;
		return (ssize_t)sbytes;
	}

	return (ssize_t)rc;
}
#  define sendfile(out_fd, in_fd, offset, count) \
          linux_sendfile((out_fd),(in_fd),(offset),(count))
#  endif
#else
#  include "compat_sendfile.h"
#endif

#define ERR416 "416 Requested Range Not Satisfiable"

/*
 * TODO: refactor this
 *
 * snprintf() usage here is a hot spot in profiling.  Perhaps one day,
 * link-time optimization will be able to work on *printf() functions
 * so we won't hurt code maintainability by optimizing away snprintf()
 * ourselves.  This function is ugly enough already
 */
static off_t http_get_resp_hdr(struct mog_http *http, struct stat *sb)
{
	char *modified;
	void *buf;
	size_t len;
	struct mog_now *now = mog_now();
	long long count;
	int rc;

	/* single buffer so we can use MSG_MORE */
	buf = mog_fsbuf_get(&len);
	modified = (char *)buf + len / 2;
	assert((len / 2) > MOG_HTTPDATE_CAPA && "fsbuf too small");
	mog_http_date(modified, MOG_HTTPDATE_CAPA, &sb->st_mtime);

	/* validate ranges */
	if (http->has_range) {
		long long offset;

		if (http->range_end < 0 && http->range_beg < 0)
			goto bad_range;
		if (http->range_beg >= sb->st_size)
			goto bad_range;

		/* bytes=M-N where M > N */
		if (http->range_beg >= 0 && http->range_end >= 0
		    && http->range_beg > http->range_end)
			goto bad_range;

		if (http->range_end < 0) { /* bytes=M- */
			/* bytes starting at M until EOF */
			assert(http->range_beg >= 0 && "should've sent 416");
			offset = (long long)http->range_beg;
			count = (long long)(sb->st_size - offset);
		} else if (http->range_beg < 0) { /* bytes=-N */
			/* last N bytes */
			assert(http->range_end >= 0 && "should've sent 416");
			offset = (long long)(sb->st_size - http->range_end);

			/* serve the entire file if client requested too much */
			if (offset < 0)
				goto resp_200;
			count = (long long)(sb->st_size - offset);
		} else { /* bytes=M-N*/
			assert(http->range_beg >= 0 && http->range_end >= 0
			       && "should've sent 416");
			offset = (long long)http->range_beg;

			/* truncate responses to current file size */
			if (http->range_end >= sb->st_size)
				http->range_end = sb->st_size - 1;
			count = (long long)http->range_end + 1 - offset;
		}

		assert(count > 0 && "bad count for 206 response");
		assert(offset >= 0 && "bad offset for 206 response");

		if (http->forward) {
			struct mog_file *file = &http->forward->as.file;

			file->foff = offset;
			file->fsize = (off_t)(offset + count);
		}

		rc = snprintf(buf, len,
			"HTTP/1.1 206 Partial Content\r\n"
			"Status: 206 Partial Content\r\n"
			"Date: %s\r\n"
			"Last-Modified: %s\r\n"
			"Content-Length: %lld\r\n"
			"Content-Type: application/octet-stream\r\n"
			"Content-Range: bytes %lld-%lld/%lld\r\n"
			"Connection: %s\r\n"
			"\r\n",
			now->httpdate,
			modified,
			count, /* Content-Length */
			offset, offset + count - 1, /* bytes M-N */
			(long long)sb->st_size,
			http->persistent ? "keep-alive" : "close");
	} else {
resp_200:
		count = (long long)sb->st_size;
		rc = snprintf(buf, len,
			"HTTP/1.1 200 OK\r\n"
			"Status: 200 OK\r\n"
			"Date: %s\r\n"
			"Last-Modified: %s\r\n"
			"Content-Length: %lld\r\n"
			"Content-Type: application/octet-stream\r\n"
			"Accept-Ranges: bytes\r\n"
			"Connection: %s\r\n"
			"\r\n",
			now->httpdate,
			modified,
			count,
			http->persistent ? "keep-alive" : "close");
	}

	/* TODO: put down the crack pipe and refactor this */
	if (0) {
bad_range:
		count = 0;
		if (http->forward) {
			mog_file_close(http->forward);
			http->forward = NULL;
		} else {
			assert(http->http_method == MOG_HTTP_METHOD_HEAD
			       && "not HTTP HEAD");
		}
		rc = snprintf(buf, len,
			"HTTP/1.1 " ERR416 "\r\n"
			"Status: " ERR416 "\r\n"
			"Date: %s\r\n"
			"Content-Length: 0\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Range: bytes */%lld\r\n"
			"Connection: %s\r\n"
			"\r\n",
			now->httpdate,
			(long long)sb->st_size,
			http->persistent ? "keep-alive" : "close");
	}

	assert(rc > 0 && rc < len && "we suck at snprintf");
	len = (size_t)rc;
	assert(http->wbuf == NULL && "tried to write to a busy client");

	if (http->http_method == MOG_HTTP_METHOD_HEAD)
		count = 0;

	http->wbuf = mog_trysend(mog_fd_of(http)->fd, buf, len, (off_t)count);

	return (off_t)count;
}

void mog_http_get_open(struct mog_http *http, char *buf)
{
	struct stat sb;
	struct mog_file *file = NULL;
	char *path = mog_http_path(http, buf);
	off_t len;

	if (!path) goto forbidden; /* path traversal attack */
	assert(http->forward == NULL && "already have http->forward");
	assert(path[0] == '/' && "bad path");

	if (path[1] == '\0') { /* keep "mogadm check" happy */
		sb.st_mtime = 0;
		sb.st_size = 0;
	} else if (http->http_method == MOG_HTTP_METHOD_HEAD) {
		if (mog_stat(http->svc, path, &sb) < 0) goto err;
		if (!S_ISREG(sb.st_mode)) goto forbidden;
	} else {
		http->forward = mog_file_open_read(http->svc, path);
		if (http->forward == NULL)
			goto err;

		file = &http->forward->as.file;
		assert(file->path == NULL && "build system bug");
		if (fstat(http->forward->fd, &sb) < 0) {
			PRESERVE_ERRNO( mog_file_close(http->forward) );
			http->forward = NULL;
			goto err;
		}
		if (!S_ISREG(sb.st_mode)) {
			mog_file_close(http->forward);
			http->forward = NULL;
			goto forbidden;
		}
		file->fsize = sb.st_size;
	}

	len = http_get_resp_hdr(http, &sb);

	/* http->forward may be NULL even if file is set if we had an error */
	if (http->wbuf == NULL && http->forward) {
		assert(file && "file unset but http->forward is set");

		if (len > (256 * 1024))
			mog_fadv_sequential(http->forward->fd, file->foff, len);
	}
	return;
err:
	switch (errno) {
	case EACCES:
forbidden:
		mog_http_resp(http, "403 Forbidden", true);
		return;
	case ENOENT:
		mog_http_resp(http, "404 Not Found", true);
		return;
	}
	PRESERVE_ERRNO(do {
		mog_http_resp(http, "500 Internal Server Error", true);
	} while(0));
}

enum mog_next mog_http_get_in_progress(struct mog_fd *mfd)
{
	struct mog_http *http = &mfd->as.http;
	struct mog_fd *file_mfd;
	struct mog_file *file;
	ssize_t w;
	off_t count;
	static const off_t max_sendfile = 1024 * 1024 * 100;

	assert(http->wbuf == NULL && "can't serve file with http->wbuf");
	assert(http->forward && http->forward != MOG_IOSTAT && "bad forward");
	file_mfd = http->forward;
	file = &file_mfd->as.file;

	assert(file->fsize >= 0 && "fsize is negative");
	assert(file->foff >= 0 && "foff is negative");
	count = file->fsize - file->foff;
	count = count > max_sendfile ? max_sendfile : count;
	if (count == 0)
		goto done;
retry:
	w = sendfile(mfd->fd, file_mfd->fd, &file->foff, (size_t)count);
	if (w > 0) {
		if (file->foff == file->fsize) goto done;
		return MOG_NEXT_ACTIVE;
	} else if (w < 0) {
		switch (errno) {
		case_EAGAIN: return MOG_NEXT_WAIT_WR;
		case EINTR: goto retry;
		}
		http->persistent = 0;
	} else { /* w == 0 */
		/*
		 * if we can't fulfill the value set by our Content-Length:
		 * header, we must kill the TCP connection
		 */
		http->persistent = 0;
		syslog(LOG_ERR,
		       "sendfile()-d 0 bytes at offset=%lld; file truncated?",
		       (long long)file->foff);
	}
done:
	mog_file_close(http->forward);
	if (http->persistent) {
		mog_http_reset(http);
		return MOG_NEXT_ACTIVE;
	}
	return MOG_NEXT_CLOSE;
}
