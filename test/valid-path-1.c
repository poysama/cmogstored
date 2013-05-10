/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "check.h"
#define VP(path) mog_valid_path((path), sizeof(path) - 1)
#define VPP(path) mog_valid_put_path((path), sizeof(path) - 1)

int main(void)
{
	assert(0 == VP("hello/.."));
	assert(1 == VP("hello/"));
	assert(1 == VP("hello.fid"));
	assert(0 == VP("../hello.fid"));
	assert(0 == VP("/../hello.fid"));
	assert(0 == VP("/hello.fid/.."));
	assert(0 == VP("/hello/../fid"));

	assert(VPP("/dev123/foo"));
	assert(VPP("/dev123/foo.fid"));
	assert(VPP("/dev123/0/1/2/3/4.fid"));
	assert(VPP("/dev16777215/0/000/000/0123456789.fid"));
	assert(!VPP("/dev123/foo/"));
	assert(!VPP("/dev123//"));
	assert(!VPP("///"));
	assert(!VPP("/wtf//"));
	assert(!VPP("/dev123/foo.fid/"));

	return 0;
}
