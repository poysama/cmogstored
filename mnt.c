/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
/*
 * Uses the mountlist library in gnulib to map system device IDs and
 * system device names to mount entries.
 */
#include "cmogstored.h"

struct init_args {
	pthread_mutex_t cond_lock;
	pthread_cond_t cond;
};

static pthread_mutex_t by_dev_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * by_dev maps (system) device IDs to a mount_entry; mount_entry structs may
 * be chained as multiple mount entries may be aliased (e.g. "rootfs" and
 * "/dev/root") on Linux.
 */
static Hash_table *by_dev;

static void me_free(void *entry)
{
	struct mount_entry *next;
	struct mount_entry *me = entry;

	do {
		free(me->me_devname);
		free(me->me_mountdir);
		assert(me->me_type == NULL
		       && me->me_type_malloced == 0
		       && "me_type still malloc-ed in mountlist");
		next = me->me_next;
		free(me);
	} while ((me = next));
}

static size_t me_hash(const void *entry, size_t tablesize)
{
	const struct mount_entry *me = entry;

	return me->me_dev % tablesize;
}

static bool me_cmp(const void *a, const void *b)
{
	const struct mount_entry *me_a = a;
	const struct mount_entry *me_b = b;

	return me_a->me_dev == me_b->me_dev;
}

static void mnt_atexit(void)
{
	hash_free(by_dev);
}

static Hash_table * mnt_new(size_t n)
{
	Hash_table *rv = hash_initialize(n, NULL, me_hash, me_cmp, me_free);

	if (!rv)
		mog_oom();
	return rv;
}

/* populates a hash table starting with the mount list */
static void mnt_populate(Hash_table *tbl)
{
	struct mount_entry *head = read_file_system_list(false);
	struct mount_entry *next;
	union {
		const void *ptr;
		struct mount_entry *old_me;
	} exist;

	for ( ; head; head = next) {
		next = head->me_next;

		/* ensure we can me_free() without side effects when skipping */
		head->me_next = NULL;

		/* we don't care about FS type at all */
		if (head->me_type_malloced) {
			free(head->me_type);
			head->me_type_malloced = 0;
		}
		head->me_type = NULL;

		if (!mog_mnt_usable(head))
			goto skip;

		/* mark the device as something we _might_ track util for */
		mog_iou_active(head->me_dev);

		switch (hash_insert_if_absent(tbl, head, &exist.ptr)) {
		case 0: {
			/* chain entries if they have multiple st_dev */
			struct mount_entry *me = exist.old_me;

			while (me->me_next)
				me = me->me_next;

			assert(me != head && "circular mount ref");
			me->me_next = head;
		}
			continue;
		case 1:
			continue;
		default: mog_oom();
		}
		assert(0 && "compiler bug?");
skip:
		me_free(head);
	}
}

/* runs inside a thread, this is called at startup before daemonization */
static void * init_once(void *ptr)
{
	struct init_args *ia = ptr;

	CHECK(int, 0, pthread_mutex_lock(&by_dev_lock) );
	assert(by_dev == NULL &&
	       "by_dev exists during initialization");
	by_dev = mnt_new(7);
	mnt_populate(by_dev);
	CHECK(int, 0, pthread_mutex_unlock(&by_dev_lock) );

	/* wake up parent thread, this tells parent to cancel us */
	CHECK(int, 0, pthread_mutex_lock(&ia->cond_lock));
	CHECK(int, 0, pthread_cond_signal(&ia->cond));
	CHECK(int, 0, pthread_mutex_unlock(&ia->cond_lock));

	mog_sleep(-1); /* wait for cancellation */
	assert(0 && "init_once did not get cancelled");
	return NULL;
}

/* once-only initialization */
static void timed_init_once(void)
{
	int rc;
	pthread_t thr;
	unsigned long tries;
	struct init_args ia = {
		.cond_lock = PTHREAD_MUTEX_INITIALIZER,
		.cond = PTHREAD_COND_INITIALIZER
	};

	CHECK(int, 0, pthread_mutex_lock(&ia.cond_lock));

	for (tries = 0; ;) {
		rc = pthread_create(&thr, NULL, init_once, &ia);
		if (rc == 0)
			break;

		/* this must succeed, keep looping */
		if (mog_pthread_create_retry(rc)) {
			if ((++tries % 1024) == 0)
				warn("pthread_create: %s (tries: %lu)",
				     strerror(rc), tries);
			sched_yield();
		} else {
			assert(0 && "pthread_create usage error");
		}
	}

	for (tries = 0; ;) {
		struct timespec ts;

		gettime(&ts);
		ts.tv_sec += 5;
		rc = pthread_cond_timedwait(&ia.cond, &ia.cond_lock, &ts);

		if (rc == 0)
			break;
		if (rc == ETIMEDOUT)
			warn("still populating mountlist (tries: %lu)",
			     ++tries);
		else if (rc == EINTR)
			continue;
		else
			assert(0 && "unhandled pthread_cond_timedwait failure");
	}
	CHECK(int, 0, pthread_mutex_unlock(&ia.cond_lock));

	/*
	 * this will load libgcc_s under glibc, we want to do this early
	 * in process lifetime to prevent load failures if we are under
	 * FD pressure later on.
	 */
	CHECK(int, 0, pthread_cancel(thr));

	CHECK(int, 0, pthread_join(thr, NULL));
	CHECK(int, 0, pthread_cond_destroy(&ia.cond));
	CHECK(int, 0, pthread_mutex_destroy(&ia.cond_lock));
	atexit(mnt_atexit);
}

