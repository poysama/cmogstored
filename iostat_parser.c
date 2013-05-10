
#line 1 "iostat_parser.rl"
/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"


#line 46 "iostat_parser.rl"



#line 15 "iostat_parser.c"
static const int iostat_parser_start = 1;
static const int iostat_parser_first_final = 8;
static const int iostat_parser_error = 0;

static const int iostat_parser_en_ignored_line = 7;
static const int iostat_parser_en_main = 1;


#line 49 "iostat_parser.rl"

void mog_iostat_init(struct mog_iostat *iostat)
{
	int cs;
	struct mog_queue *queue = iostat->queue;

	memset(iostat, 0, sizeof(struct mog_iostat));
	
#line 33 "iostat_parser.c"
	{
	cs = iostat_parser_start;
	}

#line 57 "iostat_parser.rl"
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

	
#line 56 "iostat_parser.c"
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
	}
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto tr2;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr2;
	} else
		goto tr2;
	goto tr0;
tr0:
#line 45 "iostat_parser.rl"
	{ p--; {goto st7;} }
	goto st0;
#line 83 "iostat_parser.c"
st0:
cs = 0;
	goto _out;
tr2:
#line 17 "iostat_parser.rl"
	{
			if (iostat->dev_tip < (sizeof(iostat->dev)-1))
				iostat->dev[iostat->dev_tip++] = (*p);
			}
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 98 "iostat_parser.c"
	switch( (*p) ) {
		case 9: goto st3;
		case 32: goto st3;
	}
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto tr2;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr2;
	} else
		goto tr2;
	goto tr0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	switch( (*p) ) {
		case 9: goto st4;
		case 10: goto tr0;
		case 32: goto st4;
	}
	goto st3;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	switch( (*p) ) {
		case 9: goto st4;
		case 10: goto tr0;
		case 32: goto st4;
		case 46: goto tr5;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr5;
	goto st3;
tr5:
#line 21 "iostat_parser.rl"
	{ iostat->util_tip = 0; }
#line 22 "iostat_parser.rl"
	{
			if (iostat->util_tip < (sizeof(iostat->util)-1))
				iostat->util[iostat->util_tip++] = (*p);
		}
	goto st5;
tr8:
#line 22 "iostat_parser.rl"
	{
			if (iostat->util_tip < (sizeof(iostat->util)-1))
				iostat->util[iostat->util_tip++] = (*p);
		}
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
#line 155 "iostat_parser.c"
	switch( (*p) ) {
		case 9: goto st6;
		case 10: goto tr7;
		case 32: goto st6;
		case 46: goto tr8;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr8;
	goto st3;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	switch( (*p) ) {
		case 9: goto st6;
		case 10: goto tr7;
		case 32: goto st6;
		case 46: goto tr5;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr5;
	goto st3;
tr7:
#line 41 "iostat_parser.rl"
	{
			mog_iostat_line_done(iostat);
			iostat->ready = true;
		}
#line 9 "iostat_parser.rl"
	{ iostat->dev_tip = 0; }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 191 "iostat_parser.c"
	switch( (*p) ) {
		case 9: goto st1;
		case 32: goto st1;
	}
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto tr2;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr2;
	} else
		goto tr2;
	goto tr0;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( (*p) == 10 )
		goto tr10;
	goto st7;
tr10:
#line 9 "iostat_parser.rl"
	{ iostat->dev_tip = 0; }
#line 10 "iostat_parser.rl"
	{
		if (iostat->ready) {
			iostat->ready = false;
			mog_iostat_commit();
		}
		{goto st1;}
	}
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 228 "iostat_parser.c"
	goto st0;
	}
	_test_eof1: cs = 1; goto _test_eof; 
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 

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
#line 45 "iostat_parser.rl"
	{ p--; {goto st7;} }
	break;
#line 254 "iostat_parser.c"
	}
	}

	_out: {}
	}

#line 74 "iostat_parser.rl"

	iostat->cs = cs;

	if (cs == iostat_parser_error)
		return MOG_PARSER_ERROR;

	assert(p <= pe && "buffer overflow after iostat parse");

	if (cs == iostat_parser_first_final)
		return MOG_PARSER_DONE;

	return MOG_PARSER_CONTINUE;
}
