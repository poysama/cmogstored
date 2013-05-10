/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
void mog_mnt_refresh(void);
const struct mount_entry * mog_mnt_acquire(dev_t);
void mog_mnt_release(const struct mount_entry *);
void mog_mnt_update_util(struct mog_iostat *);
char *mog_mnt_fetch_util(dev_t st_dev, char dst[MOG_IOUTIL_LEN]);

/* mnt_usable.c */
bool mog_mnt_usable(struct mount_entry *me);
