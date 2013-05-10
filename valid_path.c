
#line 1 "valid_path.rl"
/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

/*
 * we could just use strstr(), but it's buggy on some glibc and
 * we can expand this later (to tighten down to non-FIDs, for example)
 */

#line 14 "valid_path.rl"



#line 19 "valid_path.c"
static const int path_traversal_start = 0;
static const int path_traversal_first_final = 2;
static const int path_traversal_error = -1;

static const int path_traversal_en_main = 0;


#line 17 "valid_path.rl"

static bool path_traversal_found(const char *buf, size_t len)
{
	const char *p, *pe;
	bool found = false;
	int cs;
	
#line 35 "valid_path.c"
	{
	cs = path_traversal_start;
	}

#line 24 "valid_path.rl"

	p = buf;
	pe = buf + len;
	
#line 45 "valid_path.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
st0:
	if ( ++p == pe )
		goto _test_eof0;
case 0:
	if ( (*p) == 46 )
		goto st1;
	goto st0;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
	if ( (*p) == 46 )
		goto tr2;
	goto st0;
tr2:
#line 13 "valid_path.rl"
	{ found = true; {p++; cs = 2; goto _out;} }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 73 "valid_path.c"
	if ( (*p) == 46 )
		goto tr2;
	goto st3;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	if ( (*p) == 46 )
		goto st2;
	goto st3;
	}
	_test_eof0: cs = 0; goto _test_eof; 
	_test_eof1: cs = 1; goto _test_eof; 
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 

	_test_eof: {}
	_out: {}
	}

#line 28 "valid_path.rl"

	return found;
}

int mog_valid_path(const char *buf, size_t len)
{
	/* TODO: update if MogileFS supports FIDs >= 10,000,000,000 */
	if (len >= (sizeof("/dev16777215/0/000/000/0123456789.fid")))
		return 0;

	return ! path_traversal_found(buf, len);
}
