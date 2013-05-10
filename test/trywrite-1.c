/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "check.h"
#include "iov_str.h"

static void writev_simple(void)
{
	struct iovec iov[1];
	int fds[2];

	pipe_or_die(fds);

	IOV_STR(iov, "HELLO");
	{
		void *x = mog_trywritev(fds[1], iov, 1);

		assert(x == NULL && "couldn't write 5 bytes to pipe");
	}

	close_pipe(fds);
}

static void write_wrong(void)
{
	struct iovec iov[1];
	int fds[2];

	pipe_or_die(fds);

	IOV_STR(iov, "HELLO");
	{
		int null = open("/dev/null", O_RDONLY);
		void *x;

		assert(null >= 0 && "invalid FD opening /dev/nulL");
		x = mog_trywritev(null, iov, 1);
		assert(x == MOG_WR_ERROR && "did not return error");
		close(null);
	}

	{
		void *x = mog_trysend(fds[0], (void *)1, 3, 0);

		assert(x == MOG_WR_ERROR && "bad addr did not return error");
	}

	close_pipe(fds);

	{
		void *x = mog_trywritev(fds[1], iov, 1);

		assert(x == MOG_WR_ERROR && "did not return error");
	}

	{
		char buf[3];
		void *x = mog_trysend(fds[1], buf, 3, 0);

		assert(x == MOG_WR_ERROR && "bad FD did not return error");
	}
}

static void writev_buffer(void)
{
	struct iovec iov[1];
	int fds[2];
	ssize_t total = 0;
	void *x;
	struct mog_wbuf *wbuf = NULL;
	int nread;
	enum mog_write_state state;

	pipe_or_die(fds);

	IOV_STR(iov, "HELLO");

	mog_set_nonblocking(fds[1], true);

	for (;;) {
		x = mog_trywritev(fds[1], iov, 1);

		if (x == NULL) {
			total += iov[0].iov_len = 5;
		} else if (x == MOG_WR_ERROR) {
			assert(0 && "unexpected MOG_WR_ERROR");
		} else {
			assert(x && "wtf");
			wbuf = x;
			break;
		}
	}

	assert(wbuf && "wbuf not initialized, how did we break from loop?");

	/* pipe is blocked */
	{
		ioctl(fds[0], FIONREAD, &nread);
		assert(nread == total && "nread != total");
	}

	/* pipe should be busy */
	{
		state = mog_tryflush(fds[1], &wbuf);
		assert(MOG_WRSTATE_BUSY == state && "not busy?");
		assert(wbuf && "wbuf got nulled");
	}

	/* drain some */
	{
		char * tmp = xmalloc(nread);
		ssize_t r = read(fds[0], tmp, nread);

		assert(r == nread && "couldn't drain all");

		free(tmp);
	}

	/* flush (succeed) */
	{
		state = mog_tryflush(fds[1], &wbuf);
		assert(MOG_WRSTATE_DONE == state && "didn't finish");
		assert(wbuf == NULL && "didn't null");
	}

	close_pipe(fds);
}

static void trysend_buffer(void)
{
	int fds[2];
	ssize_t total = 0;
	void *x;
	struct mog_wbuf *wbuf = NULL;
	int nread;
	char buf[5];
	size_t len = sizeof(buf);

	socketpair_or_die(fds);
	mog_set_nonblocking(fds[1], true);

	for (;;) {
		x = mog_trysend(fds[1], buf, len, 0);

		if (x == NULL) {
			total += len;
		} else if (x == MOG_WR_ERROR) {
			assert(0 && "unexpected MOG_WR_ERROR");
		} else {
			assert(x && "wtf");
			wbuf = x;
			break;
		}
	}

	assert(wbuf && "wbuf not initialized, how did we break from loop?");

	/* socket is/was blocked, it should be readable */
	{
		ioctl(fds[0], FIONREAD, &nread);
		assert(nread > 0 && "nread non-zero");
	}

	/* ignore errors when closing sockets, BSDs can return weird values */
	close(fds[1]);
	close(fds[0]);
}

static void writev_big(size_t len)
{
	struct iovec iov[3];
	void *ptrv[3];
	int fds[2];
	void *x;
	int i;

	pipe_or_die(fds);

	for (i = 0; i < 3; i++) {
		ptrv[i] = iov[i].iov_base = xmalloc(len);
		iov[i].iov_len = len;
		memset(ptrv[i], 'a' + i, len);
	}

	mog_set_nonblocking(fds[1], true);
	x = mog_trywritev(fds[1], iov, 3);
	assert(x != MOG_WR_ERROR && x != NULL && "did not buffer on busy");
	free(x);

	close_pipe(fds);

	for (i = 0; i < 3; i++)
		free(ptrv[i]);
}

int main(void)
{
	writev_simple();
	write_wrong();
	writev_buffer();
	writev_big(166331);
	writev_big(65536);
	trysend_buffer();

	return 0;
}
