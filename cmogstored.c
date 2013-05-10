/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "cfg.h"
#include "nostd/setproctitle.h"
#define THIS "cmogstored"
#define DESC "alternative mogstored implementation for MogileFS"
static char summary[] = THIS " -- "DESC;
const char *argp_program_bug_address = PACKAGE_BUGREPORT;
const char *argp_program_version = THIS" "PACKAGE_VERSION;
static struct mog_fd *master_selfwake;
static sig_atomic_t sigchld_hit;
static sig_atomic_t do_exit;
static sig_atomic_t do_upgrade;
static size_t nthr;
static bool have_mgmt;
static pid_t master_pid;
static pid_t upgrade_pid;
static unsigned long worker_processes;
static bool iostat_running;

#define CFG_KEY(f) -((int)offsetof(struct mog_cfg,f) + 1)
static struct argp_option options[] = {
	{ .name = "daemonize", .key = 'd',
	  .doc = "Daemonize" },
	{ .name = "config", .key = CFG_KEY(configfile),
	  .arg = "<file>",
          .doc = "Set config file (no default, unlike mogstored)" },
	{ .name = "httplisten", .key = CFG_KEY(httplisten),
	  .arg = "<ip:port>",
	  .doc = "IP/Port HTTP server listens on" },
	/*
	 * NOT adding httpgetlisten here to avoid (further) breaking
	 * compatibility with Perl mogstored.  Most of these command-line
	 * options suck anyways.
	 */
	{ .name = "mgmtlisten", .key = CFG_KEY(mgmtlisten),
	  .arg = "<ip:port>",
	  .doc = "IP/Port management/sidechannel listens on" },
	{ .name = "docroot", .key = CFG_KEY(docroot),
	  .arg = "<path>",
	  .doc = "Docroot above device mount points.  "
	         "Defaults to "MOG_DEFAULT_DOCROOT
	},
	{ .name = "maxconns", .key = CFG_KEY(maxconns),
	  .arg = "<number>",
	  .doc = "Number of simultaneous clients to serve. "
	         "Default " MOG_STR(MOG_DEFAULT_MAXCONNS) },
	{ .name = "pidfile", .key = CFG_KEY(pidfile),
	  .arg = "<path>",
	  .doc = "path to PID file" },
	{ .name = "server", .key = CFG_KEY(server), .flags = OPTION_HIDDEN,
	  .arg = "(perlbal|none)"
	},
	{
	  /* hidden for now, don't break compat with Perl mogstored */
	  .name = "multi", .key = -'M', .flags = OPTION_HIDDEN
	},
	{
	  /* hidden for now, don't break compat with Perl mogstored */
	  .name = "worker-processes", .key = -'W', .flags = OPTION_HIDDEN,
	  .arg = "<number>",
	},
	/* we don't load a default config file like Perl mogstored at all */
	{ .name = "skipconfig", .key = 's', .flags = OPTION_HIDDEN },
	{ NULL }
};

static void new_cfg_or_die(const char *config)
{
	struct mog_cfg *cfg = mog_cfg_new(config);

	if (!cfg) die("invalid (or duplicate) config=%s", config);
	if (mog_cfg_load(cfg) == 0) return;

	die("failed to load config=%s: %s",
	    config, errno ? strerror(errno) : "parser error");
}

static void check_strtoul(unsigned long *dst, const char *s, const char *key)
{
	char *end;

	errno = 0;
	*dst = strtoul(s, &end, 10);
	if (errno)
		die_errno("failed to parse --%s=%s", key, s);
	if (*end)
		die("failed to parse --%s=%s (invalid character: %c)",
		    key, s, *end);
}

static void addr_or_die(struct mog_addrinfo **dst, const char *key, char *s)
{
	*dst = mog_listen_parse(s);
	if (!*dst)
		die("failed to parse %s=%s", key, s);
}

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	struct mog_cfg *cfg = state->input;
	int rv = 0;

	switch (key) {
	case 'd': cfg->daemonize = true; break;
	case 's': /* no-op, we don't load the default config file */; break;
	case CFG_KEY(docroot): cfg->docroot = xstrdup(arg); break;
	case CFG_KEY(pidfile): cfg->pidfile = xstrdup(arg); break;
	case CFG_KEY(configfile): new_cfg_or_die(arg); break;
	case CFG_KEY(maxconns):
		check_strtoul(&cfg->maxconns, arg, "maxconns");
		break;
	case CFG_KEY(httplisten):
		addr_or_die(&cfg->httplisten, "httplisten", arg);
		break;
	case CFG_KEY(mgmtlisten):
		addr_or_die(&cfg->mgmtlisten, "mgmtlisten", arg);
		break;
	case CFG_KEY(server):
		cfg->server = xstrdup(arg);
		mog_cfg_check_server(cfg);
		break;
	case -'M': mog_cfg_multi = true; break;
	case -'W':
		check_strtoul(&worker_processes, arg, "worker-processes");
		if (worker_processes > UINT_MAX)
			die("--worker-processes exceeded (max=%u)", UINT_MAX);
		break;
	case ARGP_KEY_ARG:
		argp_usage(state);
	case ARGP_KEY_END:
		break;
	default:
		rv = ARGP_ERR_UNKNOWN;
	}

	return rv;
}

