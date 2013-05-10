
#line 1 "mgmt_parser.rl"
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


#line 77 "mgmt_parser.rl"



#line 26 "mgmt_parser.c"
static const int mgmt_parser_start = 1;
static const int mgmt_parser_first_final = 74;
static const int mgmt_parser_error = 0;

static const int mgmt_parser_en_invalid_line = 71;
static const int mgmt_parser_en_main = 1;


#line 80 "mgmt_parser.rl"

void mog_mgmt_reset_parser(struct mog_mgmt *mgmt)
{
	int cs;
	
#line 41 "mgmt_parser.c"
	{
	cs = mgmt_parser_start;
	}

#line 85 "mgmt_parser.rl"
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

	
#line 76 "mgmt_parser.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	switch( (*p) ) {
		case 9: goto st2;
		case 10: goto tr2;
		case 13: goto st3;
		case 32: goto st2;
		case 77: goto st4;
		case 83: goto st15;
		case 115: goto st41;
		case 119: goto st65;
	}
	goto tr0;
tr0:
#line 72 "mgmt_parser.rl"
	{
		p = buf;
		p--;
		{goto st71;}
	}
	goto st0;
#line 102 "mgmt_parser.c"
st0:
cs = 0;
	goto _out;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	switch( (*p) ) {
		case 9: goto st2;
		case 10: goto tr2;
		case 13: goto st3;
		case 32: goto st2;
	}
	goto tr0;
tr2:
#line 65 "mgmt_parser.rl"
	{ mog_mgmt_fn_blank(mgmt); {p++; cs = 74; goto _out;} }
	goto st74;
tr12:
#line 47 "mgmt_parser.rl"
	{ mgmt->mark[1] = p - buf; }
#line 48 "mgmt_parser.rl"
	{ mog_mgmt_fn_digest(mgmt, buf); {p++; cs = 74; goto _out;} }
	goto st74;
tr16:
#line 48 "mgmt_parser.rl"
	{ mog_mgmt_fn_digest(mgmt, buf); {p++; cs = 74; goto _out;} }
	goto st74;
tr45:
#line 62 "mgmt_parser.rl"
	{ mgmt->mark[1] = p - buf; }
#line 63 "mgmt_parser.rl"
	{ mog_mgmt_fn_aio_threads(mgmt, buf); {p++; cs = 74; goto _out;} }
	goto st74;
tr48:
#line 63 "mgmt_parser.rl"
	{ mog_mgmt_fn_aio_threads(mgmt, buf); {p++; cs = 74; goto _out;} }
	goto st74;
tr60:
#line 66 "mgmt_parser.rl"
	{
		cmogstored_quit();
		{p++; cs = 74; goto _out;}
	}
	goto st74;
tr75:
#line 36 "mgmt_parser.rl"
	{ mgmt->mark[1] = p - buf; }
#line 37 "mgmt_parser.rl"
	{ mog_mgmt_fn_size(mgmt, buf); {p++; cs = 74; goto _out;} }
	goto st74;
tr78:
#line 37 "mgmt_parser.rl"
	{ mog_mgmt_fn_size(mgmt, buf); {p++; cs = 74; goto _out;} }
	goto st74;
tr83:
#line 50 "mgmt_parser.rl"
	{
		static int have_iostat = 1;

		if (have_iostat)
			mgmt->forward = MOG_IOSTAT;
		else
			mog_mgmt_fn_watch_err(mgmt);
		{p++; cs = 74; goto _out;}
	}
	goto st74;
st74:
	if ( ++p == pe )
		goto _test_eof74;
case 74:
#line 174 "mgmt_parser.c"
	goto tr0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	if ( (*p) == 10 )
		goto tr2;
	goto tr0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	if ( (*p) == 68 )
		goto st5;
	goto tr0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( (*p) == 53 )
		goto tr9;
	goto tr0;
tr9:
#line 41 "mgmt_parser.rl"
	{ mgmt->alg = GC_MD5; }
	goto st6;
tr51:
#line 43 "mgmt_parser.rl"
	{ mgmt->alg = GC_SHA1; }
	goto st6;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
#line 209 "mgmt_parser.c"
	if ( (*p) == 32 )
		goto st7;
	goto tr0;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( (*p) == 47 )
		goto tr11;
	goto tr0;
tr11:
#line 46 "mgmt_parser.rl"
	{ mgmt->mark[0] = p - buf; }
	goto st8;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 228 "mgmt_parser.c"
	switch( (*p) ) {
		case 10: goto tr12;
		case 13: goto tr13;
		case 32: goto tr14;
	}
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st8;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st8;
	} else
		goto st8;
	goto tr0;
