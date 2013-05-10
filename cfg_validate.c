/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "cfg.h"

static void
paths_eql_or_die(
	const char *param, const char *a_orig, const char *b_orig,
	enum canonicalize_mode_t canon_mode)
{
	char *a = mog_canonpath_die(a_orig, canon_mode);
	char *b = mog_canonpath_die(b_orig, canon_mode);
	int ok = (strcmp(a, b) == 0);

	free(a);
	free(b);

	if (ok) return;

	die("conflicting path values for %s: `%s' != `%s'",
	    param, a_orig, b_orig);
}

static void merge_addr(struct mog_addrinfo **dst, struct mog_addrinfo *src)
{
	if (!*dst && src) *dst = mog_listen_parse(src->orig);
}

static void merge_str(const char **dst, const char *src)
{
	if (!*dst && src) *dst = xstrdup(src);
}

static void validate_merge_common(struct mog_cfg *ent, struct mog_cfg *cli)
{
	merge_addr(&cli->mgmtlisten, ent->mgmtlisten);
	merge_addr(&cli->httplisten, ent->httplisten);
	merge_addr(&cli->httpgetlisten, ent->httpgetlisten);

	/* multiple config files can all specify the same pidfile */
	if (cli->pidfile && ent->pidfile)
		paths_eql_or_die("pidfile", cli->pidfile, ent->pidfile,
		                 CAN_ALL_BUT_LAST);
	merge_str(&cli->pidfile, ent->pidfile);

	cli->maxconns += ent->maxconns;
	cli->daemonize |= ent->daemonize;
}

bool mog_cfg_validate_one(void *ent_ptr, void *cli_ptr)
{
	struct mog_cfg *ent = ent_ptr;
	struct mog_cfg *cli = cli_ptr;

	/*
	 * in the mixed single config file + CLI usage case, ensure docroot
	 * is the same (or only specified in one
	 */
	if (cli->docroot && ent->docroot)
		paths_eql_or_die("docroot", cli->docroot, ent->docroot,
		                 CAN_EXISTING);
	merge_str(&cli->docroot, ent->docroot);

	validate_merge_common(ent, cli);

	return true;
}

bool mog_cfg_validate_multi(void *ent_ptr, void *cli_ptr)
{
	struct mog_cfg *ent = ent_ptr;
	struct mog_cfg *cli = cli_ptr;

	if (!ent->configfile)
		die("BUG: no config path");
	if (!ent->httplisten && !ent->mgmtlisten && !ent->httpgetlisten)
		die("no listeners in --config=%s", ent->configfile);
	if (!ent->docroot)
		die("no docroot in --config=%s", ent->configfile);

	validate_merge_common(ent, cli);

	return true;
}

static void warn_daemonize(size_t *nerr, const char *key, const char *val)
{
	warn("%s=%s must use an absolute path", key, val);
	(*nerr)++;
}

bool mog_cfg_validate_daemon(void *ent_ptr, void *nerr)
{
	struct mog_cfg *ent = ent_ptr;

	if (ent->config&& ent->config[0] != '/')
		warn_daemonize(nerr, "config", ent->config);
	if (ent->pidfile && ent->pidfile[0] != '/')
		warn_daemonize(nerr, "pidfile", ent->pidfile);
	if (ent->docroot && ent->docroot[0] != '/')
		warn_daemonize(nerr, "docroot", ent->docroot);

	return true;
}

static void die_if_set(const void *a, const char *sw)
{
	if (!a) return;
	die("--%s may not be used with multiple --config files", sw);
}

/*
 * some settings we can't make sense of when supporting multiple
 * config files
 */
void mog_cfg_die_if_cli_set(struct mog_cfg *cli)
{
	die_if_set(cli->docroot, "docroot");
	die_if_set(cli->httplisten, "httplisten");
	die_if_set(cli->mgmtlisten, "mgmtlisten");

	/* we don't actually support --httpgetlisten on the CLI ... */
	die_if_set(cli->httpgetlisten, "httpgetlisten");
}

void mog_cfg_merge_defaults(struct mog_cfg *cli)
{
	if (!cli->docroot)
		cli->docroot = xstrdup(MOG_DEFAULT_DOCROOT);

	/* default listeners */
	if (!cli->httplisten && !cli->httpgetlisten && !cli->mgmtlisten) {
		cli->httplisten = mog_listen_parse(MOG_DEFAULT_HTTPLISTEN);
		cli->mgmtlisten = mog_listen_parse(MOG_DEFAULT_MGMTLISTEN);
	}
}

void mog_cfg_check_server(struct mog_cfg *cfg)
{
	const char *s = cfg->server;

	if (!s) return;

	if (strcmp(s, "none") == 0) return;
	if (strcmp(s, "perlbal") == 0)
		warn("W: using internal HTTP for 'server = perlbal' instead");
	else
		die("E: 'server = %s' not understood by cmogstored", s);
}
