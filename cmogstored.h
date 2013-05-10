/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
/*
 * common headers, macros, and static inline functions for the entire project
 *
 * Internal APIs are very much in flux and subject to change frequently
 */
#include "config.h"
#include "queue_kqueue.h"

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifndef _POSIX_C_SOURCE
#define  _POSIX_C_SOURCE 200809L
#endif
#include <pthread.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <syslog.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/statvfs.h>
#include <time.h>
#include <argp.h>
#include <sched.h>
#include <error.h> /* GNU */
#include <poll.h>
#include "bsd/queue_safe.h"
#include "bsd/simpleq.h"

/* gnulib headers */
#include "progname.h"
#include "hash.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "canonicalize.h"
#include "verify.h"
#include "mountlist.h"
#include "base64.h"
#include "minmax.h"
#include "gc.h"
#include "nproc.h"
#include "findprog.h"
#include "timespec.h"

#include "gcc.h"
#include "util.h"
#include "defaults.h"
#include "iostat.h"
#include "mnt.h"

#define MOG_WR_ERROR ((void *)-1)
#define MOG_IOSTAT (MAP_FAILED)
#define MOG_FD_MAX (INT_MAX-1)

enum mog_write_state {
	MOG_WRSTATE_ERR = -1,
	MOG_WRSTATE_DONE = 0,
	MOG_WRSTATE_BUSY = 1
};

enum mog_parser_state {
	MOG_PARSER_ERROR = -1,
	MOG_PARSER_DONE = 0,
	MOG_PARSER_CONTINUE = 1
};

enum mog_next {
	MOG_NEXT_CLOSE = 0,
	MOG_NEXT_ACTIVE,
	MOG_NEXT_WAIT_RD,
	MOG_NEXT_WAIT_WR,
	MOG_NEXT_IGNORE /* for iostat and fsck MD5 */
};

struct mog_wbuf;
struct mog_dev {
	dev_t st_dev;
	uint32_t devid;
	char prefix[FLEXIBLE_ARRAY_MEMBER];
};

struct mog_rbuf {
	size_t rcapa;
	size_t rsize; /* only set on stash */
	char rptr[FLEXIBLE_ARRAY_MEMBER];
};

#define MOG_RBUF_OVERHEAD (sizeof(struct mog_rbuf))
#define MOG_RBUF_BASE_SIZE (512 - MOG_RBUF_OVERHEAD)
#define MOG_RBUF_MAX_SIZE (UINT16_MAX)

enum mog_prio {
	MOG_PRIO_NONE = 0,
	MOG_PRIO_FSCK
};

struct mog_mgmt;
struct mog_mgmt {
	int cs;
	enum mog_prio prio;
	struct mog_fd *forward;
	size_t offset;
	size_t mark[2];
	struct mog_rbuf *rbuf;
	struct mog_wbuf *wbuf; /* uncommonly needed */
	struct mog_svc *svc;
	enum Gc_hash alg;
	LIST_ENTRY(mog_mgmt) subscribed;
	SIMPLEQ_ENTRY(mog_mgmt) fsckq;
};

struct mog_queue;
struct mog_svc {
	int docroot_fd;
	const char *docroot;

	/* private */
	DIR *dir;
	Hash_table *by_st_dev;
	pthread_mutex_t devstats_lock;
	struct mog_queue *queue;
	LIST_HEAD(mgmt_head, mog_mgmt) devstats_subscribers;
	mode_t put_perms;
	mode_t mkcol_perms;
	int http_fd;
	int httpget_fd;
	int mgmt_fd;
	uint32_t idle_timeout;
};

enum mog_http_method {
	MOG_HTTP_METHOD_NONE = 0,
	MOG_HTTP_METHOD_GET,
	MOG_HTTP_METHOD_HEAD,
	MOG_HTTP_METHOD_PUT,
	MOG_HTTP_METHOD_DELETE,
	MOG_HTTP_METHOD_MKCOL
};

enum mog_chunk_state {
	MOG_CHUNK_STATE_SIZE = 0,
	MOG_CHUNK_STATE_DATA,
	MOG_CHUNK_STATE_TRAILER,
	MOG_CHUNK_STATE_DONE
};

struct mog_http {
	int cs;
	enum mog_http_method http_method:8;
	unsigned persistent:1;
	unsigned chunked:1;
	unsigned has_trailer_md5:1;
	unsigned has_expect_md5:1;
	unsigned has_content_range:1; /* for PUT */
	unsigned has_range:1;         /* for GET */
	unsigned skip_rbuf_defer:1;
	enum mog_chunk_state chunk_state:2;
	uint8_t path_tip;
	uint8_t path_end;
	uint16_t line_end;
	uint16_t tmp_tip;
	struct mog_fd *forward;
	size_t offset;
	off_t range_beg;
	off_t range_end;
	off_t content_len;
	struct mog_rbuf *rbuf;
	struct mog_wbuf *wbuf; /* uncommonly needed */
	struct mog_svc *svc;
	uint8_t expect_md5[16];
};

