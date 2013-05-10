/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 *
 * We use thread-local buffers as much as possible.  mog_rbuf may
 * be detached from the thread-local pointer (and grown) if we have
 * requests trickled to us or large requests.  This is unlikely with
 * MogileFS (which only deals with internal LAN traffic), and unlikely
 * even with normal, untrusted HTTP traffic.
 */
#include "cmogstored.h"
#define L1_CACHE_LINE_MAX 128 /* largest I've seen (Pentium 4) */
static size_t l1_cache_line_size = L1_CACHE_LINE_MAX;

static __thread struct mog_rbuf *tls_rbuf; /* for small reads (headers) */
static __thread unsigned char tls_fsbuf[8192]; /* for filesystem I/O */

#define MOG_MASK(align)        (~((size_t)align - 1))
#define MOG_ALIGN(align,val)   (((val) + (size_t)align - 1) & MOG_MASK(align))

static void l1_cache_line_size_detect(void)
{
#ifdef _SC_LEVEL1_DCACHE_LINESIZE
	long tmp = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);

	if (tmp > 0 && tmp <= L1_CACHE_LINE_MAX)
		l1_cache_line_size = (size_t)tmp;
#endif /* _SC_LEVEL1_DCACHE_LINESIZE */
}

void mog_alloc_quit(void)
{
	struct mog_rbuf *rbuf = tls_rbuf;

	tls_rbuf = NULL;

	mog_rbuf_free(rbuf);
}

__attribute__((constructor)) static void alloc_init(void)
{
	l1_cache_line_size_detect();
	atexit(mog_alloc_quit);
}

void mog_free_and_null(void *ptrptr)
{
	void **tmp = ptrptr;

	free(*tmp);
	*tmp = NULL;
}

_Noreturn void mog_oom(void)
{
	write(STDERR_FILENO, "OOM\n", 4);
	syslog(LOG_CRIT, "Out of memory, aborting");
	abort();
}


/*
 * Cache alignment is important for sub-pagesized allocations
 * that can be bounced between threads.  We round up the
 * allocation to the cache size
 */
void *mog_cachealign(size_t size)
{
	void *ptr;
	int err = posix_memalign(&ptr, l1_cache_line_size, size);

	switch (err) {
	case 0: return ptr;
	case ENOMEM: mog_oom();
	}
	die_errno("posix_memalign failed");
}


/* allocates a new mog_rbuf of +size+ bytes */
struct mog_rbuf *mog_rbuf_new(size_t size)
{
	struct mog_rbuf *rbuf;
	size_t bytes = size + sizeof(struct mog_rbuf);

	assert(size > 0 && "tried to allocate a zero-byte mog_rbuf");

	/*
	 * only cache-align for common allocation sizes, for larger
	 * allocations it'll lead to fragmentation and we'll only
	 * end up touching later sections of memory...
	 */
	if (size == MOG_RBUF_BASE_SIZE)
		rbuf = mog_cachealign(bytes);
	else
		rbuf = xmalloc(bytes);
	rbuf->rcapa = size;

	return rbuf;
}

MOG_NOINLINE static struct mog_rbuf *
rbuf_replace(struct mog_rbuf *rbuf, size_t size)
{
	free(rbuf); /* free(NULL) works on modern systems */
	rbuf = mog_rbuf_new(size);
	tls_rbuf = rbuf;

	return rbuf;
}

/*
 * retrieves the per-thread rbuf belonging to the current thread,
 * ensuring it is at least capable of storing the specified size
 */
struct mog_rbuf *mog_rbuf_get(size_t size)
{
	struct mog_rbuf *rbuf = tls_rbuf;

	if (rbuf && rbuf->rcapa >= size) return rbuf;

	return rbuf_replace(rbuf, size);
}

/* ensures a given rbuf is no longer associated with the current thread */
struct mog_rbuf *mog_rbuf_detach(struct mog_rbuf *rbuf)
{
	struct mog_rbuf *cur = tls_rbuf;

	if (cur == rbuf)
		tls_rbuf = NULL;

	return rbuf;
}

/*
 * Behaves similarly to realloc(), but is safe for posix_memalign()
 * Returns a detached rbuf with the contents of +cur+
 * (which may be cur itself)
 * Releases memory and returns NULL if rbuf is too big.
 */
struct mog_rbuf *mog_rbuf_grow(struct mog_rbuf *cur)
{
	struct mog_rbuf *ret;
	size_t new_size = cur->rsize + 500; /* grow by 500 bytes or so */

	if (cur->rsize == MOG_RBUF_MAX_SIZE) {
		assert(cur != tls_rbuf && "TLS rbuf is HUGE");
		free(cur);
		return NULL;
	}
	assert(cur->rsize < MOG_RBUF_MAX_SIZE && "rbuf rsize got too big");

	if (new_size > MOG_RBUF_MAX_SIZE)
		new_size = MOG_RBUF_MAX_SIZE;
	if (cur->rcapa < new_size) {
		if (cur->rcapa == MOG_RBUF_BASE_SIZE) {
			/* can't safely realloc posix_memalign'ed memory */
			ret = mog_rbuf_new(new_size);
			memcpy(ret->rptr, cur->rptr, cur->rsize);
			if (cur != tls_rbuf)
				mog_rbuf_free(cur);
		} else {
			assert(cur != tls_rbuf && "bug rbuf found in TLS");
			ret = xrealloc(cur, new_size + sizeof(struct mog_rbuf));
			ret->rcapa = new_size;
		}
	} else {
		/* this may not even happen, just in case: */
		ret = mog_rbuf_detach(cur);
	}

	return ret;
}

void mog_rbuf_free(struct mog_rbuf *rbuf)
{
	assert(((rbuf == NULL) ||
	       (tls_rbuf != rbuf)) &&
	       "trying to free undetached rbuf");
	free(rbuf);
}

void mog_rbuf_free_and_null(struct mog_rbuf **ptrptr)
{
	mog_rbuf_free(*ptrptr);
	*ptrptr = NULL;
}

/* retrieves the per-thread fsbuf and sets size to the value of fsbuf_size */
void *mog_fsbuf_get(size_t *size)
{
	void *ptr = tls_fsbuf;

	*size = sizeof(tls_fsbuf);

	return ptr;
}