tr13:
#line 47 "mgmt_parser.rl"
	{ mgmt->mark[1] = p - buf; }
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 251 "mgmt_parser.c"
	if ( (*p) == 10 )
		goto tr16;
	goto tr0;
tr14:
#line 47 "mgmt_parser.rl"
	{ mgmt->mark[1] = p - buf; }
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 263 "mgmt_parser.c"
	switch( (*p) ) {
		case 95: goto st11;
		case 102: goto st12;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st11;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st11;
	} else
		goto st11;
	goto tr0;
tr22:
#line 23 "mgmt_parser.rl"
	{ set_prio_fsck(mgmt); }
	goto st11;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
#line 285 "mgmt_parser.c"
	switch( (*p) ) {
		case 10: goto tr16;
		case 13: goto st9;
		case 95: goto st11;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st11;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st11;
	} else
		goto st11;
	goto tr0;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	switch( (*p) ) {
		case 10: goto tr16;
		case 13: goto st9;
		case 95: goto st11;
		case 115: goto st13;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st11;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st11;
	} else
		goto st11;
	goto tr0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	switch( (*p) ) {
		case 10: goto tr16;
		case 13: goto st9;
		case 95: goto st11;
		case 99: goto st14;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st11;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st11;
	} else
		goto st11;
	goto tr0;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	switch( (*p) ) {
		case 10: goto tr16;
		case 13: goto st9;
		case 95: goto st11;
		case 107: goto tr22;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st11;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st11;
	} else
		goto st11;
	goto tr0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	switch( (*p) ) {
		case 69: goto st16;
		case 72: goto st38;
		case 101: goto st16;
	}
	goto tr0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	switch( (*p) ) {
		case 82: goto st17;
		case 114: goto st17;
	}
	goto tr0;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	switch( (*p) ) {
		case 86: goto st18;
		case 118: goto st18;
	}
	goto tr0;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	switch( (*p) ) {
		case 69: goto st19;
		case 101: goto st19;
	}
	goto tr0;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
	switch( (*p) ) {
		case 82: goto st20;
		case 114: goto st20;
	}
	goto tr0;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
	if ( (*p) == 32 )
		goto st21;
	goto tr0;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
	switch( (*p) ) {
		case 65: goto st22;
		case 97: goto st22;
	}
	goto tr0;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
	switch( (*p) ) {
		case 73: goto st23;
		case 105: goto st23;
	}
	goto tr0;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
	switch( (*p) ) {
		case 79: goto st24;
		case 111: goto st24;
	}
	goto tr0;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
	if ( (*p) == 95 )
		goto st25;
	goto tr0;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
	switch( (*p) ) {
		case 84: goto st26;
		case 116: goto st26;
	}
	goto tr0;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
	switch( (*p) ) {
		case 72: goto st27;
		case 104: goto st27;
	}
	goto tr0;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
	switch( (*p) ) {
		case 82: goto st28;
		case 114: goto st28;
	}
	goto tr0;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
	switch( (*p) ) {
		case 69: goto st29;
		case 101: goto st29;
	}
	goto tr0;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
	switch( (*p) ) {
		case 65: goto st30;
		case 97: goto st30;
	}
	goto tr0;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
	switch( (*p) ) {
		case 68: goto st31;
		case 100: goto st31;
	}
	goto tr0;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
	switch( (*p) ) {
		case 83: goto st32;
		case 115: goto st32;
	}
	goto tr0;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
	switch( (*p) ) {
		case 32: goto st33;
		case 61: goto st34;
	}
	goto tr0;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
	if ( (*p) == 61 )
		goto st34;
	goto tr0;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
	if ( (*p) == 32 )
		goto st35;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr44;
	goto tr0;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr44;
	goto tr0;
tr44:
#line 61 "mgmt_parser.rl"
	{ mgmt->mark[0] = p - buf; }
	goto st36;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
