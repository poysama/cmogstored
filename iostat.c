/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */

/*
 * iostat(1) -> global mountlist -> each mog_dev in each mog_svc
 */
#include "cmogstored.h"

/* called after a stats line for a single device is complete */
void mog_iostat_line_done(struct mog_iostat *iostat)
{
	if (iostat->dev_tip < sizeof(iostat->dev)) {
		iostat->dev[iostat->dev_tip] = 0;
		if (iostat->util_tip < sizeof(iostat->util)) {
			iostat->util[iostat->util_tip] = 0;
		} else {
			iostat->util[0] = '-';
			iostat->util[1] = 0;
		}

		mog_mnt_update_util(iostat);
	} else {
		syslog(LOG_ERR, "device name from iostat too long");
	}
}

/* called every second, after stats for each device line are out */
void mog_iostat_commit(void)
{
	mog_svc_each(mog_svc_devstats_broadcast, NULL);
}

static void iostat_close(struct mog_fd *mfd)
{
	assert(mfd->fd_type == MOG_FD_TYPE_IOSTAT && "bad fd_type");
	mog_fd_put(mfd);
}

static void iostat_reset(struct mog_iostat *iostat)
{
	bool ready = iostat->ready;
	mog_iostat_init(iostat);
	iostat->ready = ready;
}

void mog_iostat_queue_step(struct mog_fd *mfd)
{
	/*
	 * only one thread ever hits this function at once,
	 * no point in ever running multiple iostat(1) processes
	 */
	static char buf[1024];
	ssize_t r;
	struct mog_iostat *iostat = &mfd->as.iostat;

	assert(mfd->fd >= 0 && mfd->fd_type == MOG_FD_TYPE_IOSTAT &&
	       "bad iostat mfd");
retry:
	r = read(mfd->fd, buf, sizeof(buf));
	if (r > 0) {
		switch (mog_iostat_parse(iostat, buf, r)) {
		case MOG_PARSER_ERROR:
			syslog(LOG_ERR, "iostat parser error");
			iostat_close(mfd);
			return;
		case MOG_PARSER_DONE:
			iostat_reset(iostat);
			goto retry;
		case MOG_PARSER_CONTINUE:
			goto retry;
		}
	} else if (r == 0) {
		/* iostat exits every 30s by default */
		iostat_close(mfd);
	} else {
		switch (errno) {
		case_EAGAIN:
			mog_idleq_push(iostat->queue, mfd, MOG_QEV_RD);
			return;
		case EINTR: goto retry;
		}
		syslog(LOG_ERR, "iostat read() failed: %m");
		iostat_close(mfd);
	}
}
