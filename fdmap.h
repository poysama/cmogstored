/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */

static inline struct mog_fd * mog_fd_of(void *as_obj)
{
	uintptr_t as_addr = (uintptr_t)as_obj;

	return (struct mog_fd *)(as_addr - offsetof(struct mog_fd, as));
}


/* used to validate a mog_fd is never in two queues at once */
static inline void mog_fd_check_in(struct mog_fd *mfd)
{
	/* currently unused */
}

/* used to validate a mog_fd is never in two queues at once */
static inline void mog_fd_check_out(struct mog_fd *mfd)
{
	/* currently unused */
}

struct mog_fd * mog_fd_init(int fd, enum mog_fd_type fd_type);
