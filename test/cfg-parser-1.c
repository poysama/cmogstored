/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "check.h"
#include "cfg.h"

int main(void)
{
	struct mog_cfg cfg;
	int rc;

	openlog("mog-test-cfg-parser-1", LOG_PID, LOG_USER);

	{
		char *s = xstrdup("127.0.0.1:666");
		struct mog_addrinfo *tmp = mog_listen_parse(s);
		assert(tmp && "failed to parse");
		mog_addrinfo_free(&tmp);
		assert(tmp == NULL && "not nulled");
		free(s);
	}

	{
		char *s = xstrdup("127-0.0.1:666");
		struct mog_addrinfo *tmp = mog_listen_parse(s);
		assert(tmp == NULL && "not null");
		free(s);
	}

	{
		char buf[] = "httplisten = 127.0.0.1:7500 # foo bar\n";
		memset(&cfg, 0, sizeof(struct mog_cfg));
		rc = mog_cfg_parse(&cfg, buf, sizeof(buf) - 1);

		assert(cfg.httplisten && "httplisten unset");
		assert(! cfg.mgmtlisten && "mgmtlisten set");
		assert(! cfg.httpgetlisten && "httpgetlisten set");
		assert(! cfg.daemonize && "daemonize set");
		assert(! cfg.maxconns && "maxconns set");
		assert(rc == 0 && "parser failed");

		mog_addrinfo_free(&cfg.httplisten);
	}

	{
		char buf[] = "httplisten = 127.0.0.1:7500\n"
		             "mgmtlisten = 127.6.6.6:7501\n"
		             "httpgetlisten = 127.6.6.6:7502\n"
			     "\n"
			     "docroot = /var/mogdata\n"
			     "daemonize\n"
			     "pidfile = /tmp/.cmogstored.pid\n"
			     "# hello \n"
			     "maxconns = 666666\n";

		memset(&cfg, 0, sizeof(struct mog_cfg));
		rc = mog_cfg_parse(&cfg, buf, sizeof(buf) - 1);

		assert(rc == 0 && "parser failed");
		assert(cfg.httplisten && "httplisten unset");
		assert(cfg.mgmtlisten && "mgmtlisten set");
		assert(cfg.httpgetlisten && "httpgetlisten set");
		assert(cfg.daemonize && "daemonize set");
		assert(cfg.maxconns == 666666 && "maxconns set");
		assert(cfg.docroot && "docroot set");
		assert(strcmp(cfg.docroot, "/var/mogdata") == 0 &&
		       "docroot mismatch");
		assert(strcmp(cfg.pidfile, "/tmp/.cmogstored.pid") == 0 &&
		       "pidfile mismatch");
		mog_addrinfo_free(&cfg.httplisten);
		mog_addrinfo_free(&cfg.mgmtlisten);
		mog_addrinfo_free(&cfg.httpgetlisten);
		mog_free(cfg.docroot);
		mog_free(cfg.pidfile);
	}

        {
		char buf[] = "httplisten = 666.0.0.1:7500\n";

		memset(&cfg, 0, sizeof(struct mog_cfg));
		rc = mog_cfg_parse(&cfg, buf, sizeof(buf) - 1);

		assert(rc == -1 && "parser should fail");
		assert(!cfg.httplisten && "nothing should've been allocated");
	}

        {
		char buf[] = "pidfile = /foo\0bar\n";

		memset(&cfg, 0, sizeof(struct mog_cfg));
		rc = mog_cfg_parse(&cfg, buf, sizeof(buf) - 1);

		assert(rc == -1 && "parser should fail");
		assert(!cfg.pidfile && "nothing should've been allocated");
	}

        {
		char buf[] = "mgmtlisten = 666.0.0-1:7500\n";

		memset(&cfg, 0, sizeof(struct mog_cfg));
		rc = mog_cfg_parse(&cfg, buf, sizeof(buf) - 1);

		assert(!cfg.mgmtlisten && "nothing should've been allocated");
	}

	return 0;
}
