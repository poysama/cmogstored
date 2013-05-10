/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

/*
 * we can lower this if we can test with lower values, NPTL minimum is 16K.
 * We also use syslog() and *printf() functions which take a lot of
 * stack under glibc, so we'll add BUFSIZ (8192 on glibc) to that
 */
#if MOG_LIBKQUEUE /* libkqueue uses quite a bit of stack */
#  define MOG_THR_STACK_SIZE (0)
#elif defined(__GLIBC__) || defined(__FreeBSD__)
#  define MOG_THR_STACK_SIZE ((16 * 1024) + MAX(8192,BUFSIZ))
#  if defined(PTHREAD_STACK_MIN) && (PTHREAD_STACK_MIN > MOG_THR_STACK_SIZE)
#    undef MOG_THR_STACK_SIZE
#    define MOG_THR_STACK_SIZE PTHREAD_STACK_MIN
#  endif
#else
#  define MOG_THR_STACK_SIZE (0)
#endif
static const size_t stacksize = (size_t)MOG_THR_STACK_SIZE;

static pthread_mutex_t sat_lock = PTHREAD_MUTEX_INITIALIZER;
struct sat_arg;
struct sat_arg {
	struct mog_queue *queue;
	size_t size;
	SIMPLEQ_ENTRY(sat_arg) qentry;
};

static SIMPLEQ_HEAD(sq, sat_arg) satqhead = SIMPLEQ_HEAD_INITIALIZER(satqhead);

/*
 * kevent() sleep is not a cancellation point, so it's possible for
 * a thread to sleep on it if the cancel request arrived right after
 * we checked for cancellation
 */
static void poke(pthread_t thr, int sig)
{
	int err;

	while ((err = pthread_kill(thr, sig)) == 0)
		sched_yield();
	assert(err == ESRCH && "pthread_kill() usage bug");
}

static bool
thr_create_fail_retry(struct mog_thrpool *tp, size_t size,
                      unsigned long *nr_eagain, int err)
{
	/* do not leave the pool w/o threads at all */
	if (tp->n_threads == 0) {
		if ((++*nr_eagain % 1024) == 0) {
			errno = err;
			syslog(LOG_ERR, "pthread_create: %m (tries: %lu)",
			       *nr_eagain);
		}
		sched_yield();
		return true;
	} else {
		errno = err;
		syslog(LOG_ERR,
		       "pthread_create: %m, only running %lu of %lu threads",
		       (unsigned long)tp->n_threads, (unsigned long)size);
		return false;
	}
}

static void thrpool_set_size(struct mog_thrpool *tp, size_t size)
{
	unsigned long nr_eagain = 0;

	CHECK(int, 0, pthread_mutex_lock(&tp->lock));
	while (size > tp->n_threads) {
		pthread_t *thr;
		pthread_attr_t attr;
		size_t bytes = (tp->n_threads + 1) * sizeof(pthread_t);
		int rc;

		tp->threads = xrealloc(tp->threads, bytes);

		CHECK(int, 0, pthread_attr_init(&attr));

		if (stacksize > 0) {
			CHECK(int, 0,
			      pthread_attr_setstacksize(&attr, stacksize));
		}

		thr = tp->threads + tp->n_threads;

		rc = pthread_create(thr, &attr, tp->start_fn, tp->start_arg);
		CHECK(int, 0, pthread_attr_destroy(&attr));

		if (rc == 0) {
			tp->n_threads++;
			nr_eagain = 0;
		} else if (mog_pthread_create_retry(rc)) {
			if (!thr_create_fail_retry(tp, size, &nr_eagain, rc))
				goto out;
		} else {
			assert(rc == 0 && "pthread_create usage error");
		}
	}

	if (tp->n_threads > size) {
		size_t i;
		int err;

		for (i = size; i < tp->n_threads; i++) {
			CHECK(int, 0, pthread_cancel(tp->threads[i]));
			err = pthread_kill(tp->threads[i], SIGURG);

			switch (err) {
			case 0:
			case ESRCH:
				break;
			default:
				assert(0 && "pthread_kill usage bug" && err);
			}
		}

		for (i = size; i < tp->n_threads; i++) {
			poke(tp->threads[i], SIGURG);

			CHECK(int, 0, pthread_join(tp->threads[i], NULL));
		}
		tp->n_threads = size;
	}
out:
	CHECK(int, 0, pthread_mutex_unlock(&tp->lock));
}

/*
 * fire and forget, we must run the actual thread count manipulation
 * in the main notify thread because we may end up terminating the
 * thread which invoked this.
 */
void mog_thrpool_set_n_threads(struct mog_queue *q, size_t size)
{
	struct sat_arg *arg;

	/* this gets free'ed in mog_thrpool_process_queue() */
	arg = xmalloc(sizeof(struct sat_arg));
	arg->size = size;
	arg->queue = q;

	/* put into the queue so main thread can process it */
	CHECK(int, 0, pthread_mutex_lock(&sat_lock));
	SIMPLEQ_INSERT_TAIL(&satqhead, arg, qentry);
	CHECK(int, 0, pthread_mutex_unlock(&sat_lock));

	/* wake up the main thread so it can process the queue */
	mog_notify(MOG_NOTIFY_SET_N_THREADS);
}

/* this runs in the main (notify) thread */
void mog_thrpool_process_queue(void)
{
	/* guard against requests bundled in one wakeup by looping here */
	for (;;) {
		struct sat_arg *arg;

		CHECK(int, 0, pthread_mutex_lock(&sat_lock));
		arg = SIMPLEQ_FIRST(&satqhead);
		if (arg)
			SIMPLEQ_REMOVE_HEAD(&satqhead, qentry);
		CHECK(int, 0, pthread_mutex_unlock(&sat_lock));

		if (arg == NULL)
			return;

		syslog(LOG_INFO, "server aio_threads=%u", (unsigned)arg->size);
		thrpool_set_size(&arg->queue->thrpool, arg->size);
		free(arg);
	}
}

void
mog_thrpool_start(struct mog_thrpool *tp, size_t n,
                  void *(*start_fn)(void *), void *arg)
{
	if (n == 0)
		n = 1;
	tp->threads = NULL;
	tp->n_threads = 0;
	tp->start_fn = start_fn;
	tp->start_arg = arg;
	CHECK(int, 0, pthread_mutex_init(&tp->lock, NULL));
	thrpool_set_size(tp, n);
}

void mog_thrpool_quit(struct mog_thrpool *tp, struct mog_queue *q)
{
	thrpool_set_size(tp, 0);
	CHECK(int, 0, pthread_mutex_destroy(&tp->lock));
	mog_free_and_null(&tp->threads);
}
