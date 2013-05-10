/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 *
 * File descriptor-based memory allocation.  We have a fixed slot of
 * 128 bytes for every file descriptor.  Once a file descriptor is
 * allocated by the OS, we use mog_fd_init()/mog_fd_get() to reserve
 * userspace memory for that FD.  We release that memory by calling
 * close(2) (via mog_close() wrapper) in mog_fd_put().
 *
 * mog_fd_get() is a simple offset lookup based on the file
 * descriptor, so the "allocation" is simple.
 *
 * This memory is never returned to the kernel, but is bounded by
 * the file descriptor limit (RLIMIT_NOFILE ("ulimit -n")) of the
 * process.  Allowing 20000 file descriptors will only use 2.5 MB
 * of userspace memory.
 *
 * Any sane OS will try to keep file descriptors low and reuse
 * low-numbered descriptors as they become available, reducing
 * fragmentation from unused slots.  We allocate aligned memory
 * to 128 bytes (matching slot size).
 *
 * 128-byte alignment and slot size are used since it:
 * a) is enough to hold per-client data in common cases without malloc()
 * b) easy to align with cache line sizes of modern (200x-201x) CPUs,
 *    avoiding unnecessary cache flushing
 *
 * This 128-byte alignment will need to be expanded to 256 bytes when
 * 128-bit general purpose CPUs become available.
 */
#include "cmogstored.h"
#define FD_PAD_SIZE ((size_t)128)
verify(sizeof(struct mog_fd) <= FD_PAD_SIZE);
static int max_fd;
static size_t fd_heaps;
static const size_t FD_PER_HEAP = 256;
static unsigned char **fd_map;
static pthread_mutex_t fd_lock = PTHREAD_MUTEX_INITIALIZER;
size_t mog_nr_active_at_quit;

static inline struct mog_fd *aref(size_t fd)
{
	unsigned char *base = fd_map[fd / FD_PER_HEAP];

	return (struct mog_fd *)(base + (fd % FD_PER_HEAP) * FD_PAD_SIZE);
}

/* only for pedantic correctness, only one thread is running here */
static void destroy_spinlocks(void)
{
	int fd;
	struct mog_fd *mfd;

	for (fd = 0; fd < max_fd; fd++) {
		mfd = aref(fd);
		CHECK(int, 0, pthread_spin_destroy(&mfd->expiring));
	}
}

static void fd_map_atexit(void)
{
	destroy_spinlocks();

	while (fd_heaps-- > 0)
		free(fd_map[fd_heaps]);
	free(fd_map);
}

static void fd_map_init(void)
{
	long open_max = sysconf(_SC_OPEN_MAX);
	size_t slots = open_max / FD_PER_HEAP + 1;
	size_t size = slots * sizeof(void *);

	assert(fd_map == NULL && "fd_map reinitialized?");
	fd_map = mog_cachealign(size);
	atexit(fd_map_atexit);
}

MOG_NOINLINE static struct mog_fd * grow_ref(size_t fd)
{
	int fd_max;

	assert(fd < INT_MAX && "fd too large");
	CHECK(int, 0, pthread_mutex_lock(&fd_lock));

	if (!fd_map) fd_map_init();
	while (fd >= (size_t)(fd_max = mog_sync_fetch(&max_fd))) {
		unsigned char *base = mog_cachealign(FD_PAD_SIZE * FD_PER_HEAP);
		struct mog_fd *tmp;
		size_t i;
		int rc;

		for (i = 0; i < FD_PER_HEAP; i++) {
			tmp = (struct mog_fd *)(base + (i * FD_PAD_SIZE));
			tmp->fd_type = MOG_FD_TYPE_UNUSED;

			rc = pthread_spin_init(&tmp->expiring, 0);
			if (rc != 0)
				die_errno("pthread_spin_init() failed");
			tmp->fd = fd_max + i;
		}

		fd_map[fd_heaps++] = base;
		(void)mog_sync_add_and_fetch(&max_fd, FD_PER_HEAP);
	}

	CHECK(int, 0, pthread_mutex_unlock(&fd_lock));

	return aref(fd);
}

/*
 * Look up a mog_fd structure based on fd.  This means memory is reused
 * by us just as FDs are reused by the kernel.
 */
struct mog_fd *mog_fd_get(int fd)
{
	assert(fd >= 0 && "FD is negative");
	if (MOG_LIKELY(fd < mog_sync_fetch(&max_fd)))
		return aref((size_t)fd);

	return grow_ref(fd);
}

static inline bool mfd_expiring_trylock(struct mog_fd *mfd)
{
	int rc = pthread_spin_trylock(&mfd->expiring);

	if (MOG_LIKELY(rc == 0))
		return true;
	assert(rc == EBUSY && "pthread_spin_trylock error");
	return false;
}

static inline void mfd_expiring_lock(struct mog_fd *mfd)
{
	CHECK(int, 0, pthread_spin_lock(&mfd->expiring));
}

static inline void mfd_expiring_unlock(struct mog_fd *mfd)
{
	CHECK(int, 0, pthread_spin_unlock(&mfd->expiring));
}

/*
 * Releases the memory used by mfd and releases the file descriptor
 * back to the OS.  mfd is unusable after this.
 */
