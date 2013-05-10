/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
static Hash_table *listeners; /* yes, we'll scale to 10K listen sockets, L10K! */

struct listener {
	union mog_sockaddr as;
	socklen_t len;
	int fd;
};

static bool listener_cmp(const void *a, const void *b)
{
	const struct listener *la = a;
	const struct listener *lb = b;

	return (la->len == lb->len) &&
	       (memcmp(&la->as.sa, &lb->as.sa, lb->len) == 0);
}

static size_t listener_hash(const void *x, size_t tablesize)
{
	const struct listener *l = x;
	size_t value = 0;
	socklen_t i;

	for (i = 0; i < l->len; i++)
		value = (value * 31 + l->as.bytes[i]) % tablesize;

	return value;
}

static void register_listen_fd(int fd)
{
	struct listener tmp;
	struct listener *ins;
	int flags = NI_NUMERICHOST | NI_NUMERICSERV;
	char hbuf[MOG_NI_MAXHOST];
	char sbuf[MOG_NI_MAXSERV];
	int rc;

	tmp.len = (socklen_t)sizeof(tmp.as);
	if (getsockname(fd, &tmp.as.sa, &tmp.len) != 0)
		die_errno("getsockname(fd=%d) failed", fd);

	rc = getnameinfo(&tmp.as.sa, tmp.len,
	                 hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), flags);
	if (rc != 0)
		die("getnameinfo failed: %s", gai_strerror(rc));

	syslog(LOG_INFO, "inherited %s:%s on fd=%d", hbuf, sbuf, fd);

	ins = xmalloc(sizeof(*ins));
	*ins = tmp;
	ins->fd = fd;

	switch (hash_insert_if_absent(listeners, ins, NULL)) {
	case 0:
		die("duplicate listener %s:%s on fd=%d", hbuf, sbuf, fd);
		break;
	case 1: /* success */
		return;
	default:
		mog_oom();
	}
}

static void listeners_cleanup(void)
{
	if (!listeners)
		return;
	hash_free(listeners);
	listeners = NULL;
}

static bool listener_close_each(void *_l, void *unused)
{
	struct listener *l = _l;

	syslog(LOG_INFO, "closing unused inherited fd=%d", l->fd);
	mog_close(l->fd);
	l->fd = -1;

	return true;
}

/* close all inherited listeners we do not need */
void mog_inherit_cleanup(void)
{
	if (!listeners)
		return;

	hash_do_for_each(listeners, listener_close_each, NULL);
	listeners_cleanup();
}

/* returns the FD belonging to the address if it was inherited */
int mog_inherit_get(struct sockaddr *addr, socklen_t len)
{
	struct listener l;
	struct listener *in;
	int fd = -1;

	if (!listeners)
		return fd;

	l.len = len;
	memcpy(&l.as.sa, addr, len);

	in = hash_delete(listeners, &l);
	if (in) {
		fd = in->fd;
		free(in);
		CHECK(int, 0, mog_set_cloexec(fd, true));
	}

	return fd;
}

void mog_inherit_init(void)
{
	char *orig = getenv("CMOGSTORED_FD");
	char *fds;
	char *tip;
	char *end;
	unsigned long fd;
	unsigned endbyte;

	if (orig == NULL)
		return;

	listeners = hash_initialize(3, NULL, listener_hash, listener_cmp, free);
	if (!listeners)
		die("failed to initialize inherited listeners hash");
	atexit(listeners_cleanup);

	fds = xstrdup(orig);
	tip = fds;

	for (;;) {
		errno = 0;
		fd = strtoul(tip, &end, 10);
		if (errno == 0) {
			if (fd > INT_MAX)
				die("inherited fd=%lu is too large", fd);
			register_listen_fd((int)fd);
		} else {
			die_errno("strtuol failed to parse: %s", tip);
		}

		/* the end (or error) */
		endbyte = *end;
		if (endbyte != ',')
			break;

		tip = end + 1; /* more FDs to parse */
	}

	free(fds);

	if (endbyte != '\0')
		die("CMOGSTORED_FD contained invalid byte: %u", endbyte);
}
