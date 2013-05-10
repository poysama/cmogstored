
#line 1 "valid_put_path.rl"
/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"


#line 10 "valid_put_path.rl"



#line 15 "valid_put_path.c"
static const int valid_put_path_start = 1;
static const int valid_put_path_first_final = 9;
static const int valid_put_path_error = 0;

static const int valid_put_path_en_main = 1;


#line 13 "valid_put_path.rl"

bool mog_valid_put_path(const char *buf, size_t len)
{
	const char *p, *pe;
	int cs;

	if (len <= 0)
		return false;
	if (buf[len - 1] == '/')
		return false;

	
#line 36 "valid_put_path.c"
	{
	cs = valid_put_path_start;
	}

#line 25 "valid_put_path.rl"

	p = buf;
	pe = buf + len;
	
#line 46 "valid_put_path.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	if ( (*p) == 47 )
		goto st2;
	goto st0;
st0:
cs = 0;
	goto _out;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	if ( (*p) == 100 )
		goto st3;
	goto st0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	if ( (*p) == 101 )
		goto st4;
	goto st0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	if ( (*p) == 118 )
		goto st5;
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st6;
	goto st0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	if ( (*p) == 47 )
		goto st7;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st6;
	goto st0;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( (*p) == 47 )
		goto st7;
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	goto st9;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 29 "valid_put_path.rl"

	return cs != valid_put_path_error;
}
