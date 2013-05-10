/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "mgmt.h"
#include "iov_str.h"
#include "digest.h"

static char *
get_path(struct iovec *dst, struct mog_mgmt *mgmt, char *buf, bool sdup)
{
	dst->iov_base = buf + mgmt->mark[0];
	dst->iov_len = mgmt->mark[1] - mgmt->mark[0];

	if (mog_valid_path(dst->iov_base, dst->iov_len)) {
		char *path;

		if (sdup) {
			path = xmalloc(dst->iov_len + 1);
			memcpy(path, dst->iov_base, dst->iov_len);
		} else {
			path = dst->iov_base;
		}

		path[dst->iov_len] = '\0';
		return path;
	} else {
		struct iovec iov;

		IOV_STR(&iov, "ERROR: uri invalid (contains ..)\r\n");
		mog_mgmt_writev(mgmt, &iov, 1);
		return NULL;
	}
}

/* starts the MD5 request */
void mog_mgmt_fn_digest(struct mog_mgmt *mgmt, char *buf)
{
	struct iovec iov[2];
	char *path = get_path(iov, mgmt, buf, true);

	if (!path) return;

	mgmt->forward = mog_file_open_read(mgmt->svc, path);
	if (mgmt->forward) {
		struct mog_file *file = &mgmt->forward->as.file;

		mog_fadv_noreuse(mgmt->forward->fd, 0, 0 /* ALL */);
		mog_fadv_sequential(mgmt->forward->fd, 0, 0 /* ALL */);
		mog_digest_init(&file->digest, mgmt->alg);
		file->pathlen = iov[0].iov_len;
		file->path = path;
	} else {
		switch (mgmt->alg) {
		case GC_MD5:
			IOV_STR(&iov[1], " MD5=-1\r\n");
			break;
		case GC_SHA1:
			IOV_STR(&iov[1], " SHA-1=-1\r\n");
			break;
		default: /* Ragel parser prevents this: */
			die("BUG: unhandled algorithm: %d", mgmt->alg);
		}
		mog_mgmt_writev(mgmt, iov, 2);
		free(path);
	}
}

/* finishes the MD5 request */
#define CLEN(s) (sizeof(s)-1)

void mog_mgmt_fn_digest_err(struct mog_mgmt *mgmt)
{
	struct iovec iov[3];
	struct mog_fd *mfd = mgmt->forward;
	struct mog_file *file = &mfd->as.file;
	long long offset = (long long)lseek(mfd->fd, 0, SEEK_CUR);
	char buf[sizeof(" at 18446744073709551615 failed\r\n") - 1];

	/* offset could be -1 here, but there ain't much we can do */

	IOV_STR(iov, "ERR read ");
	iov[1].iov_base = file->path;
	iov[1].iov_len = file->pathlen;
	iov[2].iov_base = buf;
	iov[2].iov_len = snprintf(buf, sizeof(buf),
	                          " at %lld failed\r\n", offset);
	mog_mgmt_writev(mgmt, iov, 3);
}

/* output: "/$PATH MD5=hex\r\n" */
void mog_mgmt_fn_digest_emit(struct mog_mgmt *mgmt)
{
	struct iovec iov[2];
	char buf[CLEN(" SHA-1=") + 40 + CLEN("\r\n")];
	char *b = buf;
	struct mog_fd *mfd = mgmt->forward;
	struct mog_file *file = &mfd->as.file;
	size_t len;

	iov[0].iov_base = file->path;
	iov[0].iov_len = file->pathlen;

	/* ugh, clean this up... */
	switch (mgmt->alg) {
	case GC_MD5:
		b = mempcpy(b, " MD5=", CLEN(" MD5="));
		len = 32;
		iov[1].iov_len = CLEN(" MD5=") + 32 + CLEN("\r\n");
		break;
	case GC_SHA1:
		b = mempcpy(b, " SHA-1=", CLEN(" SHA-1="));
		len = 40;
		iov[1].iov_len = sizeof(buf);
		break;
	default:
		die("BUG: unhandled algorithm: %d", mgmt->alg);
	}

	mog_digest_hex(&file->digest, b, len);
	b[len] = '\r';
	b[len + 1] = '\n';
	iov[1].iov_base = buf;
	mog_mgmt_writev(mgmt, iov, 2);
}

/*
 * writes to mgmt fd:
 *   "URI $SIZE\r\n" on success
 *   "URI -1\r\n" on failure
 *   "ERROR: uri invalid (contains ..)\r\n" on invalid paths
 */
void mog_mgmt_fn_size(struct mog_mgmt *mgmt, char *buf)
{
	struct stat sb;
	struct iovec iov[2];
	char tmp[sizeof(" 18446744073709551615\r\n") - 1];
	char *path = get_path(iov, mgmt, buf, false);

	if (!path) return;

	if (mog_stat(mgmt->svc, path, &sb) == 0) {
		long long size = (long long)sb.st_size;

		iov[1].iov_base = tmp;
		iov[1].iov_len = snprintf(tmp, sizeof(tmp), " %lld\r\n", size);
	} else {
		IOV_STR(&iov[1], " -1\r\n");
	}

	mog_mgmt_writev(mgmt, iov, 2);
}

void mog_mgmt_fn_blank(struct mog_mgmt *mgmt)
{
	struct iovec iov;

	IOV_STR(&iov, "\r\n");
	mog_mgmt_writev(mgmt, &iov, 1);
}

void mog_mgmt_fn_unknown(struct mog_mgmt *mgmt, char *buf)
{
	struct iovec iov[3];

	IOV_STR(&iov[0], "ERROR: unknown command: ");
	iov[1].iov_base = mgmt->mark[0] + buf;
	iov[1].iov_len = mgmt->mark[1] - mgmt->mark[0];
	IOV_STR(&iov[2], "\r\n");
	mog_mgmt_writev(mgmt, iov, 3);
}

void mog_mgmt_fn_watch_err(struct mog_mgmt *mgmt)
{
	struct iovec iov;

	IOV_STR(&iov, "ERR iostat unavailable\r\n");
	mog_mgmt_writev(mgmt, &iov, 1);
}

void mog_mgmt_fn_aio_threads(struct mog_mgmt *mgmt, char *buf)
{
	char *end;
	unsigned long long nr;
	struct mog_queue *q = mgmt->svc->queue;
	char *nptr = buf + mgmt->mark[0];
	char *eor = nptr + mgmt->mark[1] - mgmt->mark[0];
	struct iovec iov;

	assert((*eor == '\n' || *eor == '\r') && "missing end-of-record");
	*eor = 0;

	nr = strtoull(nptr, &end, 10);
	assert(*end == 0 && "ragel misfed mog_mgmt_fn_set_aio_threads");

	if (nr > 0 && nr <= 100)
		mog_thrpool_set_n_threads(q, nr);

	IOV_STR(&iov, "\r\n");
	mog_mgmt_writev(mgmt, &iov, 1);
}
