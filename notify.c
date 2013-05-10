/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

static int notes[MOG_NOTIFY_MAX];
static struct mog_fd *notify_mfd;
static time_t usage_file_updated_at;
static time_t usage_file_interval = 10;
struct mog_queue *mog_notify_queue;

void mog_notify_init(void)
{
	const char *interval = getenv("MOG_DISK_USAGE_INTERVAL");

	if (interval) {
		int i = atoi(interval);

		if (i > 0)
			usage_file_interval = (time_t)i;
	}

	assert(mog_notify_queue == NULL && "notify queue already initialized");
	assert(notify_mfd == NULL && "notify_mfd already initialized");

	mog_notify_queue = mog_queue_new();
	notify_mfd = mog_selfwake_new();
	if (notify_mfd) {
		struct mog_selfwake *notify = &notify_mfd->as.selfwake;
		assert(notify->writer && "notify writer not initialized");
		notify->queue = mog_notify_queue;
		mog_idleq_add(notify->queue, notify_mfd, MOG_QEV_RD);
	}
}

static void global_mkusage(void)
{
	mog_mkusage_all();
	usage_file_updated_at = time(NULL);
}

static inline bool note_xchg(enum mog_notification note, int from, int to)
{
	return __sync_bool_compare_and_swap(&notes[note], from, to);
}

static void note_run(void)
{
	if (note_xchg(MOG_NOTIFY_DEVICE_REFRESH, 1, 0))
		global_mkusage();

	if (note_xchg(MOG_NOTIFY_SET_N_THREADS, 1, 0))
		mog_thrpool_process_queue();
}

/* drain the pipe and process notifications */
static void note_queue_step(struct mog_fd *mfd)
{
	mog_selfwake_drain(mfd);
	note_run();
	mog_idleq_push(mfd->as.selfwake.queue, mfd, MOG_QEV_RD);
}

static void notify_queue_step(struct mog_fd *mfd)
{
	switch (mfd->fd_type) {
	case MOG_FD_TYPE_SELFWAKE: note_queue_step(mfd); return;
	case MOG_FD_TYPE_IOSTAT: mog_iostat_queue_step(mfd); return;
	default:
		assert(0 && mfd->fd_type && "bad fd_type in queue");
	}
}

/* this is the main loop of cmogstored */
void mog_notify_wait(bool need_usage_file)
{
	time_t next = usage_file_updated_at + usage_file_interval;
	time_t now = time(NULL);
	time_t timeout = next - now;
	struct mog_fd *mfd;

	if (next <= now)
		global_mkusage();

	/*
	 * epoll_wait() with timeout==0 can avoid some slow paths,
	 * so take anything that's already ready before sleeping
	 */
	while ((mfd = mog_idleq_wait(mog_notify_queue, 0)))
		notify_queue_step(mfd);

	if (need_usage_file == false)
		timeout = -1;
	else if (timeout > 0)
		timeout *= 1000;
	else
		timeout = 0;

	mfd = mog_idleq_wait_intr(mog_notify_queue, timeout);
	if (mfd)
		notify_queue_step(mfd);
	else if (errno == EINTR)
		note_run();
}

/* this is async-signal safe */
void mog_notify(enum mog_notification note)
{
	switch (note) {
	case MOG_NOTIFY_DEVICE_REFRESH:
	case MOG_NOTIFY_SET_N_THREADS:
		note_xchg(note, 0, 1);
		mog_selfwake_interrupt();
		break;
	case MOG_NOTIFY_SIGNAL: break;
	default: assert(0 && "bad note passed");
	}
	mog_selfwake_trigger(notify_mfd);
}