#line 547 "mgmt_parser.c"
	switch( (*p) ) {
		case 10: goto tr45;
		case 13: goto tr46;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto st36;
	goto tr0;
tr46:
#line 62 "mgmt_parser.rl"
	{ mgmt->mark[1] = p - buf; }
	goto st37;
st37:
	if ( ++p == pe )
		goto _test_eof37;
case 37:
#line 563 "mgmt_parser.c"
	if ( (*p) == 10 )
		goto tr48;
	goto tr0;
st38:
	if ( ++p == pe )
		goto _test_eof38;
case 38:
	if ( (*p) == 65 )
		goto st39;
	goto tr0;
st39:
	if ( ++p == pe )
		goto _test_eof39;
case 39:
	if ( (*p) == 45 )
		goto st40;
	goto tr0;
st40:
	if ( ++p == pe )
		goto _test_eof40;
case 40:
	if ( (*p) == 49 )
		goto tr51;
	goto tr0;
st41:
	if ( ++p == pe )
		goto _test_eof41;
case 41:
	switch( (*p) ) {
		case 69: goto st16;
		case 101: goto st16;
		case 104: goto st42;
		case 105: goto st59;
	}
	goto tr0;
st42:
	if ( ++p == pe )
		goto _test_eof42;
case 42:
	if ( (*p) == 117 )
		goto st43;
	goto tr0;
st43:
	if ( ++p == pe )
		goto _test_eof43;
case 43:
	if ( (*p) == 116 )
		goto st44;
	goto tr0;
st44:
	if ( ++p == pe )
		goto _test_eof44;
case 44:
	if ( (*p) == 100 )
		goto st45;
	goto tr0;
st45:
	if ( ++p == pe )
		goto _test_eof45;
case 45:
	if ( (*p) == 111 )
		goto st46;
	goto tr0;
st46:
	if ( ++p == pe )
		goto _test_eof46;
case 46:
	if ( (*p) == 119 )
		goto st47;
	goto tr0;
st47:
	if ( ++p == pe )
		goto _test_eof47;
case 47:
	if ( (*p) == 110 )
		goto st48;
	goto tr0;
st48:
	if ( ++p == pe )
		goto _test_eof48;
case 48:
	switch( (*p) ) {
		case 10: goto tr60;
		case 13: goto st49;
		case 32: goto st50;
	}
	goto tr0;
st49:
	if ( ++p == pe )
		goto _test_eof49;
case 49:
	if ( (*p) == 10 )
		goto tr60;
	goto tr0;
st50:
	if ( ++p == pe )
		goto _test_eof50;
case 50:
	switch( (*p) ) {
		case 32: goto st50;
		case 103: goto st51;
	}
	goto tr0;
st51:
	if ( ++p == pe )
		goto _test_eof51;
case 51:
	if ( (*p) == 114 )
		goto st52;
	goto tr0;
st52:
	if ( ++p == pe )
		goto _test_eof52;
case 52:
	if ( (*p) == 97 )
		goto st53;
	goto tr0;
st53:
	if ( ++p == pe )
		goto _test_eof53;
case 53:
	if ( (*p) == 99 )
		goto st54;
	goto tr0;
st54:
	if ( ++p == pe )
		goto _test_eof54;
case 54:
	if ( (*p) == 101 )
		goto st55;
	goto tr0;
st55:
	if ( ++p == pe )
		goto _test_eof55;
case 55:
	if ( (*p) == 102 )
		goto st56;
	goto tr0;
st56:
	if ( ++p == pe )
		goto _test_eof56;
case 56:
	if ( (*p) == 117 )
		goto st57;
	goto tr0;
st57:
	if ( ++p == pe )
		goto _test_eof57;
case 57:
	if ( (*p) == 108 )
		goto st58;
	goto tr0;
st58:
	if ( ++p == pe )
		goto _test_eof58;
case 58:
	switch( (*p) ) {
		case 10: goto tr60;
		case 13: goto st49;
	}
	goto tr0;
st59:
	if ( ++p == pe )
		goto _test_eof59;
case 59:
	if ( (*p) == 122 )
		goto st60;
	goto tr0;
st60:
	if ( ++p == pe )
		goto _test_eof60;
case 60:
	if ( (*p) == 101 )
		goto st61;
	goto tr0;
st61:
	if ( ++p == pe )
		goto _test_eof61;
case 61:
	if ( (*p) == 32 )
		goto st62;
	goto tr0;
st62:
	if ( ++p == pe )
		goto _test_eof62;
case 62:
	if ( (*p) == 47 )
		goto tr74;
	goto tr0;
tr74:
#line 35 "mgmt_parser.rl"
	{ mgmt->mark[0] = p - buf; }
	goto st63;
st63:
	if ( ++p == pe )
		goto _test_eof63;
case 63:
#line 761 "mgmt_parser.c"
	switch( (*p) ) {
		case 10: goto tr75;
		case 13: goto tr76;
	}
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st63;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st63;
	} else
		goto st63;
	goto tr0;