static void dup2_null(int oldfd, int newfd, const char *errdest)
{
	int rc;

	do {
		rc = dup2(oldfd, newfd);
	} while (rc < 0 && (errno == EINTR || errno == EBUSY));

	if (rc < 0)
		die_errno("dup2(/dev/null,%s) failed", errdest);
}

static void daemonize(void)
{
	int ready_pipe[2];
	pid_t pid;
	ssize_t r;
	int fd;

	if (pipe(ready_pipe) < 0)
		die_errno("pipe() failed");
	pid = fork();
	if (pid < 0)
		die_errno("fork() failed");
	if (pid > 0) { /* parent */
		mog_close(ready_pipe[1]);
		do {
			r = read(ready_pipe[0], &pid, sizeof(pid_t));
		} while (r < 0 && errno == EINTR);

		PRESERVE_ERRNO( mog_close(ready_pipe[0]) );
		if (r == sizeof(pid_t))
			exit(EXIT_SUCCESS);
		if (r < 0)
			die_errno("ready_pipe read error");
		die("daemonization error, check syslog");
	}

	/* child */
	mog_close(ready_pipe[0]);
	if (setsid() < 0)
		die_errno("setsid() failed");

	pid = fork();
	if (pid < 0)
		die_errno("fork() failed");
	if (pid > 0) /* intermediate parent */
		exit(EXIT_SUCCESS);

	if (chdir("/") < 0)
		die_errno("chdir(/) failed");
	fd = open("/dev/null", O_RDWR);
	if (fd < 0)
		die_errno("open(/dev/null) failed");

	dup2_null(fd, STDIN_FILENO, "stdin");
	dup2_null(fd, STDOUT_FILENO, "stdout");
	dup2_null(fd, STDERR_FILENO, "stderr");
	mog_close(fd);

	do {
		r = write(ready_pipe[1], &pid, sizeof(pid_t));
	} while (r < 0 && errno == EINTR);
	if (r < 0)
		syslog(LOG_CRIT, "ready_pipe write error: %m");
	else
		assert(r == sizeof(pid_t) && "impossible short write");
	mog_close(ready_pipe[1]);
}

#ifndef LOG_PERROR
# define LOG_PERROR 0
#endif

/* TODO: make logging configurable (how?) */
static void log_init(bool is_daemon)
{
	int mask = 0;
	int option = LOG_PID;

	if (!is_daemon)
		option |= LOG_PERROR;

	openlog(THIS, option, LOG_DAEMON);
	mask |= LOG_MASK(LOG_EMERG);
	mask |= LOG_MASK(LOG_ALERT);
	mask |= LOG_MASK(LOG_CRIT);
	mask |= LOG_MASK(LOG_ERR);
	mask |= LOG_MASK(LOG_WARNING);
	mask |= LOG_MASK(LOG_NOTICE);
	mask |= LOG_MASK(LOG_INFO);
	/* mask |= LOG_MASK(LOG_DEBUG); */
	setlogmask(mask);
}

MOG_NOINLINE static void setup(int argc, char *argv[])
{
	int pid_fd = -1;
	static struct argp argp = { options, parse_opt, NULL, summary };

	mog_mnt_refresh();
	argp_parse(&argp, argc, argv, 0, NULL, &mog_cli);
	mog_cfg_validate_or_die(&mog_cli);
	log_init(mog_cli.daemonize);
	mog_inherit_init();
	mog_cfg_svc_start_or_die(&mog_cli);
	mog_inherit_cleanup();

	if (mog_cli.pidfile) pid_fd = mog_pidfile_prepare(mog_cli.pidfile);

	/* don't daemonize if we're inheriting FDs, we're already daemonized */
	if (mog_cli.daemonize && !getenv("CMOGSTORED_FD"))
		daemonize();

	if (pid_fd >= 0 && mog_pidfile_commit(pid_fd) < 0)
		syslog(LOG_ERR,
		       "failed to write pidfile(%s): %m. continuing...",
		       mog_cli.pidfile);

	master_pid = getpid();

	/* 10 - 100 threads based on number of devices, same as mogstored */
	nthr = mog_mkusage_all() * 10;
	nthr = MAX(10, nthr);
	nthr = MIN(100, nthr);
}