struct mog_thrpool {
	pthread_mutex_t lock;
	size_t n_threads;
	size_t want_threads;
	pthread_t *threads;
	void *(*start_fn)(void *);
	void *start_arg;
};

struct mog_fd;

/*
 * this is a queue: epoll or kqueue return events in the order they occur
 * mog_queue objects can be shared by any number of mog_svcs
 */
struct mog_queue {
	int queue_fd; /* epoll or kqueue */
	struct mog_thrpool thrpool;
	LIST_ENTRY(mog_queue) qbuddies;
};

/* accept.c */
typedef void (*post_accept_fn)(int fd, struct mog_svc *);
struct mog_accept {
	struct mog_svc *svc;
	post_accept_fn post_accept_fn;
	struct mog_thrpool thrpool;
};
struct mog_accept * mog_accept_init(int fd, struct mog_svc *, post_accept_fn);
void * mog_accept_loop(void *ac);

struct mog_digest {
	enum Gc_hash alg;
	gc_hash_handle ctx;
};

struct mog_file {
	off_t fsize;
	off_t foff;
	char *path;
	size_t pathlen;
	char *tmppath; /* NULL-ed if rename()-ed away */
	void *mmptr;
	struct mog_svc *svc;
	struct mog_digest digest;
};

#include "queue_epoll.h"
#include "notify.h"

/* sig.c */
void mog_intr_disable(void);
void mog_intr_enable(void);
void mog_sleep(long seconds);
#include "selfwake.h"

enum mog_fd_type {
	MOG_FD_TYPE_UNUSED = 0,
	MOG_FD_TYPE_HTTP,
	MOG_FD_TYPE_HTTPGET,
	MOG_FD_TYPE_MGMT,
	MOG_FD_TYPE_IOSTAT,
	MOG_FD_TYPE_SELFWAKE,
	MOG_FD_TYPE_SELFPIPE,
	MOG_FD_TYPE_ACCEPT,
	MOG_FD_TYPE_FILE,
	MOG_FD_TYPE_QUEUE,
	MOG_FD_TYPE_SVC /* for docroot_fd */
};

/* fdmap.c */
struct mog_fd {
	enum mog_fd_type fd_type;
	int fd;
	pthread_spinlock_t expiring;
	union {
		struct mog_accept accept;
		struct mog_mgmt mgmt;
		struct mog_http http;
		struct mog_iostat iostat;
		struct mog_selfwake selfwake;
		struct mog_selfpipe selfpipe;
		struct mog_file file;
		struct mog_queue queue;
		struct mog_svc *svc;
	} as;
};
struct mog_fd *mog_fd_get(int fd);
void mog_fd_put(struct mog_fd *mfd);
void mog_fdmap_requeue(struct mog_queue *quit_queue);
size_t mog_fdmap_expire(uint32_t sec);
extern size_t mog_nr_active_at_quit;
#include "fdmap.h"

/* alloc.c */
void mog_free_and_null(void *ptrptr);
_Noreturn void mog_oom(void);
void *mog_cachealign(size_t size) __attribute__((malloc));
struct mog_rbuf *mog_rbuf_new(size_t size);
struct mog_rbuf *mog_rbuf_get(size_t size);
struct mog_rbuf *mog_rbuf_detach(struct mog_rbuf *rbuf);
struct mog_rbuf *mog_rbuf_grow(struct mog_rbuf *);
void mog_rbuf_free(struct mog_rbuf *);
void mog_rbuf_free_and_null(struct mog_rbuf **);
void *mog_fsbuf_get(size_t *size);
void mog_alloc_quit(void);

#define die_errno(...) do { \
	error(EXIT_FAILURE, errno, __VA_ARGS__); \
	abort(); \
} while (0)

#define die(...) do { \
	error(EXIT_FAILURE, 0, __VA_ARGS__); \
	abort(); \
} while (0)

#define warn(...) error(0, 0, __VA_ARGS__)

/* maxconns.c */
void mog_set_maxconns(unsigned long);

/* svc.c */
struct mog_svc *mog_svc_new(const char *docroot);
typedef int (*mog_scandev_cb)(const struct mog_dev *, struct mog_svc *);
size_t mog_svc_each(Hash_processor processor, void *data);
void mog_svc_upgrade_prepare(void);

