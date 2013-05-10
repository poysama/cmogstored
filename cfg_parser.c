
#line 1 "cfg_parser.rl"
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


#line 102 "cfg_parser.rl"



#line 32 "cfg_parser.c"
static const int cfg_parser_start = 1;
static const int cfg_parser_first_final = 124;
static const int cfg_parser_error = 0;

static const int cfg_parser_en_ignored_line = 123;
static const int cfg_parser_en_main = 1;


#line 105 "cfg_parser.rl"

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

	
#line 55 "cfg_parser.c"
	{
	cs = cfg_parser_start;
	}

#line 118 "cfg_parser.rl"

	p = buf;
	pe = buf + len;

	
#line 66 "cfg_parser.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
	switch( (*p) ) {
		case 9: goto st1;
		case 32: goto st1;
		case 100: goto st2;
		case 104: goto st13;
		case 109: goto st56;
		case 112: goto st89;
		case 115: goto st99;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st1;
	goto tr0;
tr0:
#line 98 "cfg_parser.rl"
	{
			p--;
			{goto st123;}
		}
	goto st0;
tr33:
#line 15 "listen_parser_common.rl"
	{
		syslog(LOG_ERR, "bad character in IPv4 address: %c", (*p));
	}
#line 98 "cfg_parser.rl"
	{
			p--;
			{goto st123;}
		}
	goto st0;
#line 106 "cfg_parser.c"
st0:
cs = 0;
	goto _out;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	switch( (*p) ) {
		case 97: goto st3;
		case 111: goto st114;
	}
	goto tr0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	if ( (*p) == 101 )
		goto st4;
	goto tr0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	if ( (*p) == 109 )
		goto st5;
	goto tr0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( (*p) == 111 )
		goto st6;
	goto tr0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	if ( (*p) == 110 )
		goto st7;
	goto tr0;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( (*p) == 105 )
		goto st8;
	goto tr0;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	if ( (*p) == 122 )
		goto st9;
	goto tr0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	if ( (*p) == 101 )
		goto st10;
	goto tr0;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	switch( (*p) ) {
		case 10: goto tr17;
		case 32: goto st11;
		case 35: goto st12;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st11;
	goto tr0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	switch( (*p) ) {
		case 9: goto st11;
		case 32: goto st11;
		case 35: goto st12;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st11;
	goto tr0;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	if ( (*p) == 10 )
		goto tr17;
	goto st12;
tr17:
#line 66 "cfg_parser.rl"
	{ cfg->daemonize = true; }
	goto st124;
tr37:
#line 50 "cfg_parser.rl"
	{
		a = mog_listen_parse_internal(mark_beg, mark_len,
		                              port_beg, port_len);
		if (!a) return -1;
		cfg->httpgetlisten = a;
	}
	goto st124;
tr57:
#line 44 "cfg_parser.rl"
	{
		a = mog_listen_parse_internal(mark_beg, mark_len,
		                              port_beg, port_len);
		if (!a) return -1;
		cfg->httplisten = a;
	}
	goto st124;
tr79:
#line 69 "cfg_parser.rl"
	{
			mark_len = p - mark_beg;
			mark_beg[mark_len] = 0;
			errno = 0;
			cfg->maxconns = strtoul(mark_beg, NULL, 10);
			if (errno) {
				syslog(LOG_ERR,
				       "failed to parse: maxconns = %s - %m",
				       mark_beg);
				return -1;
			}
		}
	goto st124;
tr97:
#line 37 "cfg_parser.rl"
	{
		a = mog_listen_parse_internal(mark_beg, mark_len,
		                              port_beg, port_len);
		if (!a) return -1;
		cfg->mgmtlisten = a;
	}
	goto st124;
tr118:
#line 61 "cfg_parser.rl"
	{
		/* delay realpath(3) until svc init, symlinks may change */
		cfg->pidfile = mystrdup("pidfile", mark_beg, p);
		if (!cfg->pidfile) return -1;
	}
	goto st124;
tr132:
#line 83 "cfg_parser.rl"
	{
			cfg->server = mystrdup("server", mark_beg, p);
			if (!cfg->server) return -1;
			mog_cfg_check_server(cfg);
		}
	goto st124;
tr141:
#line 89 "cfg_parser.rl"
	{
			char *tmp = mystrdup("serverbin", mark_beg, p);
			if (!tmp) return -1;
			warn("W: serverbin = %s ignored", tmp);
			free(tmp);
		}
	goto st124;
tr154:
#line 56 "cfg_parser.rl"
	{
		/* delay realpath(3) until svc init, symlinks may change */
		cfg->docroot = mystrdup("docroot", mark_beg, p);
		if (!cfg->docroot) return -1;
	}
	goto st124;
st124:
	if ( ++p == pe )
		goto _test_eof124;
case 124:
#line 282 "cfg_parser.c"
	switch( (*p) ) {
		case 9: goto st1;
		case 32: goto st1;
		case 100: goto st2;
		case 104: goto st13;
		case 109: goto st56;
		case 112: goto st89;
		case 115: goto st99;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st1;
	goto tr0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	if ( (*p) == 116 )
		goto st14;
	goto tr0;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	if ( (*p) == 116 )
		goto st15;
	goto tr0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	if ( (*p) == 112 )
		goto st16;
	goto tr0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	switch( (*p) ) {
		case 103: goto st17;
		case 108: goto st38;
	}
	goto tr0;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	if ( (*p) == 101 )
		goto st18;
	goto tr0;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	if ( (*p) == 116 )
		goto st19;
	goto tr0;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
	if ( (*p) == 108 )
		goto st20;
	goto tr0;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
	if ( (*p) == 105 )
		goto st21;
	goto tr0;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
	if ( (*p) == 115 )
		goto st22;
	goto tr0;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
	if ( (*p) == 116 )
		goto st23;
	goto tr0;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
	if ( (*p) == 101 )
		goto st24;
	goto tr0;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
	if ( (*p) == 110 )
		goto st25;
	goto tr0;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
	switch( (*p) ) {
		case 9: goto st25;
		case 32: goto st25;
		case 61: goto st26;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st25;
	goto tr0;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
	switch( (*p) ) {
		case 9: goto st26;
		case 32: goto st26;
		case 58: goto st36;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr34;
	} else if ( (*p) >= 11 )
		goto st26;
	goto tr33;
tr34:
#line 9 "listen_parser_common.rl"
	{ mark_beg = p; }
#line 12 "listen_parser_common.rl"
	{ port_beg = p; }
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st27;
tr40:
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st27;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
#line 424 "cfg_parser.c"
	switch( (*p) ) {
		case 10: goto tr37;
		case 32: goto st28;
		case 35: goto st29;
		case 46: goto st30;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr40;
	} else if ( (*p) >= 9 )
		goto st28;
	goto tr33;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
	switch( (*p) ) {
		case 9: goto st28;
		case 32: goto st28;
		case 35: goto st29;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st28;
	goto tr0;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
	if ( (*p) == 10 )
		goto tr37;
	goto st29;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st31;
	goto tr33;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
	if ( (*p) == 46 )
		goto st32;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st31;
	goto tr33;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st33;
	goto tr33;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
	if ( (*p) == 46 )
		goto st34;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st33;
	goto tr33;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr45;
	goto tr33;
tr45:
#line 10 "listen_parser_common.rl"
	{ mark_len = p - mark_beg + 1; }
	goto st35;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
#line 503 "cfg_parser.c"
	if ( (*p) == 58 )
		goto st36;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr45;
	goto tr33;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr46;
	goto tr33;
tr47:
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st37;
tr46:
#line 12 "listen_parser_common.rl"
	{ port_beg = p; }
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st37;
st37:
	if ( ++p == pe )
		goto _test_eof37;
case 37:
#line 530 "cfg_parser.c"
	switch( (*p) ) {
		case 10: goto tr37;
		case 32: goto st28;
		case 35: goto st29;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr47;
	} else if ( (*p) >= 9 )
		goto st28;
	goto tr33;
st38:
	if ( ++p == pe )
		goto _test_eof38;
case 38:
	if ( (*p) == 105 )
		goto st39;
	goto tr0;
st39:
	if ( ++p == pe )
		goto _test_eof39;
case 39:
	if ( (*p) == 115 )
		goto st40;
	goto tr0;
st40:
	if ( ++p == pe )
		goto _test_eof40;
case 40:
	if ( (*p) == 116 )
		goto st41;
	goto tr0;
st41:
	if ( ++p == pe )
		goto _test_eof41;
case 41:
	if ( (*p) == 101 )
		goto st42;
	goto tr0;
st42:
	if ( ++p == pe )
		goto _test_eof42;
case 42:
	if ( (*p) == 110 )
		goto st43;
	goto tr0;
st43:
	if ( ++p == pe )
		goto _test_eof43;
case 43:
	switch( (*p) ) {
		case 9: goto st43;
		case 32: goto st43;
		case 61: goto st44;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st43;
	goto tr0;
st44:
	if ( ++p == pe )
		goto _test_eof44;
case 44:
	switch( (*p) ) {
		case 9: goto st44;
		case 32: goto st44;
		case 58: goto st54;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr54;
	} else if ( (*p) >= 11 )
		goto st44;
	goto tr33;
tr54:
#line 9 "listen_parser_common.rl"
	{ mark_beg = p; }
#line 12 "listen_parser_common.rl"
	{ port_beg = p; }
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st45;
tr60:
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st45;
st45:
	if ( ++p == pe )
		goto _test_eof45;
case 45:
#line 620 "cfg_parser.c"
	switch( (*p) ) {
		case 10: goto tr57;
		case 32: goto st46;
		case 35: goto st47;
		case 46: goto st48;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr60;
	} else if ( (*p) >= 9 )
		goto st46;
	goto tr33;
st46:
	if ( ++p == pe )
		goto _test_eof46;
case 46:
	switch( (*p) ) {
		case 9: goto st46;
		case 32: goto st46;
		case 35: goto st47;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st46;
	goto tr0;
st47:
	if ( ++p == pe )
		goto _test_eof47;
case 47:
	if ( (*p) == 10 )
		goto tr57;
	goto st47;
st48:
	if ( ++p == pe )
		goto _test_eof48;
case 48:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st49;
	goto tr33;
st49:
	if ( ++p == pe )
		goto _test_eof49;
case 49:
	if ( (*p) == 46 )
		goto st50;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st49;
	goto tr33;
st50:
	if ( ++p == pe )
		goto _test_eof50;
case 50:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st51;
	goto tr33;
st51:
	if ( ++p == pe )
		goto _test_eof51;
case 51:
	if ( (*p) == 46 )
		goto st52;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st51;
	goto tr33;
st52:
	if ( ++p == pe )
		goto _test_eof52;
case 52:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr65;
	goto tr33;
tr65:
#line 10 "listen_parser_common.rl"
	{ mark_len = p - mark_beg + 1; }
	goto st53;
st53:
	if ( ++p == pe )
		goto _test_eof53;
case 53:
#line 699 "cfg_parser.c"
	if ( (*p) == 58 )
		goto st54;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr65;
	goto tr33;
st54:
	if ( ++p == pe )
		goto _test_eof54;
case 54:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr66;
	goto tr33;
tr67:
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st55;
tr66:
#line 12 "listen_parser_common.rl"
	{ port_beg = p; }
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st55;
st55:
	if ( ++p == pe )
		goto _test_eof55;
case 55:
#line 726 "cfg_parser.c"
	switch( (*p) ) {
		case 10: goto tr57;
		case 32: goto st46;
		case 35: goto st47;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr67;
	} else if ( (*p) >= 9 )
		goto st46;
	goto tr33;
st56:
	if ( ++p == pe )
		goto _test_eof56;
case 56:
	switch( (*p) ) {
		case 97: goto st57;
		case 103: goto st68;
	}
	goto tr0;
st57:
	if ( ++p == pe )
		goto _test_eof57;
case 57:
	if ( (*p) == 120 )
		goto st58;
	goto tr0;
st58:
	if ( ++p == pe )
		goto _test_eof58;
case 58:
	if ( (*p) == 99 )
		goto st59;
	goto tr0;
st59:
	if ( ++p == pe )
		goto _test_eof59;
case 59:
	if ( (*p) == 111 )
		goto st60;
	goto tr0;
st60:
	if ( ++p == pe )
		goto _test_eof60;
case 60:
	if ( (*p) == 110 )
		goto st61;
	goto tr0;
st61:
	if ( ++p == pe )
		goto _test_eof61;
case 61:
	if ( (*p) == 110 )
		goto st62;
	goto tr0;
st62:
	if ( ++p == pe )
		goto _test_eof62;
case 62:
	if ( (*p) == 115 )
		goto st63;
	goto tr0;
st63:
	if ( ++p == pe )
		goto _test_eof63;
case 63:
	switch( (*p) ) {
		case 9: goto st63;
		case 32: goto st63;
		case 61: goto st64;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st63;
	goto tr0;
st64:
	if ( ++p == pe )
		goto _test_eof64;
case 64:
	switch( (*p) ) {
		case 9: goto st64;
		case 32: goto st64;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr77;
	} else if ( (*p) >= 11 )
		goto st64;
	goto tr0;
tr77:
#line 68 "cfg_parser.rl"
	{ mark_beg = p; }
	goto st65;
st65:
	if ( ++p == pe )
		goto _test_eof65;
case 65:
#line 823 "cfg_parser.c"
	switch( (*p) ) {
		case 10: goto tr79;
		case 32: goto tr78;
		case 35: goto tr80;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st65;
	} else if ( (*p) >= 9 )
		goto tr78;
	goto tr0;
tr78:
#line 69 "cfg_parser.rl"
	{
			mark_len = p - mark_beg;
			mark_beg[mark_len] = 0;
			errno = 0;
			cfg->maxconns = strtoul(mark_beg, NULL, 10);
			if (errno) {
				syslog(LOG_ERR,
				       "failed to parse: maxconns = %s - %m",
				       mark_beg);
				return -1;
			}
		}
	goto st66;
tr117:
#line 61 "cfg_parser.rl"
	{
		/* delay realpath(3) until svc init, symlinks may change */
		cfg->pidfile = mystrdup("pidfile", mark_beg, p);
		if (!cfg->pidfile) return -1;
	}
	goto st66;
tr131:
#line 83 "cfg_parser.rl"
	{
			cfg->server = mystrdup("server", mark_beg, p);
			if (!cfg->server) return -1;
			mog_cfg_check_server(cfg);
		}
	goto st66;
tr140:
#line 89 "cfg_parser.rl"
	{
			char *tmp = mystrdup("serverbin", mark_beg, p);
			if (!tmp) return -1;
			warn("W: serverbin = %s ignored", tmp);
			free(tmp);
		}
	goto st66;
tr153:
#line 56 "cfg_parser.rl"
	{
		/* delay realpath(3) until svc init, symlinks may change */
		cfg->docroot = mystrdup("docroot", mark_beg, p);
		if (!cfg->docroot) return -1;
	}
	goto st66;
st66:
	if ( ++p == pe )
		goto _test_eof66;
case 66:
#line 887 "cfg_parser.c"
	switch( (*p) ) {
		case 9: goto st66;
		case 32: goto st66;
		case 35: goto st67;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st66;
	goto tr0;
tr80:
#line 69 "cfg_parser.rl"
	{
			mark_len = p - mark_beg;
			mark_beg[mark_len] = 0;
			errno = 0;
			cfg->maxconns = strtoul(mark_beg, NULL, 10);
			if (errno) {
				syslog(LOG_ERR,
				       "failed to parse: maxconns = %s - %m",
				       mark_beg);
				return -1;
			}
		}
	goto st67;
tr121:
#line 61 "cfg_parser.rl"
	{
		/* delay realpath(3) until svc init, symlinks may change */
		cfg->pidfile = mystrdup("pidfile", mark_beg, p);
		if (!cfg->pidfile) return -1;
	}
	goto st67;
tr133:
#line 83 "cfg_parser.rl"
	{
			cfg->server = mystrdup("server", mark_beg, p);
			if (!cfg->server) return -1;
			mog_cfg_check_server(cfg);
		}
	goto st67;
tr144:
#line 89 "cfg_parser.rl"
	{
			char *tmp = mystrdup("serverbin", mark_beg, p);
			if (!tmp) return -1;
			warn("W: serverbin = %s ignored", tmp);
			free(tmp);
		}
	goto st67;
tr157:
#line 56 "cfg_parser.rl"
	{
		/* delay realpath(3) until svc init, symlinks may change */
		cfg->docroot = mystrdup("docroot", mark_beg, p);
		if (!cfg->docroot) return -1;
	}
	goto st67;
st67:
	if ( ++p == pe )
		goto _test_eof67;
case 67:
#line 948 "cfg_parser.c"
	if ( (*p) == 10 )
		goto st124;
	goto st67;
st68:
	if ( ++p == pe )
		goto _test_eof68;
case 68:
	if ( (*p) == 109 )
		goto st69;
	goto tr0;
st69:
	if ( ++p == pe )
		goto _test_eof69;
case 69:
	if ( (*p) == 116 )
		goto st70;
	goto tr0;
st70:
	if ( ++p == pe )
		goto _test_eof70;
case 70:
	if ( (*p) == 108 )
		goto st71;
	goto tr0;
st71:
	if ( ++p == pe )
		goto _test_eof71;
case 71:
	if ( (*p) == 105 )
		goto st72;
	goto tr0;
st72:
	if ( ++p == pe )
		goto _test_eof72;
case 72:
	if ( (*p) == 115 )
		goto st73;
	goto tr0;
st73:
	if ( ++p == pe )
		goto _test_eof73;
case 73:
	if ( (*p) == 116 )
		goto st74;
	goto tr0;
st74:
	if ( ++p == pe )
		goto _test_eof74;
case 74:
	if ( (*p) == 101 )
		goto st75;
	goto tr0;
st75:
	if ( ++p == pe )
		goto _test_eof75;
case 75:
	if ( (*p) == 110 )
		goto st76;
	goto tr0;
st76:
	if ( ++p == pe )
		goto _test_eof76;
case 76:
	switch( (*p) ) {
		case 9: goto st76;
		case 32: goto st76;
		case 61: goto st77;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st76;
	goto tr0;
st77:
	if ( ++p == pe )
		goto _test_eof77;
case 77:
	switch( (*p) ) {
		case 9: goto st77;
		case 32: goto st77;
		case 58: goto st87;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr94;
	} else if ( (*p) >= 11 )
		goto st77;
	goto tr33;
tr94:
#line 9 "listen_parser_common.rl"
	{ mark_beg = p; }
#line 12 "listen_parser_common.rl"
	{ port_beg = p; }
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st78;
tr100:
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st78;
st78:
	if ( ++p == pe )
		goto _test_eof78;
case 78:
#line 1051 "cfg_parser.c"
	switch( (*p) ) {
		case 10: goto tr97;
		case 32: goto st79;
		case 35: goto st80;
		case 46: goto st81;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr100;
	} else if ( (*p) >= 9 )
		goto st79;
	goto tr33;
st79:
	if ( ++p == pe )
		goto _test_eof79;
case 79:
	switch( (*p) ) {
		case 9: goto st79;
		case 32: goto st79;
		case 35: goto st80;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st79;
	goto tr0;
st80:
	if ( ++p == pe )
		goto _test_eof80;
case 80:
	if ( (*p) == 10 )
		goto tr97;
	goto st80;
st81:
	if ( ++p == pe )
		goto _test_eof81;
case 81:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st82;
	goto tr33;
st82:
	if ( ++p == pe )
		goto _test_eof82;
case 82:
	if ( (*p) == 46 )
		goto st83;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st82;
	goto tr33;
st83:
	if ( ++p == pe )
		goto _test_eof83;
case 83:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st84;
	goto tr33;
st84:
	if ( ++p == pe )
		goto _test_eof84;
case 84:
	if ( (*p) == 46 )
		goto st85;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st84;
	goto tr33;
st85:
	if ( ++p == pe )
		goto _test_eof85;
case 85:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr105;
	goto tr33;
tr105:
#line 10 "listen_parser_common.rl"
	{ mark_len = p - mark_beg + 1; }
	goto st86;
st86:
	if ( ++p == pe )
		goto _test_eof86;
case 86:
#line 1130 "cfg_parser.c"
	if ( (*p) == 58 )
		goto st87;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr105;
	goto tr33;
st87:
	if ( ++p == pe )
		goto _test_eof87;
case 87:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr106;
	goto tr33;
tr107:
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st88;
tr106:
#line 12 "listen_parser_common.rl"
	{ port_beg = p; }
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st88;
st88:
	if ( ++p == pe )
		goto _test_eof88;
case 88:
#line 1157 "cfg_parser.c"
	switch( (*p) ) {
		case 10: goto tr97;
		case 32: goto st79;
		case 35: goto st80;
	}
	if ( (*p) > 13 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr107;
	} else if ( (*p) >= 9 )
		goto st79;
	goto tr33;
st89:
	if ( ++p == pe )
		goto _test_eof89;
case 89:
	if ( (*p) == 105 )
		goto st90;
	goto tr0;
st90:
	if ( ++p == pe )
		goto _test_eof90;
case 90:
	if ( (*p) == 100 )
		goto st91;
	goto tr0;
st91:
	if ( ++p == pe )
		goto _test_eof91;
case 91:
	if ( (*p) == 102 )
		goto st92;
	goto tr0;
st92:
	if ( ++p == pe )
		goto _test_eof92;
case 92:
	if ( (*p) == 105 )
		goto st93;
	goto tr0;
st93:
	if ( ++p == pe )
		goto _test_eof93;
case 93:
	if ( (*p) == 108 )
		goto st94;
	goto tr0;
st94:
	if ( ++p == pe )
		goto _test_eof94;
case 94:
	if ( (*p) == 101 )
		goto st95;
	goto tr0;
st95:
	if ( ++p == pe )
		goto _test_eof95;
case 95:
	switch( (*p) ) {
		case 9: goto st95;
		case 32: goto st95;
		case 61: goto st96;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st95;
	goto tr0;
st96:
	if ( ++p == pe )
		goto _test_eof96;
case 96:
	switch( (*p) ) {
		case 10: goto tr0;
		case 32: goto st96;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st96;
	goto tr115;
tr115:
#line 35 "cfg_parser.rl"
	{ mark_beg = p; }
	goto st97;
st97:
	if ( ++p == pe )
		goto _test_eof97;
case 97:
#line 1242 "cfg_parser.c"
	switch( (*p) ) {
		case 10: goto tr118;
		case 32: goto tr117;
		case 35: goto tr119;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr117;
	goto st97;
tr119:
#line 61 "cfg_parser.rl"
	{
		/* delay realpath(3) until svc init, symlinks may change */
		cfg->pidfile = mystrdup("pidfile", mark_beg, p);
		if (!cfg->pidfile) return -1;
	}
	goto st98;
st98:
	if ( ++p == pe )
		goto _test_eof98;
case 98:
#line 1263 "cfg_parser.c"
	switch( (*p) ) {
		case 10: goto tr118;
		case 32: goto tr121;
		case 35: goto tr119;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr121;
	goto st98;
st99:
	if ( ++p == pe )
		goto _test_eof99;
case 99:
	if ( (*p) == 101 )
		goto st100;
	goto tr0;
st100:
	if ( ++p == pe )
		goto _test_eof100;
case 100:
	if ( (*p) == 114 )
		goto st101;
	goto tr0;
st101:
	if ( ++p == pe )
		goto _test_eof101;
case 101:
	if ( (*p) == 118 )
		goto st102;
	goto tr0;
st102:
	if ( ++p == pe )
		goto _test_eof102;
case 102:
	if ( (*p) == 101 )
		goto st103;
	goto tr0;
st103:
	if ( ++p == pe )
		goto _test_eof103;
case 103:
	if ( (*p) == 114 )
		goto st104;
	goto tr0;
st104:
	if ( ++p == pe )
		goto _test_eof104;
case 104:
	switch( (*p) ) {
		case 9: goto st105;
		case 32: goto st105;
		case 61: goto st106;
		case 98: goto st108;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st105;
	goto tr0;
st105:
	if ( ++p == pe )
		goto _test_eof105;
case 105:
	switch( (*p) ) {
		case 9: goto st105;
		case 32: goto st105;
		case 61: goto st106;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st105;
	goto tr0;
st106:
	if ( ++p == pe )
		goto _test_eof106;
case 106:
	switch( (*p) ) {
		case 9: goto st106;
		case 32: goto st106;
	}
	if ( (*p) < 65 ) {
		if ( 11 <= (*p) && (*p) <= 13 )
			goto st106;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr130;
	} else
		goto tr130;
	goto tr0;
tr130:
#line 82 "cfg_parser.rl"
	{ mark_beg = p; }
	goto st107;
st107:
	if ( ++p == pe )
		goto _test_eof107;
case 107:
#line 1357 "cfg_parser.c"
	switch( (*p) ) {
		case 10: goto tr132;
		case 32: goto tr131;
		case 35: goto tr133;
	}
	if ( (*p) < 65 ) {
		if ( 9 <= (*p) && (*p) <= 13 )
			goto tr131;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st107;
	} else
		goto st107;
	goto tr0;
st108:
	if ( ++p == pe )
		goto _test_eof108;
case 108:
	if ( (*p) == 105 )
		goto st109;
	goto tr0;
st109:
	if ( ++p == pe )
		goto _test_eof109;
case 109:
	if ( (*p) == 110 )
		goto st110;
	goto tr0;
st110:
	if ( ++p == pe )
		goto _test_eof110;
case 110:
	switch( (*p) ) {
		case 9: goto st110;
		case 32: goto st110;
		case 61: goto st111;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st110;
	goto tr0;
st111:
	if ( ++p == pe )
		goto _test_eof111;
case 111:
	switch( (*p) ) {
		case 10: goto tr0;
		case 32: goto st111;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st111;
	goto tr138;
tr138:
#line 35 "cfg_parser.rl"
	{ mark_beg = p; }
	goto st112;
st112:
	if ( ++p == pe )
		goto _test_eof112;
case 112:
#line 1417 "cfg_parser.c"
	switch( (*p) ) {
		case 10: goto tr141;
		case 32: goto tr140;
		case 35: goto tr142;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr140;
	goto st112;
tr142:
#line 89 "cfg_parser.rl"
	{
			char *tmp = mystrdup("serverbin", mark_beg, p);
			if (!tmp) return -1;
			warn("W: serverbin = %s ignored", tmp);
			free(tmp);
		}
	goto st113;
st113:
	if ( ++p == pe )
		goto _test_eof113;
case 113:
#line 1439 "cfg_parser.c"
	switch( (*p) ) {
		case 10: goto tr141;
		case 32: goto tr144;
		case 35: goto tr142;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr144;
	goto st113;
st114:
	if ( ++p == pe )
		goto _test_eof114;
case 114:
	if ( (*p) == 99 )
		goto st115;
	goto tr0;
st115:
	if ( ++p == pe )
		goto _test_eof115;
case 115:
	if ( (*p) == 114 )
		goto st116;
	goto tr0;
st116:
	if ( ++p == pe )
		goto _test_eof116;
case 116:
	if ( (*p) == 111 )
		goto st117;
	goto tr0;
st117:
	if ( ++p == pe )
		goto _test_eof117;
case 117:
	if ( (*p) == 111 )
		goto st118;
	goto tr0;
st118:
	if ( ++p == pe )
		goto _test_eof118;
case 118:
	if ( (*p) == 116 )
		goto st119;
	goto tr0;
st119:
	if ( ++p == pe )
		goto _test_eof119;
case 119:
	switch( (*p) ) {
		case 9: goto st119;
		case 32: goto st119;
		case 61: goto st120;
	}
	if ( 11 <= (*p) && (*p) <= 13 )
		goto st119;
	goto tr0;
st120:
	if ( ++p == pe )
		goto _test_eof120;
case 120:
	switch( (*p) ) {
		case 10: goto tr0;
		case 32: goto st120;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto st120;
	goto tr151;
tr151:
#line 35 "cfg_parser.rl"
	{ mark_beg = p; }
	goto st121;
st121:
	if ( ++p == pe )
		goto _test_eof121;
case 121:
#line 1514 "cfg_parser.c"
	switch( (*p) ) {
		case 10: goto tr154;
		case 32: goto tr153;
		case 35: goto tr155;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr153;
	goto st121;
tr155:
#line 56 "cfg_parser.rl"
	{
		/* delay realpath(3) until svc init, symlinks may change */
		cfg->docroot = mystrdup("docroot", mark_beg, p);
		if (!cfg->docroot) return -1;
	}
	goto st122;
st122:
	if ( ++p == pe )
		goto _test_eof122;
case 122:
#line 1535 "cfg_parser.c"
	switch( (*p) ) {
		case 10: goto tr154;
		case 32: goto tr157;
		case 35: goto tr155;
	}
	if ( 9 <= (*p) && (*p) <= 13 )
		goto tr157;
	goto st122;
st123:
	if ( ++p == pe )
		goto _test_eof123;
case 123:
	if ( (*p) == 10 )
		goto tr159;
	goto st123;
tr159:
#line 29 "cfg_parser.rl"
	{ {goto st1;} }
	goto st125;
st125:
	if ( ++p == pe )
		goto _test_eof125;
case 125:
#line 1559 "cfg_parser.c"
	goto st0;
	}
	_test_eof1: cs = 1; goto _test_eof; 
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 
	_test_eof11: cs = 11; goto _test_eof; 
	_test_eof12: cs = 12; goto _test_eof; 
	_test_eof124: cs = 124; goto _test_eof; 
	_test_eof13: cs = 13; goto _test_eof; 
	_test_eof14: cs = 14; goto _test_eof; 
	_test_eof15: cs = 15; goto _test_eof; 
	_test_eof16: cs = 16; goto _test_eof; 
	_test_eof17: cs = 17; goto _test_eof; 
	_test_eof18: cs = 18; goto _test_eof; 
	_test_eof19: cs = 19; goto _test_eof; 
	_test_eof20: cs = 20; goto _test_eof; 
	_test_eof21: cs = 21; goto _test_eof; 
	_test_eof22: cs = 22; goto _test_eof; 
	_test_eof23: cs = 23; goto _test_eof; 
	_test_eof24: cs = 24; goto _test_eof; 
	_test_eof25: cs = 25; goto _test_eof; 
	_test_eof26: cs = 26; goto _test_eof; 
	_test_eof27: cs = 27; goto _test_eof; 
	_test_eof28: cs = 28; goto _test_eof; 
	_test_eof29: cs = 29; goto _test_eof; 
	_test_eof30: cs = 30; goto _test_eof; 
	_test_eof31: cs = 31; goto _test_eof; 
	_test_eof32: cs = 32; goto _test_eof; 
	_test_eof33: cs = 33; goto _test_eof; 
	_test_eof34: cs = 34; goto _test_eof; 
	_test_eof35: cs = 35; goto _test_eof; 
	_test_eof36: cs = 36; goto _test_eof; 
	_test_eof37: cs = 37; goto _test_eof; 
	_test_eof38: cs = 38; goto _test_eof; 
	_test_eof39: cs = 39; goto _test_eof; 
	_test_eof40: cs = 40; goto _test_eof; 
	_test_eof41: cs = 41; goto _test_eof; 
	_test_eof42: cs = 42; goto _test_eof; 
	_test_eof43: cs = 43; goto _test_eof; 
	_test_eof44: cs = 44; goto _test_eof; 
	_test_eof45: cs = 45; goto _test_eof; 
	_test_eof46: cs = 46; goto _test_eof; 
	_test_eof47: cs = 47; goto _test_eof; 
	_test_eof48: cs = 48; goto _test_eof; 
	_test_eof49: cs = 49; goto _test_eof; 
	_test_eof50: cs = 50; goto _test_eof; 
	_test_eof51: cs = 51; goto _test_eof; 
	_test_eof52: cs = 52; goto _test_eof; 
	_test_eof53: cs = 53; goto _test_eof; 
	_test_eof54: cs = 54; goto _test_eof; 
	_test_eof55: cs = 55; goto _test_eof; 
	_test_eof56: cs = 56; goto _test_eof; 
	_test_eof57: cs = 57; goto _test_eof; 
	_test_eof58: cs = 58; goto _test_eof; 
	_test_eof59: cs = 59; goto _test_eof; 
	_test_eof60: cs = 60; goto _test_eof; 
	_test_eof61: cs = 61; goto _test_eof; 
	_test_eof62: cs = 62; goto _test_eof; 
	_test_eof63: cs = 63; goto _test_eof; 
	_test_eof64: cs = 64; goto _test_eof; 
	_test_eof65: cs = 65; goto _test_eof; 
	_test_eof66: cs = 66; goto _test_eof; 
	_test_eof67: cs = 67; goto _test_eof; 
	_test_eof68: cs = 68; goto _test_eof; 
	_test_eof69: cs = 69; goto _test_eof; 
	_test_eof70: cs = 70; goto _test_eof; 
	_test_eof71: cs = 71; goto _test_eof; 
	_test_eof72: cs = 72; goto _test_eof; 
	_test_eof73: cs = 73; goto _test_eof; 
	_test_eof74: cs = 74; goto _test_eof; 
	_test_eof75: cs = 75; goto _test_eof; 
	_test_eof76: cs = 76; goto _test_eof; 
	_test_eof77: cs = 77; goto _test_eof; 
	_test_eof78: cs = 78; goto _test_eof; 
	_test_eof79: cs = 79; goto _test_eof; 
	_test_eof80: cs = 80; goto _test_eof; 
	_test_eof81: cs = 81; goto _test_eof; 
	_test_eof82: cs = 82; goto _test_eof; 
	_test_eof83: cs = 83; goto _test_eof; 
	_test_eof84: cs = 84; goto _test_eof; 
	_test_eof85: cs = 85; goto _test_eof; 
	_test_eof86: cs = 86; goto _test_eof; 
	_test_eof87: cs = 87; goto _test_eof; 
	_test_eof88: cs = 88; goto _test_eof; 
	_test_eof89: cs = 89; goto _test_eof; 
	_test_eof90: cs = 90; goto _test_eof; 
	_test_eof91: cs = 91; goto _test_eof; 
	_test_eof92: cs = 92; goto _test_eof; 
	_test_eof93: cs = 93; goto _test_eof; 
	_test_eof94: cs = 94; goto _test_eof; 
	_test_eof95: cs = 95; goto _test_eof; 
	_test_eof96: cs = 96; goto _test_eof; 
	_test_eof97: cs = 97; goto _test_eof; 
	_test_eof98: cs = 98; goto _test_eof; 
	_test_eof99: cs = 99; goto _test_eof; 
	_test_eof100: cs = 100; goto _test_eof; 
	_test_eof101: cs = 101; goto _test_eof; 
	_test_eof102: cs = 102; goto _test_eof; 
	_test_eof103: cs = 103; goto _test_eof; 
	_test_eof104: cs = 104; goto _test_eof; 
	_test_eof105: cs = 105; goto _test_eof; 
	_test_eof106: cs = 106; goto _test_eof; 
	_test_eof107: cs = 107; goto _test_eof; 
	_test_eof108: cs = 108; goto _test_eof; 
	_test_eof109: cs = 109; goto _test_eof; 
	_test_eof110: cs = 110; goto _test_eof; 
	_test_eof111: cs = 111; goto _test_eof; 
	_test_eof112: cs = 112; goto _test_eof; 
	_test_eof113: cs = 113; goto _test_eof; 
	_test_eof114: cs = 114; goto _test_eof; 
	_test_eof115: cs = 115; goto _test_eof; 
	_test_eof116: cs = 116; goto _test_eof; 
	_test_eof117: cs = 117; goto _test_eof; 
	_test_eof118: cs = 118; goto _test_eof; 
	_test_eof119: cs = 119; goto _test_eof; 
	_test_eof120: cs = 120; goto _test_eof; 
	_test_eof121: cs = 121; goto _test_eof; 
	_test_eof122: cs = 122; goto _test_eof; 
	_test_eof123: cs = 123; goto _test_eof; 
	_test_eof125: cs = 125; goto _test_eof; 

	_test_eof: {}
	if ( p == eof )
	{
	switch ( cs ) {
	case 1: 
	case 2: 
	case 3: 
	case 4: 
	case 5: 
	case 6: 
	case 7: 
	case 8: 
	case 9: 
	case 10: 
	case 11: 
	case 12: 
	case 13: 
	case 14: 
	case 15: 
	case 16: 
	case 17: 
	case 18: 
	case 19: 
	case 20: 
	case 21: 
	case 22: 
	case 23: 
	case 24: 
	case 25: 
	case 28: 
	case 29: 
	case 38: 
	case 39: 
	case 40: 
	case 41: 
	case 42: 
	case 43: 
	case 46: 
	case 47: 
	case 56: 
	case 57: 
	case 58: 
	case 59: 
	case 60: 
	case 61: 
	case 62: 
	case 63: 
	case 64: 
	case 65: 
	case 66: 
	case 67: 
	case 68: 
	case 69: 
	case 70: 
	case 71: 
	case 72: 
	case 73: 
	case 74: 
	case 75: 
	case 76: 
	case 79: 
	case 80: 
	case 89: 
	case 90: 
	case 91: 
	case 92: 
	case 93: 
	case 94: 
	case 95: 
	case 96: 
	case 97: 
	case 98: 
	case 99: 
	case 100: 
	case 101: 
	case 102: 
	case 103: 
	case 104: 
	case 105: 
	case 106: 
	case 107: 
	case 108: 
	case 109: 
	case 110: 
	case 111: 
	case 112: 
	case 113: 
	case 114: 
	case 115: 
	case 116: 
	case 117: 
	case 118: 
	case 119: 
	case 120: 
	case 121: 
	case 122: 
#line 98 "cfg_parser.rl"
	{
			p--;
			{goto st123;}
		}
	break;
	case 26: 
	case 27: 
	case 30: 
	case 31: 
	case 32: 
	case 33: 
	case 34: 
	case 35: 
	case 36: 
	case 37: 
	case 44: 
	case 45: 
	case 48: 
	case 49: 
	case 50: 
	case 51: 
	case 52: 
	case 53: 
	case 54: 
	case 55: 
	case 77: 
	case 78: 
	case 81: 
	case 82: 
	case 83: 
	case 84: 
	case 85: 
	case 86: 
	case 87: 
	case 88: 
#line 15 "listen_parser_common.rl"
	{
		syslog(LOG_ERR, "bad character in IPv4 address: %c", (*p));
	}
#line 98 "cfg_parser.rl"
	{
			p--;
			{goto st123;}
		}
	break;
#line 1830 "cfg_parser.c"
	}
	}

	_out: {}
	}

#line 123 "cfg_parser.rl"

	if (cs == cfg_parser_error)
		return -1;

	assert(p <= pe && "buffer overflow after cfg parse");
	return 0;
}
