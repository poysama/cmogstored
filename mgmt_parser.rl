/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "mgmt.h"

/*
 * only set fsck prio if we're still accepting connections, graceful
 * shutdown in single-threaded mode uses normal (fair) prio
 */
static void set_prio_fsck(struct mog_mgmt *mgmt)
{
	if (mgmt->svc->mgmt_fd >= 0)
		mgmt->prio = MOG_PRIO_FSCK;
}

%%{
	machine mgmt_parser;

	eor = '\r'?'\n';
	path = "/"[a-zA-Z0-9/\.\-]*;
	reason = ' '("fsck" @ { set_prio_fsck(mgmt); } | [a-zA-Z0-9_]+);
	invalid_line := (
		[ \t]*
		([^ \t\r]+) > { mgmt->mark[0] = fpc - buf; }
		(any-'\n')* > { mgmt->mark[1] = fpc - buf; }
		'\n'
	) @ {
		mog_mgmt_fn_unknown(mgmt, buf);
		really_done = 1;
		fbreak;
	};
	size = (
		"size "(path) > { mgmt->mark[0] = fpc - buf; }
		eor > { mgmt->mark[1] = fpc - buf; }
		@ { mog_mgmt_fn_size(mgmt, buf); fbreak; }
	);
	digest = (
		(
			"MD5" @ { mgmt->alg = GC_MD5; }
			|
			"SHA-1" @ { mgmt->alg = GC_SHA1; }
		)
		" "
		(path) > { mgmt->mark[0] = fpc - buf; }
		( reason? eor) > { mgmt->mark[1] = fpc - buf; }
		@ { mog_mgmt_fn_digest(mgmt, buf); fbreak; }
	);
	watch = "watch" eor @ {
		static int have_iostat = 1;

		if (have_iostat)
			mgmt->forward = MOG_IOSTAT;
		else
			mog_mgmt_fn_watch_err(mgmt);
		fbreak;
	};
	aio_threads = (
		"server aio_threads"i ' '?'='(' ')?
		(digit+) > { mgmt->mark[0] = fpc - buf; }
		eor > { mgmt->mark[1] = fpc - buf; }
		@ { mog_mgmt_fn_aio_threads(mgmt, buf); fbreak; }
	);
	blank = [ \t]* eor @ { mog_mgmt_fn_blank(mgmt); fbreak; };
	shutdown = "shutdown" (" "+"graceful")? eor @ {
		cmogstored_quit();
		fbreak;
	};

	command = (digest|size|watch|aio_threads|shutdown|blank);
	main := command $! {
		p = buf;
		fhold;
		fgoto invalid_line;
	};
}%%

%% write data;

void mog_mgmt_reset_parser(struct mog_mgmt *mgmt)
{
	int cs;
	%% write init;
	mgmt->cs = cs;
	mgmt->mark[0] = mgmt->mark[1] = 0;
}

void mog_mgmt_init(struct mog_mgmt *mgmt, struct mog_svc *svc)
{
	memset(mgmt, 0, sizeof(struct mog_mgmt));
	mog_mgmt_reset_parser(mgmt);
	mgmt->svc = svc;
}

enum mog_parser_state
mog_mgmt_parse(struct mog_mgmt *mgmt, char *buf, size_t len)
{
	char *p, *pe, *eof = NULL;
	int cs = mgmt->cs;
	int really_done = 0;
	size_t off = mgmt->offset;

	assert(mgmt->wbuf == NULL && "unwritten data in buffer");
	assert(off <= len && "mgmt offset past end of buffer");

	p = buf + off;
	pe = buf + len;

	assert((void *)(pe - p) == (void *)(len - off) &&
	       "pointers aren't same distance");

	%% write exec;

	if (really_done)
		cs = mgmt_parser_first_final;

	mgmt->cs = cs;
	mgmt->offset = p - buf;

	if (cs == mgmt_parser_error)
		return MOG_PARSER_ERROR;

	assert(p <= pe && "buffer overflow after mgmt parse");
	assert(mgmt->offset <= len && "offset longer than len");

	if (mgmt->cs == mgmt_parser_first_final) return MOG_PARSER_DONE;
	return MOG_PARSER_CONTINUE;
}
