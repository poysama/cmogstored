/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
%%{
	machine listen_parser_common;

	ipv4 = (digit+ '.' digit+ '.' digit+ '.' digit+)
		> { mark_beg = fpc; }
		@ { mark_len = fpc - mark_beg + 1; };
	port = (digit+)
		> { port_beg = fpc; }
		@ { port_len = fpc - port_beg + 1; };

	listen = (((ipv4)? ':')? port ) $! {
		syslog(LOG_ERR, "bad character in IPv4 address: %c", fc);
	};
}%%
