/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
static Hash_table *processes;

struct worker_kill {
	int signal;
	unsigned count;
};

static bool process_cmp(const void *_a, const void *_b)
{
	const struct mog_process *a = _a;
	const struct mog_process *b = _b;

	return a->pid == b->pid;
}

static size_t process_hash(const void *x, size_t tablesize)
{
	const struct mog_process *p = x;

	return p->pid % tablesize;
}

/* needed to make valgrind happy */
__attribute__((destructor)) static void process_atexit(void)
{
	if (processes)
		hash_free(processes);
}

/* call before forking */
void mog_process_init(size_t nr)
{
	if (nr < 3)
		nr = 3;
	processes = hash_initialize(nr, NULL, process_hash, process_cmp, free);
	if (processes == NULL)
		mog_oom();
}

void mog_process_reset(void)
{
	assert(processes && "mog_process_init() never called");
	hash_clear(processes);
}

char *mog_process_name(unsigned id)
{
	if (mog_process_is_worker(id))
		return xasprintf("worker[%u]", id);

	switch (id) {
	case MOG_PROC_UNKNOWN: return xstrdup("unknown");
	case MOG_PROC_IOSTAT: return xstrdup("iostat");
	case MOG_PROC_UPGRADE: return xstrdup("upgrade");
	}
	assert(0 && "Unknown ID");
	return xasprintf("BUG[%u]", id);
}

bool mog_process_is_worker(unsigned id)
{
	switch (id) {
	case MOG_PROC_UNKNOWN:
	case MOG_PROC_IOSTAT:
	case MOG_PROC_UPGRADE:
		return false;
	}
	return true;
}

/* hash iterator */
static bool kill_worker(void *ent, void *k)
{
	struct mog_process *p = ent;
	struct worker_kill *wk = k;

	assert(p->id != MOG_PROC_UNKNOWN &&
	      "MOG_PROC_UNKNOWN should not be registered");

	if (!mog_process_is_worker(p->id))
		return true;

	wk->count++;
	if (kill(p->pid, wk->signal) == 0)
		return true;

	/*
	 * ESRCH: race between receiving a signal and waitpid(),
	 * ignore the error but count it, so we'lll know to wait on it.
	 */
	if (errno != ESRCH)
		syslog(LOG_ERR, "could not signal worker[%u] pid=%d: %m",
		       p->id, (int)p->pid);
	return true;
}

/*
 * send signal to each worker process, returns number of processes
 * signalled.  (signal=0 counts workers registered)
 */
size_t mog_kill_each_worker(int signo)
{
	struct worker_kill wk = { .signal = signo, .count = 0 };

	hash_do_for_each(processes, kill_worker, &wk);

	return (size_t)wk.count;
}

/* Registers a process with a given id */
void mog_process_register(pid_t pid, unsigned id)
{
	struct mog_process *p = xmalloc(sizeof(struct mog_process));

	assert(id != MOG_PROC_UNKNOWN &&
	      "MOG_PROC_UNKNOWN may not be registered");

	p->pid = pid;
	p->id = id;

	if (hash_insert(processes, p) == NULL)
		mog_oom();
}

/*
 * Call on a pid after a process is reaped, returns the id of the process
 * Returns MOG_PROC_UNKNOWN if the pid was unknown
 */
unsigned mog_process_reaped(pid_t pid)
{
	struct mog_process p = { .pid = pid, .id = MOG_PROC_UNKNOWN };
	struct mog_process *r;

	r = hash_delete(processes, &p);
	if (r) {
		p.id = r->id;
		free(r);
	}
	return p.id;
}
