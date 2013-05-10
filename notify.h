/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
enum mog_notification {
	MOG_NOTIFY_SIGNAL = -1,
	MOG_NOTIFY_DEVICE_REFRESH = 0,
	MOG_NOTIFY_SET_N_THREADS = 1,
	MOG_NOTIFY_MAX
};

extern struct mog_queue *mog_notify_queue;
