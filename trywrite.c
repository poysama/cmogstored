/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

struct mog_wbuf {
	size_t len;
	size_t off;
	unsigned char buf[FLEXIBLE_ARRAY_MEMBER];
};

static void * wbuf_newv(size_t total, struct iovec *iov, int iovcnt)
{
	struct mog_wbuf *wbuf = xmalloc(sizeof(struct mog_wbuf) + total);
	void *dst = wbuf->buf;
	int i;

	wbuf->len = total;
	wbuf->off = 0;

	for (i = 0; i < iovcnt; i++)
		dst = mempcpy(dst, iov[i].iov_base, iov[i].iov_len);

	return wbuf;
}

static void * wbuf_new(void *buf, size_t len)
{
	struct iovec iov;

	iov.iov_base = buf;
	iov.iov_len = len;

	return wbuf_newv(len, &iov, 1);
}

enum mog_write_state mog_tryflush(int fd, struct mog_wbuf **x)
{
	struct mog_wbuf *wbuf = *x;
	unsigned char *ptr = wbuf->buf + wbuf->off;
	size_t len = wbuf->len - wbuf->off;

	for (;;) {
		ssize_t w = write(fd, ptr, len);

		if (w == len) {
			mog_free_and_null(x);
			return MOG_WRSTATE_DONE;
		}
		if (w >= 0) {
			wbuf->off += w;
			ptr += w;
			len -= w;

			continue;
		}

		assert(w < 0 && "no error from write(2)");

		switch (errno) {
		case_EAGAIN: return MOG_WRSTATE_BUSY;
		case EINTR: continue;
		}

		mog_free_and_null(x);
		return MOG_WRSTATE_ERR;
	}
}

/*
 * returns
 * - NULL on full write
 * - MOG_WR_ERROR on error (and sets errno)
 * - address to a new mog_wbuf with unbuffered contents on partial write
 */
void * mog_trywritev(int fd, struct iovec *iov, int iovcnt)
{
	ssize_t total = 0;
	ssize_t w;
	int i;

	for (i = 0; i < iovcnt; i++)
		total += iov[i].iov_len;

	if (total == 0)
		return NULL;
retry:
	w = writev(fd, iov, iovcnt);

	if (w == total) {
		return NULL;
	} else if (w < 0) {
		switch (errno) {
		case_EAGAIN: return wbuf_newv(total, iov, iovcnt);
		case EINTR: goto retry;
		}
		return MOG_WR_ERROR;
	} else {
		struct iovec *new_iov = iov;

		total -= w;

		 /* skip over iovecs we've already written completely */
		for (i = 0; i < iovcnt; i++, new_iov++) {
			if (w == 0)
				break;
			/*
			 * partially written iovec,
			 * modify and retry with current iovec in front
			 */
			if (new_iov->iov_len > (size_t)w) {
				unsigned char *base = new_iov->iov_base;

				new_iov->iov_len -= w;
				base += w;
				new_iov->iov_base = (void *)base;
				break;
			}

			w -= new_iov->iov_len;
		}

		/* retry without the already-written iovecs */
		iovcnt -= i;
		iov = new_iov;
		goto retry;
	}
}

/*
 * returns
 * - NULL on full write
 * - MOG_WR_ERROR on error (and sets errno)
 * - address to a new mog_wbuf with unbuffered contents on partial write
 */
void * mog_trysend(int fd, void *buf, size_t len, off_t more)
{
	if (MOG_MSG_MORE) {
		int flags = more > 0 ? MOG_MSG_MORE : 0;

		while (len > 0) {
			ssize_t w = send(fd, buf, len, flags);

			if (w == (ssize_t)len)
				return NULL; /* all done */

			if (w < 0) {
				switch (errno) {
				case_EAGAIN: return wbuf_new(buf, len);
				case EINTR: continue;
				}
				return MOG_WR_ERROR;
			} else {
				buf = (char *)buf + w;
				len -= w;
			}
		}

		return NULL;
	} else {
		struct iovec iov;

		iov.iov_base = buf;
		iov.iov_len = len;

		return mog_trywritev(fd, &iov, 1);
	}
}
