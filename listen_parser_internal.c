/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "listen_parser.h"

struct mog_addrinfo *
mog_listen_parse_internal(
	char *mark_beg, size_t mark_len, char *port_beg, size_t port_len)
{
	const char *node = NULL;
	struct addrinfo hints;
	struct addrinfo *result = NULL;
	struct mog_addrinfo *mog_addr = NULL;
	int s;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = IPPROTO_TCP;

	if (mark_len) {
		mark_beg[mark_len] = 0;
		node = mark_beg;
	}
	port_beg[port_len] = 0;
	s = getaddrinfo(node, port_beg, &hints, &result);
	if (s != 0)
		syslog(LOG_ERR, "failed to resolve %s:%s - %s",
		       node ? node : "(nil)", port_beg, gai_strerror(s));

	if (result) {
		mog_addr = xmalloc(sizeof(struct mog_addrinfo));
		mog_addr->addr = result;
		if (!node) node = "0.0.0.0";
		mog_addr->orig = xasprintf("%s:%s", node, port_beg);
	}

	return mog_addr;
}