/* dev.c */
struct mog_dev * mog_dev_new(struct mog_svc *, uint32_t mog_devid);
int mog_dev_mkusage(const struct mog_dev *, struct mog_svc *);

/* valid_path.rl */
int mog_valid_path(const char *buf, size_t len);

/* trywrite.c */
void * mog_trywritev(int fd, struct iovec *iov, int iovcnt);
enum mog_write_state mog_tryflush(int fd, struct mog_wbuf **);
void * mog_trysend(int fd, void *buf, size_t len, off_t more);

#include "fs.h"

/* pidfile.c */
int mog_pidfile_prepare(const char *path);
int mog_pidfile_commit(int fd);
bool mog_pidfile_upgrade_prepare(void);
void mog_pidfile_upgrade_abort(void);

/* svc_dev.c */
bool mog_svc_devstats_broadcast(void *svc, void *ignored);
void mog_svc_devstats_subscribe(struct mog_mgmt *);
void mog_svc_dev_shutdown(void);
size_t mog_mkusage_all(void);

/* cloexec_detect.c */
extern bool mog_cloexec_atomic;

/* cloexec_from.c */
void mog_cloexec_from(int lowfd);

/* iostat_process.c */
bool mog_iostat_respawn(int oldstatus) MOG_CHECK;

/* cfg_parser.rl */
struct mog_cfg;
int mog_cfg_parse(struct mog_cfg *, char *buf, size_t len);

/* cfg.c */
struct mog_cfg * mog_cfg_new(const char *configfile);
int mog_cfg_load(struct mog_cfg *);
void mog_cfg_svc_start_or_die(struct mog_cfg *cli);
extern struct mog_cfg mog_cli;
extern bool mog_cfg_multi;

/* listen_parser.rl */
struct mog_addrinfo *mog_listen_parse(const char *host_with_port);

/* canonpath.c */
char *mog_canonpath(const char *path, enum canonicalize_mode_t canon_mode);
char *mog_canonpath_die(const char *path, enum canonicalize_mode_t canon_mode);

/* thrpool.c */
void mog_thrpool_start(struct mog_thrpool *, size_t n,
                       void *(*start_fn)(void *), void *arg);
void mog_thrpool_quit(struct mog_thrpool *, struct mog_queue *);
void mog_thrpool_set_n_threads(struct mog_queue *q, size_t size);
void mog_thrpool_process_queue(void);

/* mgmt.c */
void mog_mgmt_writev(struct mog_mgmt *, struct iovec *, int iovcnt);
void mog_mgmt_post_accept(int fd, struct mog_svc *);
enum mog_next mog_mgmt_queue_step(struct mog_fd *) MOG_CHECK;
void mog_mgmt_quit_step(struct mog_fd *);

/* queue_epoll.c */
struct mog_queue * mog_queue_new(void);
void mog_idleq_add(struct mog_queue *, struct mog_fd *, enum mog_qev);
void mog_idleq_push(struct mog_queue *, struct mog_fd *, enum mog_qev);
struct mog_fd * mog_idleq_wait(struct mog_queue *, int timeout);
struct mog_fd *
mog_queue_xchg(struct mog_queue *, struct mog_fd *, enum mog_qev);
struct mog_fd * mog_idleq_wait_intr(struct mog_queue *q, int timeout);

/* addrinfo.c */
struct mog_addrinfo {
	const char *orig;
	struct addrinfo *addr;
};
void mog_addrinfo_free(struct mog_addrinfo **);

/* bind_listen.c */
int mog_bind_listen(struct addrinfo *, const char *accept_filter);

/* close.c */
void mog_close(int fd);

/* mog_queue_loop.c */
void * mog_queue_loop(void *arg);
void mog_queue_quit_loop(struct mog_queue *queue);

/* queue_step.c */
enum mog_next mog_queue_step(struct mog_fd *mfd) MOG_CHECK;

/* file.c */
struct mog_fd * mog_file_open_read(struct mog_svc *, char *path);
struct mog_fd * mog_file_open_put(struct mog_svc *, char *path, int flags);
void mog_file_close(struct mog_fd *);
bool mog_open_expire_retry(struct mog_svc *);

/* notify.c */
void mog_notify_init(void);
void mog_notify(enum mog_notification);
void mog_notify_wait(bool need_usage_file);

/* http_parser.rl */
void mog_http_reset_parser(struct mog_http *);
void mog_http_init(struct mog_http *, struct mog_svc *);
enum mog_parser_state mog_http_parse(struct mog_http *, char *buf, size_t len);

/* http_get.c */
void mog_http_get_open(struct mog_http *, char *buf);
enum mog_next mog_http_get_in_progress(struct mog_fd *);