void mog_mnt_refresh(void)
{
	Hash_table *new, *old;
	size_t n = 0;
	static pthread_mutex_t refresh_lock = PTHREAD_MUTEX_INITIALIZER;

	CHECK(int, 0, pthread_mutex_lock(&refresh_lock) ); /* protects old */

	CHECK(int, 0, pthread_mutex_lock(&by_dev_lock) );
	old = by_dev; /* save early for validation */
	if (old)
		n = hash_get_n_buckets_used(old);
	CHECK(int, 0, pthread_mutex_unlock(&by_dev_lock) );

	if (old) {
		mog_iou_cleanup_begin();
		new = mnt_new(n);
		mnt_populate(new); /* slow, can stat all devices */

		/* quickly swap in the new mount list */
		CHECK(int, 0, pthread_mutex_lock(&by_dev_lock) );
		assert(old == by_dev &&
		       "by_dev hash modified during update");
		by_dev = new;
		CHECK(int, 0, pthread_mutex_unlock(&by_dev_lock) );

		/*
		 * must cleanup _after_ replacing by_dev, since readers
		 * can still mark devices as active before we wrlock.
		 */
		mog_iou_cleanup_finish();
		hash_free(old);
	} else {
		timed_init_once();
	}

	CHECK(int, 0, pthread_mutex_unlock(&refresh_lock) );
}

/*
 * Looks up a mount_entry by st_dev, returns NULL if nothing was found
 * Users may only acquire one mount entry at a time and MUST release it
 */
const struct mount_entry * mog_mnt_acquire(dev_t st_dev)
{
	struct mount_entry me = { .me_dev = st_dev };
	struct mount_entry *rv;

	CHECK(int, 0, pthread_mutex_lock(&by_dev_lock) );
	rv = hash_lookup(by_dev, &me);

	/* user must release this via mog_mnt_release if non-NULL */
	if (rv) {
		struct mount_entry *rv_me = rv;

		/*
		 * if multiple entries match st_dev, favor the one
		 * with a leading slash
		 */
		while (rv_me && rv_me->me_devname[0] != '/')
			rv_me = rv_me->me_next;

		return rv_me ? rv_me : rv;
	}

	CHECK(int, 0, pthread_mutex_unlock(&by_dev_lock) );
	return NULL;
}

/* releases the mount entry, allowing mog_mnt_acquire to be called again */
void mog_mnt_release(const struct mount_entry *me)
{
	struct mount_entry *check_me;
	union { const void *in; void *out; } deconst = { .in = me };

	check_me = hash_lookup(by_dev, deconst.out);

	while (check_me->me_next && check_me != me)
		check_me = check_me->me_next;

	assert(check_me == me && "did not release acquired mount_entry");
	CHECK(int, 0, pthread_mutex_unlock(&by_dev_lock) );
}

struct mnt_update {
	const char *prefix;
	size_t prefixlen;
	char util[MOG_IOUTIL_LEN];
};

/*
 * returns true if the mount entry matches the update request
 * (and thus can be updated).  False if no match.
 */
static bool me_update_match(struct mount_entry *me, struct mnt_update *update)
{
	if (strlen(me->me_devname) < update->prefixlen)
		return false;
	return memcmp(update->prefix, me->me_devname, update->prefixlen) == 0;
}

/* Hash iterator */
static bool update_util_each(void *ent, void *upd)
{
	struct mount_entry *me = ent;
	struct mnt_update *update = upd;
	dev_t this_dev = me->me_dev;

	/* attempt to resolve multiple mounts mapped to the same mount point */
	for (; me; me = me->me_next) {
		assert(this_dev == me->me_dev && "me_dev mismatch");

		if (me_update_match(me, update)) {
			mog_iou_write(this_dev, update->util);
			/*
			 * We could cull mismatched mount entries here.
			 * mount point aliasing is relatively uncommon so
			 * probably not worth the code.
			 */
			break;
		}
	}

	return true; /* continue */
}

/*
 * takes a line of iostat information and updates entries in our
 * mountlist which match it.  This is O(mountpoints) for now.
 */
void mog_mnt_update_util(struct mog_iostat *iostat)
{
	struct mnt_update update;
	const char *devsuffix = iostat->dev;

	update.prefix = xasprintf("/dev/%s", devsuffix);
	update.prefixlen = strlen(update.prefix);
	assert(sizeof(update.util) == sizeof(iostat->util));
	memcpy(&update.util, iostat->util, sizeof(update.util));

	CHECK(int, 0, pthread_mutex_lock(&by_dev_lock) );
	(void)hash_do_for_each(by_dev, update_util_each, &update);
	CHECK(int, 0, pthread_mutex_unlock(&by_dev_lock) );

	mog_free(update.prefix);
}
