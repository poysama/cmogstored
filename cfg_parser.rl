/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
/*
 * parses config files used by the original (Perl) mogstored
 */
#include "cmogstored.h"
#include "cfg.h"
#include "listen_parser.h"

static char *mystrdup(const char *key, char *mark_beg, const char *p)
{
	size_t mark_len = p - mark_beg;
	mark_beg[mark_len] = 0;
	if (strlen(mark_beg) != mark_len) {
		syslog(LOG_ERR, "NUL character in `%s' value", key);
		return NULL;
	}

	return xstrdup(mark_beg);
}

%%{
	machine cfg_parser;
	include listen_parser_common "listen_parser_common.rl";

	eor = '\n';
	ignored_line := ( (any-'\n')* eor ) @ { fgoto main; };
	lws = (space-'\n');
	comment = lws* ("#"(any-'\n')*);
	comment_line = comment eor;
	sep = lws* "=" lws*;

	path = ((any - space)+) > { mark_beg = fpc; };

	mgmtlisten = lws* "mgmtlisten" sep listen comment* (eor) > {
		a = mog_listen_parse_internal(mark_beg, mark_len,
		                              port_beg, port_len);
		if (!a) return -1;
		cfg->mgmtlisten = a;
	};

	httplisten = lws* "httplisten" sep listen comment* eor > {
		a = mog_listen_parse_internal(mark_beg, mark_len,
		                              port_beg, port_len);
		if (!a) return -1;
		cfg->httplisten = a;
	};
	httpgetlisten = lws* "httpgetlisten" sep listen comment* eor > {
		a = mog_listen_parse_internal(mark_beg, mark_len,
		                              port_beg, port_len);
		if (!a) return -1;
		cfg->httpgetlisten = a;
	};
	docroot = lws* "docroot" sep path (comment* eor) > {
		/* delay realpath(3) until svc init, symlinks may change */
		cfg->docroot = mystrdup("docroot", mark_beg, fpc);
		if (!cfg->docroot) return -1;
	};
	pidfile = lws* "pidfile" sep path (comment* eor) > {
		/* delay realpath(3) until svc init, symlinks may change */
		cfg->pidfile = mystrdup("pidfile", mark_beg, fpc);
		if (!cfg->pidfile) return -1;
	};
	daemonize = lws* "daemonize" comment* eor > { cfg->daemonize = true; };
	maxconns = lws* "maxconns" sep
		(digit+) > { mark_beg = fpc; }
		(comment* eor) > {
			mark_len = fpc - mark_beg;
			mark_beg[mark_len] = 0;
			errno = 0;
			cfg->maxconns = strtoul(mark_beg, NULL, 10);
			if (errno) {
				syslog(LOG_ERR,
				       "failed to parse: maxconns = %s - %m",
				       mark_beg);
				return -1;
			}
		};
	server = lws* "server" sep
		(alpha+) > { mark_beg = fpc; }
		(comment* eor) > {
			cfg->server = mystrdup("server", mark_beg, fpc);
			if (!cfg->server) return -1;
			mog_cfg_check_server(cfg);
		};
	serverbin = lws* "serverbin" sep path
		(comment* eor) > {
			char *tmp = mystrdup("serverbin", mark_beg, fpc);
			if (!tmp) return -1;
			warn("W: serverbin = %s ignored", tmp);
			free(tmp);
		};
	main := (mgmtlisten | httplisten | httpgetlisten |
	         pidfile | docroot | daemonize | maxconns |
		 server | serverbin ) +
		$! {
			fhold;
			fgoto ignored_line;
		};
}%%

%% write data;

/* this is a one-shot parser, no need to stream local config files  */
int mog_cfg_parse(struct mog_cfg *cfg, char *buf, size_t len)
{
	char *p, *pe, *eof = NULL;
	char *mark_beg = NULL;
	char *port_beg = NULL;
	size_t mark_len = 0;
	size_t port_len = 0;
	struct mog_addrinfo *a;
	int cs;

	%% write init;

	p = buf;
	pe = buf + len;

	%% write exec;

	if (cs == cfg_parser_error)
		return -1;

	assert(p <= pe && "buffer overflow after cfg parse");
	return 0;
}