/* http.c */
void mog_http_post_accept(int fd, struct mog_svc *);
void mog_httpget_post_accept(int fd, struct mog_svc *);
enum mog_next mog_http_queue_step(struct mog_fd *) MOG_CHECK;
void mog_http_quit_step(struct mog_fd *);
char *mog_http_path(struct mog_http *, char *buf);
void mog_http_reset(struct mog_http *);

/* http_dav.c */
void mog_http_delete(struct mog_http *http, char *buf);
void mog_http_mkcol(struct mog_http *http, char *buf);

/* http_put.c */
void mog_http_put(struct mog_http *http, char *buf, size_t buf_len);
enum mog_next mog_http_put_in_progress(struct mog_fd *);
bool mog_http_write_full(struct mog_fd *file_mfd, char *buf, size_t buf_len);

/* chunk_parser.rl */
void mog_chunk_init(struct mog_http *);
enum mog_parser_state mog_chunk_parse(struct mog_http *, char *buf, size_t len);

/* http_date.c */
#define MOG_HTTPDATE_CAPA (sizeof("Thu, 01 Jan 1970 00:00:00 GMT"))
struct mog_now {
	time_t ntime;
	char httpdate[MOG_HTTPDATE_CAPA];
};
char *mog_http_date(char *dst, size_t len, const time_t *timep);
struct mog_now *mog_now(void);

/* mkpath_for.c */
int mog_mkpath_for(struct mog_svc *svc, char *path);

#include "fadvise.h"

/* queue_common.c */
struct mog_queue *mog_queue_init(int queue_fd);
void mog_queue_stop(struct mog_queue *keep);

/* fsck_queue.c */
bool mog_fsck_queue_ready(struct mog_fd *mfd) MOG_CHECK;
void mog_fsck_queue_next(struct mog_fd *mfd);

/* valid_put_path.rl */
bool mog_valid_put_path(const char *buf, size_t len);

/* ioutil.c */
void mog_iou_cleanup_begin(void);
void mog_iou_cleanup_finish(void);
void mog_iou_read(dev_t, char buf[MOG_IOUTIL_LEN]);
void mog_iou_write(dev_t, const char buf[MOG_IOUTIL_LEN]);
void mog_iou_active(dev_t);

#include "activeq.h"

/*
 * non-Linux may not allow MSG_MORE on stream sockets,
 * so limit MSG_MORE usage to Linux for now
 */
#if defined(MSG_MORE) && defined(__linux__)
#  define MOG_MSG_MORE (MSG_MORE)
#else
#  define MOG_MSG_MORE (0)
#endif

#if defined(TCP_NOPUSH) /* FreeBSD */
/*
 * TCP_NOPUSH in modern versions of FreeBSD behave identically to
 * TCP_CORK under Linux (which we used before we switched Linux to MSG_MORE)
 */
#  define MOG_TCP_NOPUSH TCP_NOPUSH
#else
#  define MOG_TCP_NOPUSH (0)
#endif

/* cmogstored.c */
void cmogstored_quit(void);

/* inherit.c */
void mog_inherit_init(void);
int mog_inherit_get(struct sockaddr *addr, socklen_t len);
void mog_inherit_cleanup(void);

/* process.c */
#define MOG_PROC_UNKNOWN (UINT_MAX)
#define MOG_PROC_IOSTAT  (UINT_MAX-1)
#define MOG_PROC_UPGRADE (UINT_MAX-2)
struct mog_process {
	pid_t pid;
	unsigned id;
};

void mog_process_init(size_t nr);
void mog_process_reset(void);
char *mog_process_name(unsigned id);
bool mog_process_is_worker(unsigned id);
size_t mog_kill_each_worker(int signo);
void mog_process_register(pid_t, unsigned id);
unsigned mog_process_reaped(pid_t);

/* upgrade.c */
void mog_upgrade_prepare(int argc, char *argv[], char *envp[]);
pid_t mog_upgrade_spawn(void);

/* exit.c */
_Noreturn void cmogstored_exit(void);

/*
 * We only deal with ipv4 and ipv6 addresses (and no human-friendly
 * hostnames/service names), so we can use smaller constants than the
 * standard NI_MAXHOST/NI_MAXSERV values (1025 and 32 respectively).
 * This reduces our per-thread stack usage and keeps caches hotter.
 */
#define MOG_NI_MAXHOST (INET6_ADDRSTRLEN)
#define MOG_NI_MAXSERV (sizeof(":65536"))

/* avoid sockaddr_storage since that bigger than we need */
union mog_sockaddr {
	struct sockaddr_in in;
	struct sockaddr_in6 in6;
	struct sockaddr sa;
	unsigned char bytes[1];
};
