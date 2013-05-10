/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */

/* stringify +s+ */
#define MOG_STR(s) MOG_STR0(s)
#define MOG_STR0(s) #s

/*
 * some systems define EWOULDBLOCK to a different value than EAGAIN,
 * but POSIX allows them to be identical.
 */
#if defined(EWOULDBLOCK) && (EWOULDBLOCK != EAGAIN)
#  define case_EAGAIN case EAGAIN: case EWOULDBLOCK
#else
#  define case_EAGAIN case EAGAIN
#endif

/* free(3) causes compiler warnings on const, so we de-const here */
static inline void mog_free(const void *ptr)
{
	union { const void *in; void *out; } deconst = { .in = ptr };

	free(deconst.out);
}

#define PRESERVE_ERRNO(code) do { \
	int save_err = errno; \
	code; \
	errno = save_err; \
} while (0)

# define CHECK(type, expect, expr) do { \
	type checkvar = (expr); \
	assert(checkvar==(expect)&& "BUG" && __FILE__ && __LINE__); \
	} while (0)

static inline void mog_cancel_enable(void)
{
	int old;

	CHECK(int, 0, pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old));
	assert(old == PTHREAD_CANCEL_DISABLE && "redundant cancel enable");
}

static inline void mog_cancel_disable(void)
{
	int old;

	CHECK(int, 0, pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old));
	assert(old == PTHREAD_CANCEL_ENABLE && "redundant cancel disable");
}

/* compiler should optimize this away */
__attribute__((const)) static inline off_t off_t_max(void)
{
	return (off_t)(sizeof(long) == sizeof(off_t) ? LONG_MAX : LLONG_MAX);
}

#if defined(HAVE_IOCTL) && defined(FIONBIO)
/*
 * FIONBIO * requires only one syscall for reliable operation rather than
 * two syscalls with fcntl() calls, so use it if possible.
 */
static inline int mog_set_nonblocking(int fd, const bool value)
{
	int flag = value ? 1 : 0;

	return ioctl(fd, FIONBIO, &flag);
}
#else /* use gnulib */
#include "nonblocking.h"
#define mog_set_nonblocking(fd, value) set_nonblocking_flag((fd), (value))
#endif

/*
 * the only FD_* flag that exists as of 2012 is FD_CLOEXEC, if new ones
 * ever get defined, we wouldn't be using them in the first place without
 * updating this code... (no way they'd be on by default).
 */
static inline int mog_set_cloexec(int fd, const bool set)
{
	return fcntl(fd, F_SETFD, set ? FD_CLOEXEC : 0);
}

static inline bool mog_pthread_create_retry(const int err)
{
	/*
	 * older versions of glibc return ENOMEM instead of EAGAIN
	 * ref: http://www.sourceware.org/bugzilla/show_bug.cgi?id=386
	 * Remove the ENOMEM check by 2023 (unless other OSes have this
	 * bug).
	 */
	return (err == EAGAIN || err == ENOMEM);
}