/* Hash iterator function */
static bool svc_start_each(void *svcptr, void *qptr)
{
	struct mog_svc *svc = svcptr;
	struct mog_queue *q = qptr;
	struct mog_accept *ac;
	size_t athr = (size_t)num_processors(NPROC_CURRENT);

	/*
	 * try to distribute accept() callers between workers more evenly
	 * with wake-one accept() behavior by trimming down on acceptors
	 */
	if (worker_processes) {
		athr /= worker_processes;
		if (athr == 0)
			athr = 1;
	}

	svc->queue = q;

	if (svc->mgmt_fd >= 0) {
		have_mgmt = true;
		ac = mog_accept_init(svc->mgmt_fd, svc, mog_mgmt_post_accept);

		/*
		 * mgmt port is rarely used and always persistent, so it
		 * does not need multiple threads for blocking accept()
		 */
		mog_thrpool_start(&ac->thrpool, 1, mog_accept_loop, ac);
	}

	if (svc->http_fd >= 0) {
		ac = mog_accept_init(svc->http_fd, svc, mog_http_post_accept);
		mog_thrpool_start(&ac->thrpool, athr, mog_accept_loop, ac);
	}

	if (svc->httpget_fd >= 0) {
		ac = mog_accept_init(svc->httpget_fd, svc,
		                     mog_httpget_post_accept);
		mog_thrpool_start(&ac->thrpool, athr, mog_accept_loop, ac);
	}

	return true;
}

static void worker_wakeup_handler(int signum)
{
	switch (signum) {
	case SIGUSR2: do_upgrade = 1; break;
	case SIGCHLD: sigchld_hit = 1; break;
	case SIGQUIT:
	case SIGTERM:
	case SIGINT:
		do_exit = signum;
	}
	mog_notify(MOG_NOTIFY_SIGNAL);
}

static void wakeup_noop(int signum)
{
	/* just something to cause EINTR */
}

static void master_wakeup_handler(int signum)
{
	switch (signum) {
	case SIGCHLD: sigchld_hit = 1; break;
	case SIGUSR2: do_upgrade = 1; break;
	case SIGQUIT:
	case SIGTERM:
	case SIGINT:
		do_exit = signum;
	}
	mog_selfwake_trigger(master_selfwake);
}

static void siginit(void (*wakeup_handler)(int))
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	CHECK(int, 0, sigemptyset(&sa.sa_mask) );

	sa.sa_handler = SIG_IGN;
	CHECK(int, 0, sigaction(SIGPIPE, &sa, NULL));

	sa.sa_handler = wakeup_noop;
	CHECK(int, 0, sigaction(SIGURG, &sa, NULL));

	sa.sa_handler = wakeup_handler;

	/* TERM and INT are graceful shutdown for now, no immediate shutdown */
	CHECK(int, 0, sigaction(SIGTERM, &sa, NULL));
	CHECK(int, 0, sigaction(SIGINT, &sa, NULL));

	CHECK(int, 0, sigaction(SIGQUIT, &sa, NULL)); /* graceful shutdown */
	CHECK(int, 0, sigaction(SIGUSR1, &sa, NULL)); /* no-op, nginx compat */
	CHECK(int, 0, sigaction(SIGUSR2, &sa, NULL)); /* upgrade */

	/*
	 * SIGWINCH/SIGHUP are no-ops for now to allow reuse of nginx init
	 * scripts.  We should support them in the future.
	 * SIGWINCH will disable new connections and drop idlers
	 * SIGHUP will reenable new connections/idlers after SIGWINCH
	 */
	CHECK(int, 0, sigaction(SIGWINCH, &sa, NULL));
	CHECK(int, 0, sigaction(SIGHUP, &sa, NULL));

	sa.sa_flags = SA_NOCLDSTOP;
	CHECK(int, 0, sigaction(SIGCHLD, &sa, NULL));
}

static void process_died(pid_t pid, int status);

static void sigchld_handler(void)
{
	sigchld_hit = 0;

	for (;;) {
		int status;
		pid_t pid = waitpid(-1, &status, WNOHANG);

		if (pid > 0) {
			process_died(pid, status);
		} else if (pid == 0) {
			return;
		} else {
			switch (errno) {
			case EINTR: sigchld_hit = 1; return; /* retry later */
			case ECHILD: return;
			default: die_errno("waitpid");
			}
		}
	}
}

static void upgrade_handler(void)
{
	do_upgrade = 0;
	if (upgrade_pid > 0) {
		syslog(LOG_INFO, "upgrade already running on PID:%d",
		       upgrade_pid);
	} else {
		if (master_pid == getpid())
			upgrade_pid = mog_upgrade_spawn();
		/* else: worker processes (if configured) do not upgrade */
	}
}

