/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "cfg.h"

static Hash_table *all_cfg; /* we support multiple configs -> svcs */
struct mog_cfg mog_cli;
bool mog_cfg_multi;

static void cfg_free_internal(struct mog_cfg *cfg)
{
	mog_free(cfg->docroot);
	mog_free(cfg->pidfile);
	mog_free(cfg->config);
	mog_free(cfg->configfile);
	mog_free(cfg->server);
	mog_addrinfo_free(&cfg->mgmtlisten);
	mog_addrinfo_free(&cfg->httplisten);
	mog_addrinfo_free(&cfg->httpgetlisten);
	/* let svc.c deal with cfg->svc for now */
}

static void cfg_free(void *ptr)
{
	struct mog_cfg *cfg = ptr;
	cfg_free_internal(cfg);
	free(cfg);
}

static size_t cfg_hash(const void *x, size_t tablesize)
{
	const struct mog_cfg *cfg = x;

	return hash_string(cfg->configfile, tablesize);
}

static bool cfg_cmp(const void *a, const void *b)
{
	const struct mog_cfg *cfg_a = a;
	const struct mog_cfg *cfg_b = b;

	return strcmp(cfg_a->configfile, cfg_b->configfile) == 0;
}

static void cfg_atexit(void)
{
	hash_free(all_cfg);
	cfg_free_internal(&mog_cli);
}

__attribute__((constructor)) static void cfg_init(void)
{
	all_cfg = hash_initialize(7, NULL, cfg_hash, cfg_cmp, cfg_free);
	if (!all_cfg)
		mog_oom();

	atexit(cfg_atexit);
}

struct mog_cfg * mog_cfg_new(const char *configfile)
{
	struct mog_cfg *cfg = xzalloc(sizeof(struct mog_cfg));

	cfg->configfile = mog_canonpath_die(configfile, CAN_EXISTING);
	cfg->config = xstrdup(configfile);

	switch (hash_insert_if_absent(all_cfg, cfg, NULL)) {
	case 0:
		cfg_free(cfg);
		cfg = NULL;
	case 1: break;
	default: mog_oom();
	}

	return cfg;
}

int mog_cfg_load(struct mog_cfg *cfg)
{
	struct stat sb;
	char *buf = NULL;
	ssize_t r;
	int rc = -1;
	int fd = open(cfg->configfile, O_RDONLY);

	if (fd < 0) goto out;
	if (fstat(fd, &sb) < 0) goto out;

	buf = xmalloc(sb.st_size + strlen("\n"));

	errno = 0;
	r = read(fd, buf, sb.st_size);
	if (r != sb.st_size)
		die("read(..., %ld) failed on %s: %s",
		    (long)sb.st_size, cfg->configfile,
		    errno ? strerror(errno) : "EOF");

	buf[r] = '\n'; /* parser _needs_ a trailing newline */
	rc = mog_cfg_parse(cfg, buf, r + 1);
out:
	PRESERVE_ERRNO(do {
		if (buf) free(buf);
		if (fd >= 0) mog_close(fd);
	} while(0));

	return rc;
}

static size_t nr_config(void)
{
	return all_cfg ? hash_get_n_entries(all_cfg) : 0;
}

#define RELPATH_ERR \
"relative paths are incompatible with --daemonize and SIGUSR2 upgrades"
static void validate_daemonize(struct mog_cfg *cli)
{
	size_t nerr = 0;
	const char *path = getenv("PATH");
	const char *p;

	hash_do_for_each(all_cfg, mog_cfg_validate_daemon, &nerr);

	/* cli may have merged identical settings */
	if (!nerr)
		mog_cfg_validate_daemon(cli, &nerr);

	/* we may use confstr(_CS_PATH) in the future, currently we do not */
	if (!path)
		die("PATH environment must be set");

	p = path;

	/* trailing ':' in PATH is identical to trailing ":." (cwd) */
	if (p[strlen(p) - 1] == ':')
		goto err;

	while (*p) {
		if (*p == '/') {
			p = strchr(p, ':');
			if (!p)
				break;
			p++;
			continue;
		}
err:
		warn("PATH environment contains relative path: %s", p);
		nerr++;
		break;
	}

	if (nerr)
		die(RELPATH_ERR);
}

#define MULTI_CFG_ERR \
"--multi must be set if using multiple --config/-c switches"

void mog_cfg_validate_or_die(struct mog_cfg *cli)
{
	switch (nr_config()) {
	case 0:
		mog_cfg_merge_defaults(cli);
		assert(cli->configfile == NULL &&
		       "BUG: --config was set but not detected");
		break; /* CLI-only */
	case 1:
		hash_do_for_each(all_cfg, mog_cfg_validate_one, cli);
		mog_cfg_merge_defaults(cli);
		break;
	default: /* multiple config files */
		mog_cfg_die_if_cli_set(cli);
		hash_do_for_each(all_cfg, mog_cfg_validate_multi, cli);
		if (!mog_cfg_multi)
			die(MULTI_CFG_ERR);
	}
	if (cli->daemonize)
		validate_daemonize(cli);
	mog_set_maxconns(cli->maxconns);
}

static int bind_or_die(struct mog_addrinfo *a, const char *accept_filter)
{
	int fd;

	if (a == NULL) return -1;
	fd = mog_bind_listen(a->addr, accept_filter);
	if (fd >= 0) return fd;

	die_errno("addr=%s failed to bind+listen", a->orig);
}

static bool svc_from_cfg(void *cfg_ptr, void *ignored)
{
	struct mog_cfg *cfg = cfg_ptr;
	struct mog_svc *svc;

	assert(cfg->docroot && "no docroot specified");
	svc = mog_svc_new(cfg->docroot);
	if (!svc)
		die("failed to load svc from docroot=%s", cfg->docroot);

	svc->mgmt_fd = bind_or_die(cfg->mgmtlisten, "dataready");

	if (cfg->server && strcmp(cfg->server, "none") == 0)
		return true;

	svc->http_fd = bind_or_die(cfg->httplisten, "httpready");
	svc->httpget_fd = bind_or_die(cfg->httpgetlisten, "httpready");

	return true;
}

void mog_cfg_svc_start_or_die(struct mog_cfg *cli)
{
	switch (nr_config()) {
	case 0:
	case 1:
		svc_from_cfg(cli, NULL);
		break;
	default:
		hash_do_for_each(all_cfg, svc_from_cfg, NULL);
	}
}
