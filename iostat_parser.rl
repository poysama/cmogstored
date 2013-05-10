/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

%%{
	machine iostat_parser;
	eor = '\n' @ { iostat->dev_tip = 0; };
	ignored_line := ( (any-'\n')* eor ) @ {
		if (iostat->ready) {
			iostat->ready = false;
			mog_iostat_commit();
		}
		fgoto main;
	};
	device_name = ([a-zA-Z0-9/]+){1,71} $ {
			if (iostat->dev_tip < (sizeof(iostat->dev)-1))
				iostat->dev[iostat->dev_tip++] = fc;
			};
	utilization = ([0-9\.]+){1,7} > { iostat->util_tip = 0; }
		 $ {
			if (iostat->util_tip < (sizeof(iostat->util)-1))
				iostat->util[iostat->util_tip++] = fc;
		};
	lws = (' '|'\t');
	stats = lws*
		(
			device_name
			lws+
		)

		# Skip the middle section for now, some folks may use
		# await/svctm here.  Not sure how standardized those
		# fields are on non-Linux platforms...
		(any - '\n')*
		lws+

		utilization
		lws*
		eor > {
			mog_iostat_line_done(iostat);
			iostat->ready = true;
		};
	main := (stats $! { fhold; fgoto ignored_line; })+;
}%%

%% write data;

void mog_iostat_init(struct mog_iostat *iostat)
{
	int cs;
	struct mog_queue *queue = iostat->queue;

	memset(iostat, 0, sizeof(struct mog_iostat));
	%% write init;
	iostat->cs = cs;
	iostat->queue = queue;
}

enum mog_parser_state
mog_iostat_parse(struct mog_iostat *iostat, char *buf, size_t len)
{
	char *p, *pe, *eof = NULL;
	int cs = iostat->cs;

	if (cs == iostat_parser_first_final)
		return MOG_PARSER_DONE;

	p = buf;
	pe = buf + len;

	%% write exec;

	iostat->cs = cs;

	if (cs == iostat_parser_error)
		return MOG_PARSER_ERROR;

	assert(p <= pe && "buffer overflow after iostat parse");

	if (cs == iostat_parser_first_final)
		return MOG_PARSER_DONE;

	return MOG_PARSER_CONTINUE;
}
