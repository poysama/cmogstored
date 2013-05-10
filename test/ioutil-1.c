/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "check.h"

int main(void)
{
	char tmp[MOG_IOUTIL_LEN];
	dev_t i;

	/* just ensure we don't segfault */
	mog_iou_active(1);
	mog_iou_active(2);

	/* we should get back what we wrote */
	memcpy(tmp, "6.66", 5);
	mog_iou_write(3, tmp);
	mog_iou_active(3);
	memset(tmp, 0, sizeof(tmp));
	mog_iou_read(3, tmp);
	assert(strcmp(tmp, "6.66") == 0);

	/* cleanup should've clobbered us */
	mog_iou_cleanup_begin();
	mog_iou_cleanup_finish();
	memset(tmp, 0, sizeof(tmp));
	mog_iou_read(3, tmp);
	assert(strcmp(tmp, "-") == 0);

	/* cleanup should not clobber active entries */
	memcpy(tmp, "6.66", 5);
	mog_iou_write(3, tmp);
	mog_iou_cleanup_begin();
	mog_iou_active(3);
	mog_iou_cleanup_finish();
	mog_iou_read(3, tmp);
	assert(strcmp(tmp, "6.66") == 0);

	/* create many devices and cleanup all of them */
	for (i = 0; i < 100; i++)
		mog_iou_active(i);
	mog_iou_cleanup_begin();
	mog_iou_active(3);
	mog_iou_cleanup_finish();
	mog_iou_read(3, tmp);
	assert(strcmp(tmp, "6.66") == 0);
	for (i = 0; i < 100; i++) {
		if (i == 3)
			continue;
		mog_iou_read(i, tmp);
		assert(strcmp(tmp, "-") == 0);
	}

	return 0;
}
