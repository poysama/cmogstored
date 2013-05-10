/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
struct mog_svc;
struct mog_cfg {
	const char *docroot;
	bool daemonize;
	unsigned long maxconns;
	const char *pidfile;
	const char *configfile; /* expanded path */
	const char *config; /* command-line arg */
	const char *server;
	struct mog_addrinfo *httplisten;
	struct mog_addrinfo *mgmtlisten;
	struct mog_addrinfo *httpgetlisten; /* unique to cmogstored */
	struct mog_svc *svc;
};

void mog_cfg_validate_or_die(struct mog_cfg *cli);
bool mog_cfg_validate_one(void *ent, void *cli);
bool mog_cfg_validate_multi(void *ent, void *cli);
bool mog_cfg_validate_daemon(void *ent, void *nerr);
void mog_cfg_die_if_cli_set(struct mog_cfg *);
void mog_cfg_merge_defaults(struct mog_cfg *);
void mog_cfg_check_server(struct mog_cfg *);
