/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
static void iov_str(struct iovec *iov, const char *str, size_t len)
{
	union { const char *in; char *out; } deconst;

	deconst.in = str;
	iov->iov_base = deconst.out;
	iov->iov_len = len;
}

#define IOV_STR(iov,str) iov_str((iov),(str),sizeof(str)-1)