tr76:
#line 36 "mgmt_parser.rl"
	{ mgmt->mark[1] = p - buf; }
	goto st64;
st64:
	if ( ++p == pe )
		goto _test_eof64;
case 64:
#line 783 "mgmt_parser.c"
	if ( (*p) == 10 )
		goto tr78;
	goto tr0;
st65:
	if ( ++p == pe )
		goto _test_eof65;
case 65:
	if ( (*p) == 97 )
		goto st66;
	goto tr0;
st66:
	if ( ++p == pe )
		goto _test_eof66;
case 66:
	if ( (*p) == 116 )
		goto st67;
	goto tr0;
st67:
	if ( ++p == pe )
		goto _test_eof67;
case 67:
	if ( (*p) == 99 )
		goto st68;
	goto tr0;
st68:
	if ( ++p == pe )
		goto _test_eof68;
case 68:
	if ( (*p) == 104 )
		goto st69;
	goto tr0;
st69:
	if ( ++p == pe )
		goto _test_eof69;
case 69:
	switch( (*p) ) {
		case 10: goto tr83;
		case 13: goto st70;
	}
	goto tr0;
st70:
	if ( ++p == pe )
		goto _test_eof70;
case 70:
	if ( (*p) == 10 )
		goto tr83;
	goto tr0;
st71:
	if ( ++p == pe )
		goto _test_eof71;
case 71:
	switch( (*p) ) {
		case 9: goto st71;
		case 13: goto st0;
		case 32: goto st71;
	}
	goto tr85;
tr85:
#line 26 "mgmt_parser.rl"
	{ mgmt->mark[0] = p - buf; }
	goto st72;
tr88:
#line 27 "mgmt_parser.rl"
	{ mgmt->mark[1] = p - buf; }
	goto st72;
st72:
	if ( ++p == pe )
		goto _test_eof72;
case 72:
#line 853 "mgmt_parser.c"
	switch( (*p) ) {
		case 9: goto tr89;
		case 10: goto tr90;
		case 13: goto tr89;
		case 32: goto tr89;
	}
	goto tr88;
tr89:
#line 27 "mgmt_parser.rl"
	{ mgmt->mark[1] = p - buf; }
	goto st73;
st73:
	if ( ++p == pe )
		goto _test_eof73;
case 73:
#line 869 "mgmt_parser.c"
	if ( (*p) == 10 )
		goto tr92;
	goto st73;
tr92:
#line 29 "mgmt_parser.rl"
	{
		mog_mgmt_fn_unknown(mgmt, buf);
		really_done = 1;
		{p++; cs = 75; goto _out;}
	}
	goto st75;
st75:
	if ( ++p == pe )
		goto _test_eof75;
case 75:
#line 885 "mgmt_parser.c"
	goto st0;
tr90:
#line 27 "mgmt_parser.rl"
	{ mgmt->mark[1] = p - buf; }
#line 29 "mgmt_parser.rl"
	{
		mog_mgmt_fn_unknown(mgmt, buf);
		really_done = 1;
		{p++; cs = 76; goto _out;}
	}
	goto st76;
st76:
	if ( ++p == pe )
		goto _test_eof76;
case 76:
#line 901 "mgmt_parser.c"
	switch( (*p) ) {
		case 9: goto tr89;
		case 10: goto tr90;
		case 13: goto tr89;
		case 32: goto tr89;
	}
	goto tr88;
	}
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof74: cs = 74; goto _test_eof; 
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
	_test_eof75: cs = 75; goto _test_eof; 
	_test_eof76: cs = 76; goto _test_eof; 

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
	case 26: 
	case 27: 
	case 28: 
	case 29: 
	case 30: 
	case 31: 
	case 32: 
	case 33: 
	case 34: 
	case 35: 
	case 36: 
	case 37: 
	case 38: 
	case 39: 
	case 40: 
	case 41: 
	case 42: 
	case 43: 
	case 44: 
	case 45: 
	case 46: 
	case 47: 
	case 48: 
	case 49: 
	case 50: 
	case 51: 
	case 52: 
	case 53: 
	case 54: 
	case 55: 
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
#line 72 "mgmt_parser.rl"
	{
		p = buf;
		p--;
		{goto st71;}
	}
	break;
#line 1067 "mgmt_parser.c"
	}
	}

	_out: {}
	}

#line 114 "mgmt_parser.rl"

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
