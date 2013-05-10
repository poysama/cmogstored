/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#define MOG_PRINTF __attribute__((format(printf,1,2)))
#define MOG_LIKELY(x) (__builtin_expect((x), 1))
#define MOG_UNLIKELY(x) (__builtin_expect((x), 0))
#define MOG_NOINLINE __attribute__((noinline))
#define MOG_CHECK __attribute__((warn_unused_result))
#define mog_sync_add_and_fetch(dst,val) __sync_add_and_fetch((dst),(val))
#define mog_sync_sub_and_fetch(dst,val) __sync_sub_and_fetch((dst),(val))

/* need the synchronization, right? */
#define mog_sync_fetch(dst) mog_sync_add_and_fetch((dst),0)
