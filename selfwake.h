/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#if (defined(HAVE_EPOLL_WAIT) && defined(HAVE_PPOLL)) \
    || defined(HAVE_EPOLL_PWAIT)
# define MOG_SELFPIPE 0
#else
# define MOG_SELFPIPE 1
#endif

struct mog_selfwake {
	struct mog_queue *queue;
	struct mog_fd *writer;
};

/* only for pipe */
struct mog_selfpipe {
	struct mog_fd *reader; /* points to mog_selfwake */
};

#if MOG_SELFPIPE
struct mog_fd * mog_selfwake_new(void);
void mog_selfwake_trigger(struct mog_fd *);
void mog_selfwake_drain(struct mog_fd *);
static inline void mog_selfwake_interrupt(void) {}
void mog_selfwake_wait(struct mog_fd *);
#else /* use Linux-only facilities like epoll_pwait or ppoll */
static inline void mog_selfwake_wait(struct mog_fd *mfd)
{
	mog_sleep(-1);
}
static inline struct mog_fd * mog_selfwake_new(void) { return NULL; }
static inline void mog_selfwake_trigger(struct mog_fd *mfd) {}
static inline void mog_selfwake_drain(struct mog_fd *mfd) {}
static inline void mog_selfwake_interrupt(void)
{
	CHECK(int, 0, kill(getpid(), SIGURG));
}
#endif /* Linux-only stuff */