static void main_worker_loop(const pid_t parent)
{
	mog_cancel_disable(); /* mog_idleq_wait() now relies on this */
	while (parent == 0 || parent == getppid()) {
		mog_notify_wait(have_mgmt);
		if (sigchld_hit)
			sigchld_handler();
		if (do_upgrade)
			upgrade_handler();
		if (do_exit)
			cmogstored_exit();
		if (have_mgmt)
			mog_mnt_refresh();
		else if (have_mgmt && !iostat_running && !do_exit)
			/*
			 * maybe iostat was not installed/available/usable at
			 * startup, but became usable later
			 */
			iostat_running = mog_iostat_respawn(0);
	}

	syslog(LOG_INFO, "parent=%d abandoned us, dying", parent);
	cmogstored_exit();
}

static void run_worker(const pid_t parent)
{
	struct mog_queue *q = mog_queue_new();

	mog_notify_init();
	siginit(worker_wakeup_handler);
	mog_thrpool_start(&q->thrpool, nthr, mog_queue_loop, q);
	have_mgmt = false;
	mog_svc_each(svc_start_each, q); /* this will set have_mgmt */
	if (have_mgmt) {
		iostat_running = mog_iostat_respawn(0);
		if (!iostat_running)
			syslog(LOG_WARNING, "iostat(1) not available/running");
	}
	main_worker_loop(parent);
}

static void fork_worker(unsigned worker_id)
{
	pid_t pid;
	pid_t parent = getpid(); /* not using getppid() since it's racy */

	pid = fork();
	if (pid > 0) {
		mog_process_register(pid, worker_id);
	} else if (pid == 0) {
		/* workers have no workers of their own */
		worker_processes = 0;
		mog_process_reset();

		/* worker will call mog_intr_enable() later in notify loop */
		run_worker(parent);
		exit(EXIT_SUCCESS);
	} else {
		syslog(LOG_ERR, "fork() failed: %m, sleeping for 10s");
		mog_sleep(10);
	}
}

/* run when a worker dies */
static void worker_died(pid_t pid, int status, unsigned id)
{
	if (do_exit)
		return;

	syslog(LOG_INFO,
	       "worker[%u] PID:%d died with status=%d, respawning",
	       id, (int)pid, status);
	fork_worker(id);
}

static void iostat_died(pid_t pid, int status)
{
	if (do_exit)
		return;
	iostat_running = mog_iostat_respawn(status);
}

/* run when any process dies */
static void process_died(pid_t pid, int status)
{
	unsigned id = mog_process_reaped(pid);
	char *name;

	if (mog_process_is_worker(id)) {
		worker_died(pid, status, id);
		return;
	}

	switch (id) {
	case MOG_PROC_IOSTAT:
		assert(worker_processes == 0 &&
		       "master process registered iostat process");
		iostat_died(pid, status);
		return;
	case MOG_PROC_UPGRADE:
		assert(pid == upgrade_pid && "upgrade_pid misplaced");
		syslog(LOG_INFO, "upgrade PID:%d exited with status=%d",
		       pid, status);
		mog_pidfile_upgrade_abort();
		upgrade_pid = -1;
		return;
	default:
		/* could be an inherited iostat if we're using worker+master */
		name = mog_process_name(id);
		syslog(LOG_INFO,
		       "reaped %s pid=%d with status=%d, ignoring",
		       name, (int)pid, status);
		free(name);
	}
}

static void run_master(void)
{
	unsigned id;
	size_t running = worker_processes;

	master_selfwake = mog_selfwake_new();
	siginit(master_wakeup_handler);

	for (id = 0; id < worker_processes; id++)
		fork_worker(id);

	while (running > 0) {
		mog_selfwake_wait(master_selfwake);
		if (sigchld_hit)
			sigchld_handler();
		if (do_upgrade)
			upgrade_handler();
		if (do_exit)
			running = mog_kill_each_worker(SIGQUIT);
	}
}

int main(int argc, char *argv[], char *envp[])
{
	mog_upgrade_prepare(argc, argv, envp);
	/* hack for older gcov + gcc, see nostd/setproctitle.h */
	spt_init(argc, argv, envp);
	set_program_name(argv[0]);

	mog_intr_disable();
	setup(argc, argv); /* this daemonizes */

	mog_process_init(worker_processes);
	if (worker_processes == 0)
		run_worker(0);
	else
		run_master();

	return 0;
}

/* called by the "shutdown" command via mgmt */
void cmogstored_quit(void)
{
	if (master_pid != getpid()) {
		if (kill(master_pid, SIGQUIT) != 0)
			syslog(LOG_ERR,
			       "SIGQUIT failed on master process (pid=%d): %m",
				master_pid);
	} else {
		worker_wakeup_handler(SIGQUIT);
	}
}
