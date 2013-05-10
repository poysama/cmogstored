
#line 1 "listen_parser.rl"
/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "listen_parser.h"

#line 15 "listen_parser.rl"



#line 15 "listen_parser.c"
static const int listen_parser_start = 1;
static const int listen_parser_first_final = 11;
static const int listen_parser_error = 0;

static const int listen_parser_en_main = 1;


#line 18 "listen_parser.rl"

static struct mog_addrinfo *listen_parse(char *str)
{
	char *p, *pe, *eof = NULL;
	char *mark_beg = NULL;
	char *port_beg = NULL;
	size_t mark_len = 0;
	size_t port_len = 0;
	struct mog_addrinfo *a = NULL;
	int cs;

	
#line 36 "listen_parser.c"
	{
	cs = listen_parser_start;
	}

#line 30 "listen_parser.rl"

	p = str;
	pe = str + strlen(str) + 1;

	
#line 47 "listen_parser.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	if ( (*p) == 58 )
		goto st9;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr1;
	goto tr0;
tr0:
#line 15 "listen_parser_common.rl"
	{
		syslog(LOG_ERR, "bad character in IPv4 address: %c", (*p));
	}
	goto st0;
#line 65 "listen_parser.c"
st0:
cs = 0;
	goto _out;
tr1:
#line 9 "listen_parser_common.rl"
	{ mark_beg = p; }
#line 12 "listen_parser_common.rl"
	{ port_beg = p; }
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st2;
tr5:
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 85 "listen_parser.c"
	switch( (*p) ) {
		case 0: goto tr3;
		case 46: goto st3;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr5;
	goto tr0;
tr3:
#line 11 "listen_parser.rl"
	{
		a = mog_listen_parse_internal(mark_beg, mark_len,
		                              port_beg, port_len);
	}
	goto st11;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
#line 104 "listen_parser.c"
	goto st0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st4;
	goto tr0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	if ( (*p) == 46 )
		goto st5;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st4;
	goto tr0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st6;
	goto tr0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	if ( (*p) == 46 )
		goto st7;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st6;
	goto tr0;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr10;
	goto tr0;
tr10:
#line 10 "listen_parser_common.rl"
	{ mark_len = p - mark_beg + 1; }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 153 "listen_parser.c"
	if ( (*p) == 58 )
		goto st9;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr10;
	goto tr0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr11;
	goto tr0;
tr12:
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st10;
tr11:
#line 12 "listen_parser_common.rl"
	{ port_beg = p; }
#line 13 "listen_parser_common.rl"
	{ port_len = p - port_beg + 1; }
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 180 "listen_parser.c"
	if ( (*p) == 0 )
		goto tr3;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr12;
	goto tr0;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof11: cs = 11; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 

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
#line 15 "listen_parser_common.rl"
	{
		syslog(LOG_ERR, "bad character in IPv4 address: %c", (*p));
	}
	break;
#line 217 "listen_parser.c"
	}
	}

	_out: {}
	}

#line 35 "listen_parser.rl"

	if ((cs == listen_parser_error) && a)
		mog_addrinfo_free(&a);

	assert(p <= pe && "buffer overflow after listen parse");
	return a;
}

struct mog_addrinfo *mog_listen_parse(const char *str)
{
	char *tmp = xstrdup(str);
	struct mog_addrinfo *rv = listen_parse(tmp);
	free(tmp);

	return rv;
}