void mog_fd_put(struct mog_fd *mfd)
{
	int fd = mfd->fd;

	assert(fd >= 0 && "FD is negative");
	assert(fd < mog_sync_fetch(&max_fd) && "FD too small");
	assert(aref(fd) == mfd && "tried to put incorrect mog_fd back in");

	mfd_expiring_lock(mfd);
	mfd->fd_type = MOG_FD_TYPE_UNUSED;
	mfd_expiring_unlock(mfd);
	mog_close(fd);
	/* mog_fd_get(fd) may be called here in another thread */
}

/* called during shutdown, no other threads are running when this is called */
void mog_fdmap_requeue(struct mog_queue *quit_queue)
{
	int fd;
	struct mog_fd *mfd;

	for (fd = max_fd - 1; fd >= 0; fd--) {
		mfd = aref(fd);
		switch (mfd->fd_type) {
		case MOG_FD_TYPE_MGMT:
			/* ignore fsck priority in shutdown: */
			mfd->as.mgmt.prio = MOG_PRIO_NONE;
			/* fall-through: */
		case MOG_FD_TYPE_HTTP:
		case MOG_FD_TYPE_HTTPGET:
			mog_activeq_add(quit_queue, mfd);
			mog_nr_active_at_quit++;
		default:
			break;
		}
	}
}

struct mog_fd * mog_fd_init(int fd, enum mog_fd_type fd_type)
{
	struct mog_fd *mfd = mog_fd_get(fd);

	assert(mfd->fd == fd && "mfd->fd incorrect");
	mfd_expiring_lock(mfd);
	mfd->fd_type = fd_type;
	mfd_expiring_unlock(mfd);

	return mfd;
}

#ifndef __linux__
/* ugh, FreeBSD implements TCP_INFO but doesn't expose the fields we need */
size_t mog_fdmap_expire(uint32_t sec)
{
	return 0;
}
#else /* Linux TCP_INFO tracks last_data_{sent,recv} */

static bool tcp_timedout(struct tcp_info *info, uint32_t msec)
{
	bool send_timedout = !!(info->tcpi_last_data_sent > msec);

	/*
	 * tcpi_last_data_recv is not valid unless
	 * tcpi_ato (ACK timeout) is set
	 */
	if (info->tcpi_ato == 0)
		return send_timedout && (info->tcpi_last_ack_recv > msec);

	return send_timedout && (info->tcpi_last_data_recv > msec);
}

static size_t expire_http(struct mog_fd *mfd, uint32_t msec)
{
	struct tcp_info info;
	socklen_t len = (socklen_t)sizeof(struct tcp_info);

	if (getsockopt(mfd->fd, IPPROTO_TCP, TCP_INFO, &info, &len) == 0) {
		if (info.tcpi_state == TCP_ESTABLISHED &&
		    tcp_timedout(&info, msec)) {
			if (shutdown(mfd->fd, SHUT_RDWR) == 0)
				return 1;

			syslog(LOG_WARNING, "BUG? expire_http,shutdown: %m");
		}
	} else {
		assert(errno != EINVAL && "BUG: getsockopt: EINVAL");
		assert(errno != EFAULT && "BUG: getsockopt: EFAULT");
		syslog(LOG_WARNING, "BUG? expire_http,getsockopt: %m");
	}

	return 0;
}

size_t mog_fdmap_expire(uint32_t sec)
{
	int fd;
	struct mog_fd *mfd;
	size_t expired = 0;
	uint32_t msec = sec * 1000;
	static time_t last_expire;
	time_t now;
	int rc = pthread_mutex_trylock(&fd_lock);

	if (rc != 0) {
		assert(rc == EBUSY && "pthread_mutex_trylock failed" && rc);

		/* sleep on the lock, another thread already doing work */
		CHECK(int, 0, pthread_mutex_lock(&fd_lock));
		CHECK(int, 0, pthread_mutex_unlock(&fd_lock));
		goto out;
	}

	now = time(NULL);
	if (now == last_expire)
		goto out_unlock;

	/* skip stdin, stdout, stderr */
	for (fd = 3; fd < max_fd; fd++) {
		mfd = aref(fd);

		/* bail if another thread just locked it (for close) */
		if (mfd_expiring_trylock(mfd)) {
			switch (mfd->fd_type) {
			case MOG_FD_TYPE_HTTP:
			case MOG_FD_TYPE_HTTPGET:
				expired += expire_http(mfd, msec);
			default:
				mfd_expiring_unlock(mfd);
				break;
			}
		}
	}

	now = time(NULL);
	if (expired > 0 || last_expire != now)
		syslog(LOG_NOTICE, "expired %llu idle connections (>%u sec)",
		       (unsigned long long)expired, (unsigned)sec);
	last_expire = now;

out_unlock:
	CHECK(int, 0, pthread_mutex_unlock(&fd_lock));
out:
	/*
	 * let other threads:
	 * 1) wake up from epoll_wait()
	 * 2) attempt to read/write
	 * 3) hit error
	 * 4) close sockets.
	 */
	for (fd = (int)expired * 8; --fd >= 0; )
		sched_yield();

	return expired;
}
#endif /* Linux-only */
