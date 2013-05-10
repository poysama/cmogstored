/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

/*
 * mog_listen_parse and mog_cfg_parse generate mog_addrinfo objects internally
 */
void mog_addrinfo_free(struct mog_addrinfo **aptr)
{
	struct mog_addrinfo *a = *aptr;

	if (!a) return;

	*aptr = NULL;
	mog_free(a->orig);
	freeaddrinfo(a->addr);
	free(a);
}
