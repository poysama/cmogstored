
#line 1 "http_parser.rl"
/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "http_util.h"

static bool length_incr(off_t *len, unsigned c)
{
	off_t prev = *len;

	*len *= 10;
	*len += c - '0';

	if (*len >= prev)
		return true;

	errno = ERANGE;
	*len = -1;

	return false;
}


#line 116 "http_parser.rl"



#line 32 "http_parser.c"
static const int http_parser_start = 1;
static const int http_parser_first_final = 468;
static const int http_parser_error = 0;

static const int http_parser_en_ignored_trailer = 240;
static const int http_parser_en_more_trailers = 249;
static const int http_parser_en_ignored_header = 293;
static const int http_parser_en_more_headers = 302;
static const int http_parser_en_main = 1;


#line 119 "http_parser.rl"

void mog_http_reset_parser(struct mog_http *http)
{
	int cs;
	struct mog_rbuf *rbuf = http->rbuf;
	struct mog_svc *svc = http->svc;

	
#line 53 "http_parser.c"
	{
	cs = http_parser_start;
	}

#line 127 "http_parser.rl"
	memset(http, 0, sizeof(struct mog_http));
	http->cs = cs;
	http->rbuf = rbuf;
	http->svc = svc;
}

void mog_http_init(struct mog_http *http, struct mog_svc *svc)
{
	http->svc = svc;
	http->rbuf = NULL;
	mog_http_reset_parser(http);
}

enum mog_parser_state
mog_http_parse(struct mog_http *http, char *buf, size_t len)
{
	char *p, *pe, *eof = NULL;
	int cs = http->cs;
	int really_done = 0;
	size_t off = http->offset;

	assert(http->wbuf == NULL && "unwritten data in buffer");
	assert(off <= len && "http offset past end of buffer");

	p = buf + off;
	pe = buf + len;

	assert((void *)(pe - p) == (void *)(len - off) &&
	       "pointers aren't same distance");

	errno = 0;
	
#line 91 "http_parser.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
case 1:
	switch( (*p) ) {
		case 68: goto tr0;
		case 71: goto tr2;
		case 72: goto tr3;
		case 77: goto tr4;
		case 80: goto tr5;
	}
	goto st0;
tr27:
#line 104 "http_parser.rl"
	{
			assert(http->line_end > 0 &&
			       "no previous request/header line");
			assert(buf[http->line_end] == '\n' &&
			       "bad http->line_end");
			p = buf + http->line_end + 1;
			assert(p <= pe && "overflow");
			{goto st293;}
		}
	goto st0;
tr77:
#line 52 "http_parser.rl"
	{ errno = EINVAL; {p++; cs = 0; goto _out;} }
#line 104 "http_parser.rl"
	{
			assert(http->line_end > 0 &&
			       "no previous request/header line");
			assert(buf[http->line_end] == '\n' &&
			       "bad http->line_end");
			p = buf + http->line_end + 1;
			assert(p <= pe && "overflow");
			{goto st293;}
		}
	goto st0;
tr85:
#line 30 "http_common.rl"
	{
				if (!http->has_expect_md5) {
					errno = EINVAL;
					{p++; cs = 0; goto _out;}
				}
			}
#line 104 "http_parser.rl"
	{
			assert(http->line_end > 0 &&
			       "no previous request/header line");
			assert(buf[http->line_end] == '\n' &&
			       "bad http->line_end");
			p = buf + http->line_end + 1;
			assert(p <= pe && "overflow");
			{goto st293;}
		}
	goto st0;
tr123:
#line 85 "http_parser.rl"
	{ errno = EINVAL; {p++; cs = 0; goto _out;} }
#line 104 "http_parser.rl"
	{
			assert(http->line_end > 0 &&
			       "no previous request/header line");
			assert(buf[http->line_end] == '\n' &&
			       "bad http->line_end");
			p = buf + http->line_end + 1;
			assert(p <= pe && "overflow");
			{goto st293;}
		}
	goto st0;
tr202:
#line 59 "http_parser.rl"
	{ errno = EINVAL; {p++; cs = 0; goto _out;} }
#line 104 "http_parser.rl"
	{
			assert(http->line_end > 0 &&
			       "no previous request/header line");
			assert(buf[http->line_end] == '\n' &&
			       "bad http->line_end");
			p = buf + http->line_end + 1;
			assert(p <= pe && "overflow");
			{goto st293;}
		}
	goto st0;
tr205:
#line 65 "http_parser.rl"
	{ errno = EINVAL; {p++; cs = 0; goto _out;} }
#line 104 "http_parser.rl"
	{
			assert(http->line_end > 0 &&
			       "no previous request/header line");
			assert(buf[http->line_end] == '\n' &&
			       "bad http->line_end");
			p = buf + http->line_end + 1;
			assert(p <= pe && "overflow");
			{goto st293;}
		}
	goto st0;
tr268:
#line 40 "http_common.rl"
	{
			if (http->line_end > 0) {
				assert(buf[http->line_end] == '\n'
				       && "bad http->line_end");
				p = buf + http->line_end + 1;
			} else {
				p = buf;
			}
			assert(p <= pe && "overflow");
			{goto st240;}
		}
	goto st0;
tr283:
#line 30 "http_common.rl"
	{
				if (!http->has_expect_md5) {
					errno = EINVAL;
					{p++; cs = 0; goto _out;}
				}
			}
#line 40 "http_common.rl"
	{
			if (http->line_end > 0) {
				assert(buf[http->line_end] == '\n'
				       && "bad http->line_end");
				p = buf + http->line_end + 1;
			} else {
				p = buf;
			}
			assert(p <= pe && "overflow");
			{goto st240;}
		}
	goto st0;
#line 228 "http_parser.c"
st0:
cs = 0;
	goto _out;
tr0:
#line 36 "http_parser.rl"
	{ http->http_method = MOG_HTTP_METHOD_DELETE; }
	goto st2;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
#line 240 "http_parser.c"
	if ( (*p) == 69 )
		goto st3;
	goto st0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	if ( (*p) == 76 )
		goto st4;
	goto st0;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	if ( (*p) == 69 )
		goto st5;
	goto st0;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	if ( (*p) == 84 )
		goto st6;
	goto st0;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	if ( (*p) == 69 )
		goto st7;
	goto st0;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( (*p) == 32 )
		goto st8;
	goto st0;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	switch( (*p) ) {
		case 47: goto tr12;
		case 104: goto st222;
	}
	goto st0;
tr12:
#line 42 "http_parser.rl"
	{ http->path_tip = to_u8(p - buf); }
	goto st9;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
#line 296 "http_parser.c"
	switch( (*p) ) {
		case 32: goto tr14;
		case 47: goto tr12;
	}
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st186;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st186;
	} else
		goto st186;
	goto st0;
tr14:
#line 44 "http_parser.rl"
	{ http->path_end = to_u8(p - buf); }
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 318 "http_parser.c"
	if ( (*p) == 72 )
		goto st11;
	goto st0;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	if ( (*p) == 84 )
		goto st12;
	goto st0;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	if ( (*p) == 84 )
		goto st13;
	goto st0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	if ( (*p) == 80 )
		goto st14;
	goto st0;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
	if ( (*p) == 47 )
		goto st15;
	goto st0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	if ( (*p) == 49 )
		goto st16;
	goto st0;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	if ( (*p) == 46 )
		goto st17;
	goto st0;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	switch( (*p) ) {
		case 48: goto st18;
		case 49: goto tr24;
	}
	goto st0;
tr24:
#line 45 "http_parser.rl"
	{ http->persistent = 1; }
	goto st18;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
#line 381 "http_parser.c"
	if ( (*p) == 13 )
		goto st19;
	goto st0;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
	if ( (*p) == 10 )
		goto tr26;
	goto st0;
tr26:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st20;
tr138:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
#line 86 "http_parser.rl"
	{ http->has_range = 1; }
	goto st20;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
#line 406 "http_parser.c"
	switch( (*p) ) {
		case 13: goto st21;
		case 67: goto st22;
		case 82: goto st101;
		case 84: goto st119;
		case 99: goto st22;
		case 114: goto st101;
		case 116: goto st119;
	}
	goto tr27;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
	if ( (*p) == 10 )
		goto tr32;
	goto st0;
tr32:
#line 113 "http_parser.rl"
	{ really_done = 1; {p++; cs = 468; goto _out;} }
	goto st468;
st468:
	if ( ++p == pe )
		goto _test_eof468;
case 468:
#line 432 "http_parser.c"
	goto st0;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
	switch( (*p) ) {
		case 79: goto st23;
		case 111: goto st23;
	}
	goto tr27;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
	switch( (*p) ) {
		case 78: goto st24;
		case 110: goto st24;
	}
	goto tr27;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
	switch( (*p) ) {
		case 78: goto st25;
		case 84: goto st51;
		case 110: goto st25;
		case 116: goto st51;
	}
	goto tr27;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
	switch( (*p) ) {
		case 69: goto st26;
		case 101: goto st26;
	}
	goto tr27;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
	switch( (*p) ) {
		case 67: goto st27;
		case 99: goto st27;
	}
	goto tr27;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
	switch( (*p) ) {
		case 84: goto st28;
		case 116: goto st28;
	}
	goto tr27;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
	switch( (*p) ) {
		case 73: goto st29;
		case 105: goto st29;
	}
	goto tr27;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
	switch( (*p) ) {
		case 79: goto st30;
		case 111: goto st30;
	}
	goto tr27;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
	switch( (*p) ) {
		case 78: goto st31;
		case 110: goto st31;
	}
	goto tr27;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
	if ( (*p) == 58 )
		goto st32;
	goto tr27;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
	switch( (*p) ) {
		case 9: goto st32;
		case 13: goto st33;
		case 32: goto st32;
		case 67: goto st36;
		case 75: goto st42;
		case 99: goto st36;
		case 107: goto st42;
	}
	goto tr27;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
	if ( (*p) == 10 )
		goto tr47;
	goto tr27;
tr47:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st34;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
#line 553 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st35;
		case 32: goto st35;
	}
	goto tr27;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
	switch( (*p) ) {
		case 9: goto st35;
		case 32: goto st35;
		case 67: goto st36;
		case 75: goto st42;
		case 99: goto st36;
		case 107: goto st42;
	}
	goto tr27;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
	switch( (*p) ) {
		case 76: goto st37;
		case 108: goto st37;
	}
	goto tr27;
st37:
	if ( ++p == pe )
		goto _test_eof37;
case 37:
	switch( (*p) ) {
		case 79: goto st38;
		case 111: goto st38;
	}
	goto tr27;
st38:
	if ( ++p == pe )
		goto _test_eof38;
case 38:
	switch( (*p) ) {
		case 83: goto st39;
		case 115: goto st39;
	}
	goto tr27;
st39:
	if ( ++p == pe )
		goto _test_eof39;
case 39:
	switch( (*p) ) {
		case 69: goto tr52;
		case 101: goto tr52;
	}
	goto tr27;
tr52:
#line 94 "http_parser.rl"
	{ http->persistent = 0; }
	goto st40;
tr63:
#line 95 "http_parser.rl"
	{ http->persistent = 1; }
	goto st40;
tr186:
#line 88 "http_parser.rl"
	{ http->chunked = 1; }
	goto st40;
tr209:
#line 67 "http_parser.rl"
	{ http->has_content_range = 1; }
	goto st40;
st40:
	if ( ++p == pe )
		goto _test_eof40;
case 40:
#line 628 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st40;
		case 13: goto st41;
		case 32: goto st40;
	}
	goto tr27;
tr187:
#line 88 "http_parser.rl"
	{ http->chunked = 1; }
	goto st41;
tr210:
#line 67 "http_parser.rl"
	{ http->has_content_range = 1; }
	goto st41;
st41:
	if ( ++p == pe )
		goto _test_eof41;
case 41:
#line 647 "http_parser.c"
	if ( (*p) == 10 )
		goto tr26;
	goto tr27;
st42:
	if ( ++p == pe )
		goto _test_eof42;
case 42:
	switch( (*p) ) {
		case 69: goto st43;
		case 101: goto st43;
	}
	goto tr27;
st43:
	if ( ++p == pe )
		goto _test_eof43;
case 43:
	switch( (*p) ) {
		case 69: goto st44;
		case 101: goto st44;
	}
	goto tr27;
st44:
	if ( ++p == pe )
		goto _test_eof44;
case 44:
	switch( (*p) ) {
		case 80: goto st45;
		case 112: goto st45;
	}
	goto tr27;
st45:
	if ( ++p == pe )
		goto _test_eof45;
case 45:
	if ( (*p) == 45 )
		goto st46;
	goto tr27;
st46:
	if ( ++p == pe )
		goto _test_eof46;
case 46:
	switch( (*p) ) {
		case 65: goto st47;
		case 97: goto st47;
	}
	goto tr27;
st47:
	if ( ++p == pe )
		goto _test_eof47;
case 47:
	switch( (*p) ) {
		case 76: goto st48;
		case 108: goto st48;
	}
	goto tr27;
st48:
	if ( ++p == pe )
		goto _test_eof48;
case 48:
	switch( (*p) ) {
		case 73: goto st49;
		case 105: goto st49;
	}
	goto tr27;
st49:
	if ( ++p == pe )
		goto _test_eof49;
case 49:
	switch( (*p) ) {
		case 86: goto st50;
		case 118: goto st50;
	}
	goto tr27;
st50:
	if ( ++p == pe )
		goto _test_eof50;
case 50:
	switch( (*p) ) {
		case 69: goto tr63;
		case 101: goto tr63;
	}
	goto tr27;
st51:
	if ( ++p == pe )
		goto _test_eof51;
case 51:
	switch( (*p) ) {
		case 69: goto st52;
		case 101: goto st52;
	}
	goto tr27;
st52:
	if ( ++p == pe )
		goto _test_eof52;
case 52:
	switch( (*p) ) {
		case 78: goto st53;
		case 110: goto st53;
	}
	goto tr27;
st53:
	if ( ++p == pe )
		goto _test_eof53;
case 53:
	switch( (*p) ) {
		case 84: goto st54;
		case 116: goto st54;
	}
	goto tr27;
st54:
	if ( ++p == pe )
		goto _test_eof54;
case 54:
	if ( (*p) == 45 )
		goto st55;
	goto tr27;
st55:
	if ( ++p == pe )
		goto _test_eof55;
case 55:
	switch( (*p) ) {
		case 76: goto st56;
		case 77: goto st67;
		case 82: goto st166;
		case 108: goto st56;
		case 109: goto st67;
		case 114: goto st166;
	}
	goto tr27;
st56:
	if ( ++p == pe )
		goto _test_eof56;
case 56:
	switch( (*p) ) {
		case 69: goto st57;
		case 101: goto st57;
	}
	goto tr27;
st57:
	if ( ++p == pe )
		goto _test_eof57;
case 57:
	switch( (*p) ) {
		case 78: goto st58;
		case 110: goto st58;
	}
	goto tr27;
st58:
	if ( ++p == pe )
		goto _test_eof58;
case 58:
	switch( (*p) ) {
		case 71: goto st59;
		case 103: goto st59;
	}
	goto tr27;
st59:
	if ( ++p == pe )
		goto _test_eof59;
case 59:
	switch( (*p) ) {
		case 84: goto st60;
		case 116: goto st60;
	}
	goto tr27;
st60:
	if ( ++p == pe )
		goto _test_eof60;
case 60:
	switch( (*p) ) {
		case 72: goto st61;
		case 104: goto st61;
	}
	goto tr27;
st61:
	if ( ++p == pe )
		goto _test_eof61;
case 61:
	if ( (*p) == 58 )
		goto st62;
	goto tr27;
st62:
	if ( ++p == pe )
		goto _test_eof62;
case 62:
	switch( (*p) ) {
		case 9: goto st62;
		case 13: goto st63;
		case 32: goto st62;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr79;
	goto tr77;
st63:
	if ( ++p == pe )
		goto _test_eof63;
case 63:
	if ( (*p) == 10 )
		goto tr80;
	goto tr27;
tr80:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st64;
st64:
	if ( ++p == pe )
		goto _test_eof64;
case 64:
#line 856 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st65;
		case 32: goto st65;
	}
	goto tr27;
st65:
	if ( ++p == pe )
		goto _test_eof65;
case 65:
	switch( (*p) ) {
		case 9: goto st65;
		case 32: goto st65;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr79;
	goto tr77;
tr79:
#line 48 "http_parser.rl"
	{
			if (!length_incr(&http->content_len, (*p)))
				{p++; cs = 66; goto _out;}
		}
	goto st66;
st66:
	if ( ++p == pe )
		goto _test_eof66;
case 66:
#line 884 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st40;
		case 13: goto st41;
		case 32: goto st40;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr79;
	goto tr77;
st67:
	if ( ++p == pe )
		goto _test_eof67;
case 67:
	switch( (*p) ) {
		case 68: goto st68;
		case 100: goto st68;
	}
	goto tr27;
st68:
	if ( ++p == pe )
		goto _test_eof68;
case 68:
	if ( (*p) == 53 )
		goto st69;
	goto tr27;
st69:
	if ( ++p == pe )
		goto _test_eof69;
case 69:
	if ( (*p) == 58 )
		goto st70;
	goto tr27;
st70:
	if ( ++p == pe )
		goto _test_eof70;
case 70:
	switch( (*p) ) {
		case 9: goto st70;
		case 13: goto st71;
		case 32: goto st70;
		case 43: goto tr87;
	}
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto tr87;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr87;
	} else
		goto tr87;
	goto tr85;
st71:
	if ( ++p == pe )
		goto _test_eof71;
case 71:
	if ( (*p) == 10 )
		goto tr88;
	goto tr27;
tr88:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st72;
st72:
	if ( ++p == pe )
		goto _test_eof72;
case 72:
#line 950 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st73;
		case 32: goto st73;
	}
	goto tr27;
st73:
	if ( ++p == pe )
		goto _test_eof73;
case 73:
	switch( (*p) ) {
		case 9: goto st73;
		case 32: goto st73;
		case 43: goto tr87;
	}
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto tr87;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr87;
	} else
		goto tr87;
	goto tr85;
tr87:
#line 15 "http_common.rl"
	{ http->tmp_tip = to_u16(p - buf); }
	goto st74;
st74:
	if ( ++p == pe )
		goto _test_eof74;
case 74:
#line 982 "http_parser.c"
	if ( (*p) == 43 )
		goto st75;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st75;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st75;
	} else
		goto st75;
	goto tr85;
st75:
	if ( ++p == pe )
		goto _test_eof75;
case 75:
	if ( (*p) == 43 )
		goto st76;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st76;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st76;
	} else
		goto st76;
	goto tr85;
st76:
	if ( ++p == pe )
		goto _test_eof76;
case 76:
	if ( (*p) == 43 )
		goto st77;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st77;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st77;
	} else
		goto st77;
	goto tr85;
st77:
	if ( ++p == pe )
		goto _test_eof77;
case 77:
	if ( (*p) == 43 )
		goto st78;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st78;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st78;
	} else
		goto st78;
	goto tr85;
st78:
	if ( ++p == pe )
		goto _test_eof78;
case 78:
	if ( (*p) == 43 )
		goto st79;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st79;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st79;
	} else
		goto st79;
	goto tr85;
st79:
	if ( ++p == pe )
		goto _test_eof79;
case 79:
	if ( (*p) == 43 )
		goto st80;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st80;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st80;
	} else
		goto st80;
	goto tr85;
st80:
	if ( ++p == pe )
		goto _test_eof80;
case 80:
	if ( (*p) == 43 )
		goto st81;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st81;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st81;
	} else
		goto st81;
	goto tr85;
st81:
	if ( ++p == pe )
		goto _test_eof81;
case 81:
	if ( (*p) == 43 )
		goto st82;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st82;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st82;
	} else
		goto st82;
	goto tr85;
st82:
	if ( ++p == pe )
		goto _test_eof82;
case 82:
	if ( (*p) == 43 )
		goto st83;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st83;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st83;
	} else
		goto st83;
	goto tr85;
st83:
	if ( ++p == pe )
		goto _test_eof83;
case 83:
	if ( (*p) == 43 )
		goto st84;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st84;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st84;
	} else
		goto st84;
	goto tr85;
st84:
	if ( ++p == pe )
		goto _test_eof84;
case 84:
	if ( (*p) == 43 )
		goto st85;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st85;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st85;
	} else
		goto st85;
	goto tr85;
st85:
	if ( ++p == pe )
		goto _test_eof85;
case 85:
	if ( (*p) == 43 )
		goto st86;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st86;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st86;
	} else
		goto st86;
	goto tr85;
st86:
	if ( ++p == pe )
		goto _test_eof86;
case 86:
	if ( (*p) == 43 )
		goto st87;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st87;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st87;
	} else
		goto st87;
	goto tr85;
st87:
	if ( ++p == pe )
		goto _test_eof87;
case 87:
	if ( (*p) == 43 )
		goto st88;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st88;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st88;
	} else
		goto st88;
	goto tr85;
st88:
	if ( ++p == pe )
		goto _test_eof88;
case 88:
	if ( (*p) == 43 )
		goto st89;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st89;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st89;
	} else
		goto st89;
	goto tr85;
st89:
	if ( ++p == pe )
		goto _test_eof89;
case 89:
	if ( (*p) == 43 )
		goto st90;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st90;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st90;
	} else
		goto st90;
	goto tr85;
st90:
	if ( ++p == pe )
		goto _test_eof90;
case 90:
	if ( (*p) == 43 )
		goto st91;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st91;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st91;
	} else
		goto st91;
	goto tr85;
st91:
	if ( ++p == pe )
		goto _test_eof91;
case 91:
	if ( (*p) == 43 )
		goto st92;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st92;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st92;
	} else
		goto st92;
	goto tr85;
st92:
	if ( ++p == pe )
		goto _test_eof92;
case 92:
	if ( (*p) == 43 )
		goto st93;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st93;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st93;
	} else
		goto st93;
	goto tr85;
st93:
	if ( ++p == pe )
		goto _test_eof93;
case 93:
	if ( (*p) == 43 )
		goto st94;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st94;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st94;
	} else
		goto st94;
	goto tr85;
st94:
	if ( ++p == pe )
		goto _test_eof94;
case 94:
	if ( (*p) == 43 )
		goto st95;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st95;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st95;
	} else
		goto st95;
	goto tr85;
st95:
	if ( ++p == pe )
		goto _test_eof95;
case 95:
	if ( (*p) == 61 )
		goto st96;
	goto tr85;
st96:
	if ( ++p == pe )
		goto _test_eof96;
case 96:
	if ( (*p) == 61 )
		goto st97;
	goto tr85;
st97:
	if ( ++p == pe )
		goto _test_eof97;
case 97:
	switch( (*p) ) {
		case 9: goto tr113;
		case 13: goto tr114;
		case 32: goto tr113;
	}
	goto tr85;
tr113:
#line 17 "http_common.rl"
	{
			uint16_t tmp_end = to_u16(p - buf);
			char *in = buf + http->tmp_tip;
			size_t inlen = tmp_end - http->tmp_tip;
			char *out = (char *)http->expect_md5;
			size_t outlen = sizeof(http->expect_md5);
			bool rc;

			rc = base64_decode_ctx(NULL, in, inlen, out, &outlen);
			assert(rc == true && outlen == 16
			       && "base64_decoder broke for HTTP");
			http->has_expect_md5 = 1;
		  }
	goto st98;
st98:
	if ( ++p == pe )
		goto _test_eof98;
case 98:
#line 1338 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st98;
		case 13: goto st99;
		case 32: goto st98;
	}
	goto tr85;
tr114:
#line 17 "http_common.rl"
	{
			uint16_t tmp_end = to_u16(p - buf);
			char *in = buf + http->tmp_tip;
			size_t inlen = tmp_end - http->tmp_tip;
			char *out = (char *)http->expect_md5;
			size_t outlen = sizeof(http->expect_md5);
			bool rc;

			rc = base64_decode_ctx(NULL, in, inlen, out, &outlen);
			assert(rc == true && outlen == 16
			       && "base64_decoder broke for HTTP");
			http->has_expect_md5 = 1;
		  }
	goto st99;
st99:
	if ( ++p == pe )
		goto _test_eof99;
case 99:
#line 1365 "http_parser.c"
	if ( (*p) == 10 )
		goto tr117;
	goto tr85;
tr117:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st100;
st100:
	if ( ++p == pe )
		goto _test_eof100;
case 100:
#line 1377 "http_parser.c"
	switch( (*p) ) {
		case 13: goto st21;
		case 67: goto st22;
		case 82: goto st101;
		case 84: goto st119;
		case 99: goto st22;
		case 114: goto st101;
		case 116: goto st119;
	}
	goto tr85;
st101:
	if ( ++p == pe )
		goto _test_eof101;
case 101:
	switch( (*p) ) {
		case 65: goto st102;
		case 97: goto st102;
	}
	goto tr27;
st102:
	if ( ++p == pe )
		goto _test_eof102;
case 102:
	switch( (*p) ) {
		case 78: goto st103;
		case 110: goto st103;
	}
	goto tr27;
st103:
	if ( ++p == pe )
		goto _test_eof103;
case 103:
	switch( (*p) ) {
		case 71: goto st104;
		case 103: goto st104;
	}
	goto tr27;
st104:
	if ( ++p == pe )
		goto _test_eof104;
case 104:
	switch( (*p) ) {
		case 69: goto st105;
		case 101: goto st105;
	}
	goto tr27;
st105:
	if ( ++p == pe )
		goto _test_eof105;
case 105:
	if ( (*p) == 58 )
		goto st106;
	goto tr27;
st106:
	if ( ++p == pe )
		goto _test_eof106;
case 106:
	switch( (*p) ) {
		case 9: goto st106;
		case 13: goto st107;
		case 32: goto st106;
		case 98: goto tr125;
	}
	goto tr123;
st107:
	if ( ++p == pe )
		goto _test_eof107;
case 107:
	if ( (*p) == 10 )
		goto tr126;
	goto tr27;
tr126:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st108;
st108:
	if ( ++p == pe )
		goto _test_eof108;
case 108:
#line 1457 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st109;
		case 32: goto st109;
	}
	goto tr27;
st109:
	if ( ++p == pe )
		goto _test_eof109;
case 109:
	switch( (*p) ) {
		case 9: goto st109;
		case 32: goto st109;
		case 98: goto tr125;
	}
	goto tr123;
tr125:
#line 69 "http_parser.rl"
	{
				http->range_beg = http->range_end = -1;
			}
	goto st110;
st110:
	if ( ++p == pe )
		goto _test_eof110;
case 110:
#line 1483 "http_parser.c"
	if ( (*p) == 121 )
		goto st111;
	goto tr123;
st111:
	if ( ++p == pe )
		goto _test_eof111;
case 111:
	if ( (*p) == 116 )
		goto st112;
	goto tr123;
st112:
	if ( ++p == pe )
		goto _test_eof112;
case 112:
	if ( (*p) == 101 )
		goto st113;
	goto tr123;
st113:
	if ( ++p == pe )
		goto _test_eof113;
case 113:
	if ( (*p) == 115 )
		goto st114;
	goto tr123;
st114:
	if ( ++p == pe )
		goto _test_eof114;
case 114:
	if ( (*p) == 61 )
		goto st115;
	goto tr123;
tr134:
#line 72 "http_parser.rl"
	{
				if (http->range_beg < 0)
					http->range_beg = 0;
				if (!length_incr(&http->range_beg, (*p)))
					{p++; cs = 115; goto _out;}
			}
	goto st115;
st115:
	if ( ++p == pe )
		goto _test_eof115;
case 115:
#line 1528 "http_parser.c"
	if ( (*p) == 45 )
		goto st116;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr134;
	goto tr123;
tr137:
#line 79 "http_parser.rl"
	{
				if (http->range_end < 0)
					http->range_end = 0;
				if (!length_incr(&http->range_end, (*p)))
					{p++; cs = 116; goto _out;}
			}
	goto st116;
st116:
	if ( ++p == pe )
		goto _test_eof116;
case 116:
#line 1547 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st117;
		case 13: goto st118;
		case 32: goto st117;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr137;
	goto tr123;
st117:
	if ( ++p == pe )
		goto _test_eof117;
case 117:
	switch( (*p) ) {
		case 9: goto st117;
		case 13: goto st118;
		case 32: goto st117;
	}
	goto tr27;
st118:
	if ( ++p == pe )
		goto _test_eof118;
case 118:
	if ( (*p) == 10 )
		goto tr138;
	goto tr27;
st119:
	if ( ++p == pe )
		goto _test_eof119;
case 119:
	switch( (*p) ) {
		case 82: goto st120;
		case 114: goto st120;
	}
	goto tr27;
st120:
	if ( ++p == pe )
		goto _test_eof120;
case 120:
	switch( (*p) ) {
		case 65: goto st121;
		case 97: goto st121;
	}
	goto tr27;
st121:
	if ( ++p == pe )
		goto _test_eof121;
case 121:
	switch( (*p) ) {
		case 73: goto st122;
		case 78: goto st141;
		case 105: goto st122;
		case 110: goto st141;
	}
	goto tr27;
st122:
	if ( ++p == pe )
		goto _test_eof122;
case 122:
	switch( (*p) ) {
		case 76: goto st123;
		case 108: goto st123;
	}
	goto tr27;
st123:
	if ( ++p == pe )
		goto _test_eof123;
case 123:
	switch( (*p) ) {
		case 69: goto st124;
		case 101: goto st124;
	}
	goto tr27;
st124:
	if ( ++p == pe )
		goto _test_eof124;
case 124:
	switch( (*p) ) {
		case 82: goto st125;
		case 114: goto st125;
	}
	goto tr27;
st125:
	if ( ++p == pe )
		goto _test_eof125;
case 125:
	if ( (*p) == 58 )
		goto st126;
	goto tr27;
st126:
	if ( ++p == pe )
		goto _test_eof126;
case 126:
	switch( (*p) ) {
		case 9: goto st126;
		case 13: goto st127;
		case 32: goto st126;
		case 44: goto st40;
		case 45: goto st130;
		case 67: goto st131;
		case 99: goto st131;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st130;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st130;
	} else
		goto st130;
	goto tr27;
st127:
	if ( ++p == pe )
		goto _test_eof127;
case 127:
	if ( (*p) == 10 )
		goto tr150;
	goto tr27;
tr150:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st128;
st128:
	if ( ++p == pe )
		goto _test_eof128;
case 128:
#line 1673 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st129;
		case 32: goto st129;
	}
	goto tr27;
st129:
	if ( ++p == pe )
		goto _test_eof129;
case 129:
	switch( (*p) ) {
		case 9: goto st129;
		case 32: goto st129;
		case 44: goto st40;
		case 45: goto st130;
		case 67: goto st131;
		case 99: goto st131;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st130;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st130;
	} else
		goto st130;
	goto tr27;
tr161:
#line 90 "http_parser.rl"
	{ http->has_trailer_md5 = 1; }
	goto st130;
st130:
	if ( ++p == pe )
		goto _test_eof130;
case 130:
#line 1708 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st40;
		case 13: goto st41;
		case 32: goto st40;
		case 45: goto st130;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st130;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st130;
	} else
		goto st130;
	goto tr27;
st131:
	if ( ++p == pe )
		goto _test_eof131;
case 131:
	switch( (*p) ) {
		case 9: goto st40;
		case 13: goto st41;
		case 32: goto st40;
		case 45: goto st130;
		case 79: goto st132;
		case 111: goto st132;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st130;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st130;
	} else
		goto st130;
	goto tr27;
st132:
	if ( ++p == pe )
		goto _test_eof132;
case 132:
	switch( (*p) ) {
		case 9: goto st40;
		case 13: goto st41;
		case 32: goto st40;
		case 45: goto st130;
		case 78: goto st133;
		case 110: goto st133;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st130;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st130;
	} else
		goto st130;
	goto tr27;
st133:
	if ( ++p == pe )
		goto _test_eof133;
case 133:
	switch( (*p) ) {
		case 9: goto st40;
		case 13: goto st41;
		case 32: goto st40;
		case 45: goto st130;
		case 84: goto st134;
		case 116: goto st134;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st130;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st130;
	} else
		goto st130;
	goto tr27;
st134:
	if ( ++p == pe )
		goto _test_eof134;
case 134:
	switch( (*p) ) {
		case 9: goto st40;
		case 13: goto st41;
		case 32: goto st40;
		case 45: goto st130;
		case 69: goto st135;
		case 101: goto st135;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st130;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st130;
	} else
		goto st130;
	goto tr27;
st135:
	if ( ++p == pe )
		goto _test_eof135;
case 135:
	switch( (*p) ) {
		case 9: goto st40;
		case 13: goto st41;
		case 32: goto st40;
		case 45: goto st130;
		case 78: goto st136;
		case 110: goto st136;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st130;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st130;
	} else
		goto st130;
	goto tr27;
st136:
	if ( ++p == pe )
		goto _test_eof136;
case 136:
	switch( (*p) ) {
		case 9: goto st40;
		case 13: goto st41;
		case 32: goto st40;
		case 45: goto st130;
		case 84: goto st137;
		case 116: goto st137;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st130;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st130;
	} else
		goto st130;
	goto tr27;
st137:
	if ( ++p == pe )
		goto _test_eof137;
case 137:
	switch( (*p) ) {
		case 9: goto st40;
		case 13: goto st41;
		case 32: goto st40;
		case 45: goto st138;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st130;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st130;
	} else
		goto st130;
	goto tr27;
st138:
	if ( ++p == pe )
		goto _test_eof138;
case 138:
	switch( (*p) ) {
		case 9: goto st40;
		case 13: goto st41;
		case 32: goto st40;
		case 45: goto st130;
		case 77: goto st139;
		case 109: goto st139;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st130;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st130;
	} else
		goto st130;
	goto tr27;
st139:
	if ( ++p == pe )
		goto _test_eof139;
case 139:
	switch( (*p) ) {
		case 9: goto st40;
		case 13: goto st41;
		case 32: goto st40;
		case 45: goto st130;
		case 68: goto st140;
		case 100: goto st140;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st130;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st130;
	} else
		goto st130;
	goto tr27;
st140:
	if ( ++p == pe )
		goto _test_eof140;
case 140:
	switch( (*p) ) {
		case 9: goto st40;
		case 13: goto st41;
		case 32: goto st40;
		case 45: goto st130;
		case 53: goto tr161;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st130;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st130;
	} else
		goto st130;
	goto tr27;
st141:
	if ( ++p == pe )
		goto _test_eof141;
case 141:
	switch( (*p) ) {
		case 83: goto st142;
		case 115: goto st142;
	}
	goto tr27;
st142:
	if ( ++p == pe )
		goto _test_eof142;
case 142:
	switch( (*p) ) {
		case 70: goto st143;
		case 102: goto st143;
	}
	goto tr27;
st143:
	if ( ++p == pe )
		goto _test_eof143;
case 143:
	switch( (*p) ) {
		case 69: goto st144;
		case 101: goto st144;
	}
	goto tr27;
st144:
	if ( ++p == pe )
		goto _test_eof144;
case 144:
	switch( (*p) ) {
		case 82: goto st145;
		case 114: goto st145;
	}
	goto tr27;
st145:
	if ( ++p == pe )
		goto _test_eof145;
case 145:
	if ( (*p) == 45 )
		goto st146;
	goto tr27;
st146:
	if ( ++p == pe )
		goto _test_eof146;
case 146:
	switch( (*p) ) {
		case 69: goto st147;
		case 101: goto st147;
	}
	goto tr27;
st147:
	if ( ++p == pe )
		goto _test_eof147;
case 147:
	switch( (*p) ) {
		case 78: goto st148;
		case 110: goto st148;
	}
	goto tr27;
st148:
	if ( ++p == pe )
		goto _test_eof148;
case 148:
	switch( (*p) ) {
		case 67: goto st149;
		case 99: goto st149;
	}
	goto tr27;
st149:
	if ( ++p == pe )
		goto _test_eof149;
case 149:
	switch( (*p) ) {
		case 79: goto st150;
		case 111: goto st150;
	}
	goto tr27;
st150:
	if ( ++p == pe )
		goto _test_eof150;
case 150:
	switch( (*p) ) {
		case 68: goto st151;
		case 100: goto st151;
	}
	goto tr27;
st151:
	if ( ++p == pe )
		goto _test_eof151;
case 151:
	switch( (*p) ) {
		case 73: goto st152;
		case 105: goto st152;
	}
	goto tr27;
st152:
	if ( ++p == pe )
		goto _test_eof152;
case 152:
	switch( (*p) ) {
		case 78: goto st153;
		case 110: goto st153;
	}
	goto tr27;
st153:
	if ( ++p == pe )
		goto _test_eof153;
case 153:
	switch( (*p) ) {
		case 71: goto st154;
		case 103: goto st154;
	}
	goto tr27;
st154:
	if ( ++p == pe )
		goto _test_eof154;
case 154:
	if ( (*p) == 58 )
		goto st155;
	goto tr27;
st155:
	if ( ++p == pe )
		goto _test_eof155;
case 155:
	switch( (*p) ) {
		case 9: goto st155;
		case 13: goto st156;
		case 32: goto st155;
		case 67: goto st159;
		case 99: goto st159;
	}
	goto tr27;
st156:
	if ( ++p == pe )
		goto _test_eof156;
case 156:
	if ( (*p) == 10 )
		goto tr178;
	goto tr27;
tr178:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st157;
st157:
	if ( ++p == pe )
		goto _test_eof157;
case 157:
#line 2080 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st158;
		case 32: goto st158;
	}
	goto tr27;
st158:
	if ( ++p == pe )
		goto _test_eof158;
case 158:
	switch( (*p) ) {
		case 9: goto st158;
		case 32: goto st158;
		case 67: goto st159;
		case 99: goto st159;
	}
	goto tr27;
st159:
	if ( ++p == pe )
		goto _test_eof159;
case 159:
	switch( (*p) ) {
		case 72: goto st160;
		case 104: goto st160;
	}
	goto tr27;
st160:
	if ( ++p == pe )
		goto _test_eof160;
case 160:
	switch( (*p) ) {
		case 85: goto st161;
		case 117: goto st161;
	}
	goto tr27;
st161:
	if ( ++p == pe )
		goto _test_eof161;
case 161:
	switch( (*p) ) {
		case 78: goto st162;
		case 110: goto st162;
	}
	goto tr27;
st162:
	if ( ++p == pe )
		goto _test_eof162;
case 162:
	switch( (*p) ) {
		case 75: goto st163;
		case 107: goto st163;
	}
	goto tr27;
st163:
	if ( ++p == pe )
		goto _test_eof163;
case 163:
	switch( (*p) ) {
		case 69: goto st164;
		case 101: goto st164;
	}
	goto tr27;
st164:
	if ( ++p == pe )
		goto _test_eof164;
case 164:
	switch( (*p) ) {
		case 68: goto st165;
		case 100: goto st165;
	}
	goto tr27;
st165:
	if ( ++p == pe )
		goto _test_eof165;
case 165:
	switch( (*p) ) {
		case 9: goto tr186;
		case 13: goto tr187;
		case 32: goto tr186;
	}
	goto tr27;
st166:
	if ( ++p == pe )
		goto _test_eof166;
case 166:
	switch( (*p) ) {
		case 65: goto st167;
		case 97: goto st167;
	}
	goto tr27;
st167:
	if ( ++p == pe )
		goto _test_eof167;
case 167:
	switch( (*p) ) {
		case 78: goto st168;
		case 110: goto st168;
	}
	goto tr27;
st168:
	if ( ++p == pe )
		goto _test_eof168;
case 168:
	switch( (*p) ) {
		case 71: goto st169;
		case 103: goto st169;
	}
	goto tr27;
st169:
	if ( ++p == pe )
		goto _test_eof169;
case 169:
	switch( (*p) ) {
		case 69: goto st170;
		case 101: goto st170;
	}
	goto tr27;
st170:
	if ( ++p == pe )
		goto _test_eof170;
case 170:
	if ( (*p) == 58 )
		goto st171;
	goto tr27;
st171:
	if ( ++p == pe )
		goto _test_eof171;
case 171:
	switch( (*p) ) {
		case 9: goto st171;
		case 13: goto st172;
		case 32: goto st171;
		case 98: goto st175;
	}
	goto tr27;
st172:
	if ( ++p == pe )
		goto _test_eof172;
case 172:
	if ( (*p) == 10 )
		goto tr195;
	goto tr27;
tr195:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st173;
st173:
	if ( ++p == pe )
		goto _test_eof173;
case 173:
#line 2230 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st174;
		case 32: goto st174;
	}
	goto tr27;
st174:
	if ( ++p == pe )
		goto _test_eof174;
case 174:
	switch( (*p) ) {
		case 9: goto st174;
		case 32: goto st174;
		case 98: goto st175;
	}
	goto tr27;
st175:
	if ( ++p == pe )
		goto _test_eof175;
case 175:
	if ( (*p) == 121 )
		goto st176;
	goto tr27;
st176:
	if ( ++p == pe )
		goto _test_eof176;
case 176:
	if ( (*p) == 116 )
		goto st177;
	goto tr27;
st177:
	if ( ++p == pe )
		goto _test_eof177;
case 177:
	if ( (*p) == 101 )
		goto st178;
	goto tr27;
st178:
	if ( ++p == pe )
		goto _test_eof178;
case 178:
	if ( (*p) == 115 )
		goto st179;
	goto tr27;
st179:
	if ( ++p == pe )
		goto _test_eof179;
case 179:
	switch( (*p) ) {
		case 9: goto st180;
		case 32: goto st180;
	}
	goto tr27;
st180:
	if ( ++p == pe )
		goto _test_eof180;
case 180:
	switch( (*p) ) {
		case 9: goto st180;
		case 32: goto st180;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr203;
	goto tr202;
tr203:
#line 55 "http_parser.rl"
	{
			if (!length_incr(&http->range_beg, (*p)))
				{p++; cs = 181; goto _out;}
		}
	goto st181;
st181:
	if ( ++p == pe )
		goto _test_eof181;
case 181:
#line 2305 "http_parser.c"
	if ( (*p) == 45 )
		goto st182;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr203;
	goto tr202;
st182:
	if ( ++p == pe )
		goto _test_eof182;
case 182:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr206;
	goto tr205;
tr206:
#line 61 "http_parser.rl"
	{
			if (!length_incr(&http->range_end, (*p)))
				{p++; cs = 183; goto _out;}
		}
	goto st183;
st183:
	if ( ++p == pe )
		goto _test_eof183;
case 183:
#line 2329 "http_parser.c"
	if ( (*p) == 47 )
		goto st184;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr206;
	goto tr205;
st184:
	if ( ++p == pe )
		goto _test_eof184;
case 184:
	if ( (*p) == 42 )
		goto st185;
	goto tr27;
st185:
	if ( ++p == pe )
		goto _test_eof185;
case 185:
	switch( (*p) ) {
		case 9: goto tr209;
		case 13: goto tr210;
		case 32: goto tr209;
	}
	goto tr27;
st186:
	if ( ++p == pe )
		goto _test_eof186;
case 186:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st187;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st187;
	} else
		goto st187;
	goto st0;
st187:
	if ( ++p == pe )
		goto _test_eof187;
case 187:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st188;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st188;
	} else
		goto st188;
	goto st0;
st188:
	if ( ++p == pe )
		goto _test_eof188;
case 188:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st189;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st189;
	} else
		goto st189;
	goto st0;
st189:
	if ( ++p == pe )
		goto _test_eof189;
case 189:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st190;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st190;
	} else
		goto st190;
	goto st0;
st190:
	if ( ++p == pe )
		goto _test_eof190;
case 190:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st191;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st191;
	} else
		goto st191;
	goto st0;
st191:
	if ( ++p == pe )
		goto _test_eof191;
case 191:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st192;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st192;
	} else
		goto st192;
	goto st0;
st192:
	if ( ++p == pe )
		goto _test_eof192;
case 192:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st193;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st193;
	} else
		goto st193;
	goto st0;
st193:
	if ( ++p == pe )
		goto _test_eof193;
case 193:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st194;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st194;
	} else
		goto st194;
	goto st0;
st194:
	if ( ++p == pe )
		goto _test_eof194;
case 194:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st195;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st195;
	} else
		goto st195;
	goto st0;
st195:
	if ( ++p == pe )
		goto _test_eof195;
case 195:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st196;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st196;
	} else
		goto st196;
	goto st0;
st196:
	if ( ++p == pe )
		goto _test_eof196;
case 196:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st197;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st197;
	} else
		goto st197;
	goto st0;
st197:
	if ( ++p == pe )
		goto _test_eof197;
case 197:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st198;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st198;
	} else
		goto st198;
	goto st0;
st198:
	if ( ++p == pe )
		goto _test_eof198;
case 198:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st199;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st199;
	} else
		goto st199;
	goto st0;
st199:
	if ( ++p == pe )
		goto _test_eof199;
case 199:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st200;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st200;
	} else
		goto st200;
	goto st0;
st200:
	if ( ++p == pe )
		goto _test_eof200;
case 200:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st201;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st201;
	} else
		goto st201;
	goto st0;
st201:
	if ( ++p == pe )
		goto _test_eof201;
case 201:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st202;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st202;
	} else
		goto st202;
	goto st0;
st202:
	if ( ++p == pe )
		goto _test_eof202;
case 202:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st203;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st203;
	} else
		goto st203;
	goto st0;
st203:
	if ( ++p == pe )
		goto _test_eof203;
case 203:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st204;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st204;
	} else
		goto st204;
	goto st0;
st204:
	if ( ++p == pe )
		goto _test_eof204;
case 204:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st205;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st205;
	} else
		goto st205;
	goto st0;
st205:
	if ( ++p == pe )
		goto _test_eof205;
case 205:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st206;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st206;
	} else
		goto st206;
	goto st0;
st206:
	if ( ++p == pe )
		goto _test_eof206;
case 206:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st207;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st207;
	} else
		goto st207;
	goto st0;
st207:
	if ( ++p == pe )
		goto _test_eof207;
case 207:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st208;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st208;
	} else
		goto st208;
	goto st0;
st208:
	if ( ++p == pe )
		goto _test_eof208;
case 208:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st209;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st209;
	} else
		goto st209;
	goto st0;
st209:
	if ( ++p == pe )
		goto _test_eof209;
case 209:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st210;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st210;
	} else
		goto st210;
	goto st0;
st210:
	if ( ++p == pe )
		goto _test_eof210;
case 210:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st211;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st211;
	} else
		goto st211;
	goto st0;
st211:
	if ( ++p == pe )
		goto _test_eof211;
case 211:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st212;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st212;
	} else
		goto st212;
	goto st0;
st212:
	if ( ++p == pe )
		goto _test_eof212;
case 212:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st213;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st213;
	} else
		goto st213;
	goto st0;
st213:
	if ( ++p == pe )
		goto _test_eof213;
case 213:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st214;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st214;
	} else
		goto st214;
	goto st0;
st214:
	if ( ++p == pe )
		goto _test_eof214;
case 214:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st215;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st215;
	} else
		goto st215;
	goto st0;
st215:
	if ( ++p == pe )
		goto _test_eof215;
case 215:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st216;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st216;
	} else
		goto st216;
	goto st0;
st216:
	if ( ++p == pe )
		goto _test_eof216;
case 216:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st217;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st217;
	} else
		goto st217;
	goto st0;
st217:
	if ( ++p == pe )
		goto _test_eof217;
case 217:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st218;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st218;
	} else
		goto st218;
	goto st0;
st218:
	if ( ++p == pe )
		goto _test_eof218;
case 218:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st219;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st219;
	} else
		goto st219;
	goto st0;
st219:
	if ( ++p == pe )
		goto _test_eof219;
case 219:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st220;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st220;
	} else
		goto st220;
	goto st0;
st220:
	if ( ++p == pe )
		goto _test_eof220;
case 220:
	if ( (*p) == 32 )
		goto tr14;
	if ( (*p) < 65 ) {
		if ( 45 <= (*p) && (*p) <= 57 )
			goto st221;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st221;
	} else
		goto st221;
	goto st0;
st221:
	if ( ++p == pe )
		goto _test_eof221;
case 221:
	if ( (*p) == 32 )
		goto tr14;
	goto st0;
st222:
	if ( ++p == pe )
		goto _test_eof222;
case 222:
	if ( (*p) == 116 )
		goto st223;
	goto st0;
st223:
	if ( ++p == pe )
		goto _test_eof223;
case 223:
	if ( (*p) == 116 )
		goto st224;
	goto st0;
st224:
	if ( ++p == pe )
		goto _test_eof224;
case 224:
	if ( (*p) == 112 )
		goto st225;
	goto st0;
st225:
	if ( ++p == pe )
		goto _test_eof225;
case 225:
	if ( (*p) == 58 )
		goto st226;
	goto st0;
st226:
	if ( ++p == pe )
		goto _test_eof226;
case 226:
	if ( (*p) == 47 )
		goto st227;
	goto st0;
st227:
	if ( ++p == pe )
		goto _test_eof227;
case 227:
	if ( (*p) == 47 )
		goto st228;
	goto st0;
st228:
	if ( ++p == pe )
		goto _test_eof228;
case 228:
	if ( (*p) == 47 )
		goto st0;
	goto st229;
st229:
	if ( ++p == pe )
		goto _test_eof229;
case 229:
	if ( (*p) == 47 )
		goto tr12;
	goto st229;
tr2:
#line 33 "http_parser.rl"
	{ http->http_method = MOG_HTTP_METHOD_GET; }
	goto st230;
st230:
	if ( ++p == pe )
		goto _test_eof230;
case 230:
#line 2948 "http_parser.c"
	if ( (*p) == 69 )
		goto st231;
	goto st0;
st231:
	if ( ++p == pe )
		goto _test_eof231;
case 231:
	if ( (*p) == 84 )
		goto st7;
	goto st0;
tr3:
#line 34 "http_parser.rl"
	{ http->http_method = MOG_HTTP_METHOD_HEAD; }
	goto st232;
st232:
	if ( ++p == pe )
		goto _test_eof232;
case 232:
#line 2967 "http_parser.c"
	if ( (*p) == 69 )
		goto st233;
	goto st0;
st233:
	if ( ++p == pe )
		goto _test_eof233;
case 233:
	if ( (*p) == 65 )
		goto st234;
	goto st0;
st234:
	if ( ++p == pe )
		goto _test_eof234;
case 234:
	if ( (*p) == 68 )
		goto st7;
	goto st0;
tr4:
#line 37 "http_parser.rl"
	{ http->http_method = MOG_HTTP_METHOD_MKCOL; }
	goto st235;
st235:
	if ( ++p == pe )
		goto _test_eof235;
case 235:
#line 2993 "http_parser.c"
	if ( (*p) == 75 )
		goto st236;
	goto st0;
st236:
	if ( ++p == pe )
		goto _test_eof236;
case 236:
	if ( (*p) == 67 )
		goto st237;
	goto st0;
st237:
	if ( ++p == pe )
		goto _test_eof237;
case 237:
	if ( (*p) == 79 )
		goto st238;
	goto st0;
st238:
	if ( ++p == pe )
		goto _test_eof238;
case 238:
	if ( (*p) == 76 )
		goto st7;
	goto st0;
tr5:
#line 35 "http_parser.rl"
	{ http->http_method = MOG_HTTP_METHOD_PUT; }
	goto st239;
st239:
	if ( ++p == pe )
		goto _test_eof239;
case 239:
#line 3026 "http_parser.c"
	if ( (*p) == 85 )
		goto st231;
	goto st0;
st240:
	if ( ++p == pe )
		goto _test_eof240;
case 240:
	if ( (*p) == 45 )
		goto st241;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st241;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st241;
	} else
		goto st241;
	goto st0;
st241:
	if ( ++p == pe )
		goto _test_eof241;
case 241:
	switch( (*p) ) {
		case 45: goto st241;
		case 58: goto st242;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st241;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st241;
	} else
		goto st241;
	goto st0;
st242:
	if ( ++p == pe )
		goto _test_eof242;
case 242:
	switch( (*p) ) {
		case 9: goto st242;
		case 13: goto st246;
		case 32: goto st242;
		case 127: goto st0;
	}
	if ( 0 <= (*p) && (*p) <= 31 )
		goto st0;
	goto st243;
st243:
	if ( ++p == pe )
		goto _test_eof243;
case 243:
	switch( (*p) ) {
		case 9: goto st244;
		case 13: goto st245;
		case 127: goto st0;
	}
	if ( 0 <= (*p) && (*p) <= 31 )
		goto st0;
	goto st243;
st244:
	if ( ++p == pe )
		goto _test_eof244;
case 244:
	switch( (*p) ) {
		case 9: goto st244;
		case 13: goto st245;
		case 32: goto st244;
	}
	goto st0;
st245:
	if ( ++p == pe )
		goto _test_eof245;
case 245:
	if ( (*p) == 10 )
		goto tr265;
	goto st0;
tr265:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
#line 36 "http_common.rl"
	{
		{goto st249;}
	}
	goto st469;
st469:
	if ( ++p == pe )
		goto _test_eof469;
case 469:
#line 3116 "http_parser.c"
	goto st0;
st246:
	if ( ++p == pe )
		goto _test_eof246;
case 246:
	if ( (*p) == 10 )
		goto tr266;
	goto st0;
tr266:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st247;
st247:
	if ( ++p == pe )
		goto _test_eof247;
case 247:
#line 3133 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st248;
		case 32: goto st248;
	}
	goto st0;
st248:
	if ( ++p == pe )
		goto _test_eof248;
case 248:
	switch( (*p) ) {
		case 9: goto st248;
		case 32: goto st248;
		case 127: goto st0;
	}
	if ( 0 <= (*p) && (*p) <= 31 )
		goto st0;
	goto st243;
st249:
	if ( ++p == pe )
		goto _test_eof249;
case 249:
	switch( (*p) ) {
		case 13: goto st250;
		case 67: goto st251;
		case 99: goto st251;
	}
	goto tr268;
st250:
	if ( ++p == pe )
		goto _test_eof250;
case 250:
	if ( (*p) == 10 )
		goto tr271;
	goto st0;
tr271:
#line 51 "http_common.rl"
	{
		http->chunk_state = MOG_CHUNK_STATE_DONE;
		http->line_end = to_u16(p - buf);
		really_done = 1;
		{p++; cs = 470; goto _out;}
	}
	goto st470;
st470:
	if ( ++p == pe )
		goto _test_eof470;
case 470:
#line 3181 "http_parser.c"
	goto st0;
st251:
	if ( ++p == pe )
		goto _test_eof251;
case 251:
	switch( (*p) ) {
		case 79: goto st252;
		case 111: goto st252;
	}
	goto tr268;
st252:
	if ( ++p == pe )
		goto _test_eof252;
case 252:
	switch( (*p) ) {
		case 78: goto st253;
		case 110: goto st253;
	}
	goto tr268;
st253:
	if ( ++p == pe )
		goto _test_eof253;
case 253:
	switch( (*p) ) {
		case 84: goto st254;
		case 116: goto st254;
	}
	goto tr268;
st254:
	if ( ++p == pe )
		goto _test_eof254;
case 254:
	switch( (*p) ) {
		case 69: goto st255;
		case 101: goto st255;
	}
	goto tr268;
st255:
	if ( ++p == pe )
		goto _test_eof255;
case 255:
	switch( (*p) ) {
		case 78: goto st256;
		case 110: goto st256;
	}
	goto tr268;
st256:
	if ( ++p == pe )
		goto _test_eof256;
case 256:
	switch( (*p) ) {
		case 84: goto st257;
		case 116: goto st257;
	}
	goto tr268;
st257:
	if ( ++p == pe )
		goto _test_eof257;
case 257:
	if ( (*p) == 45 )
		goto st258;
	goto tr268;
st258:
	if ( ++p == pe )
		goto _test_eof258;
case 258:
	switch( (*p) ) {
		case 77: goto st259;
		case 109: goto st259;
	}
	goto tr268;
st259:
	if ( ++p == pe )
		goto _test_eof259;
case 259:
	switch( (*p) ) {
		case 68: goto st260;
		case 100: goto st260;
	}
	goto tr268;
st260:
	if ( ++p == pe )
		goto _test_eof260;
case 260:
	if ( (*p) == 53 )
		goto st261;
	goto tr268;
st261:
	if ( ++p == pe )
		goto _test_eof261;
case 261:
	if ( (*p) == 58 )
		goto st262;
	goto tr268;
st262:
	if ( ++p == pe )
		goto _test_eof262;
case 262:
	switch( (*p) ) {
		case 9: goto st262;
		case 13: goto st263;
		case 32: goto st262;
		case 43: goto tr285;
	}
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto tr285;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr285;
	} else
		goto tr285;
	goto tr283;
st263:
	if ( ++p == pe )
		goto _test_eof263;
case 263:
	if ( (*p) == 10 )
		goto tr286;
	goto tr268;
tr286:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st264;
st264:
	if ( ++p == pe )
		goto _test_eof264;
case 264:
#line 3310 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st265;
		case 32: goto st265;
	}
	goto tr268;
st265:
	if ( ++p == pe )
		goto _test_eof265;
case 265:
	switch( (*p) ) {
		case 9: goto st265;
		case 32: goto st265;
		case 43: goto tr285;
	}
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto tr285;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr285;
	} else
		goto tr285;
	goto tr283;
tr285:
#line 15 "http_common.rl"
	{ http->tmp_tip = to_u16(p - buf); }
	goto st266;
st266:
	if ( ++p == pe )
		goto _test_eof266;
case 266:
#line 3342 "http_parser.c"
	if ( (*p) == 43 )
		goto st267;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st267;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st267;
	} else
		goto st267;
	goto tr283;
st267:
	if ( ++p == pe )
		goto _test_eof267;
case 267:
	if ( (*p) == 43 )
		goto st268;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st268;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st268;
	} else
		goto st268;
	goto tr283;
st268:
	if ( ++p == pe )
		goto _test_eof268;
case 268:
	if ( (*p) == 43 )
		goto st269;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st269;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st269;
	} else
		goto st269;
	goto tr283;
st269:
	if ( ++p == pe )
		goto _test_eof269;
case 269:
	if ( (*p) == 43 )
		goto st270;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st270;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st270;
	} else
		goto st270;
	goto tr283;
st270:
	if ( ++p == pe )
		goto _test_eof270;
case 270:
	if ( (*p) == 43 )
		goto st271;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st271;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st271;
	} else
		goto st271;
	goto tr283;
st271:
	if ( ++p == pe )
		goto _test_eof271;
case 271:
	if ( (*p) == 43 )
		goto st272;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st272;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st272;
	} else
		goto st272;
	goto tr283;
st272:
	if ( ++p == pe )
		goto _test_eof272;
case 272:
	if ( (*p) == 43 )
		goto st273;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st273;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st273;
	} else
		goto st273;
	goto tr283;
st273:
	if ( ++p == pe )
		goto _test_eof273;
case 273:
	if ( (*p) == 43 )
		goto st274;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st274;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st274;
	} else
		goto st274;
	goto tr283;
st274:
	if ( ++p == pe )
		goto _test_eof274;
case 274:
	if ( (*p) == 43 )
		goto st275;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st275;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st275;
	} else
		goto st275;
	goto tr283;
st275:
	if ( ++p == pe )
		goto _test_eof275;
case 275:
	if ( (*p) == 43 )
		goto st276;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st276;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st276;
	} else
		goto st276;
	goto tr283;
st276:
	if ( ++p == pe )
		goto _test_eof276;
case 276:
	if ( (*p) == 43 )
		goto st277;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st277;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st277;
	} else
		goto st277;
	goto tr283;
st277:
	if ( ++p == pe )
		goto _test_eof277;
case 277:
	if ( (*p) == 43 )
		goto st278;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st278;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st278;
	} else
		goto st278;
	goto tr283;
st278:
	if ( ++p == pe )
		goto _test_eof278;
case 278:
	if ( (*p) == 43 )
		goto st279;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st279;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st279;
	} else
		goto st279;
	goto tr283;
st279:
	if ( ++p == pe )
		goto _test_eof279;
case 279:
	if ( (*p) == 43 )
		goto st280;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st280;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st280;
	} else
		goto st280;
	goto tr283;
st280:
	if ( ++p == pe )
		goto _test_eof280;
case 280:
	if ( (*p) == 43 )
		goto st281;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st281;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st281;
	} else
		goto st281;
	goto tr283;
st281:
	if ( ++p == pe )
		goto _test_eof281;
case 281:
	if ( (*p) == 43 )
		goto st282;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st282;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st282;
	} else
		goto st282;
	goto tr283;
st282:
	if ( ++p == pe )
		goto _test_eof282;
case 282:
	if ( (*p) == 43 )
		goto st283;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st283;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st283;
	} else
		goto st283;
	goto tr283;
st283:
	if ( ++p == pe )
		goto _test_eof283;
case 283:
	if ( (*p) == 43 )
		goto st284;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st284;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st284;
	} else
		goto st284;
	goto tr283;
st284:
	if ( ++p == pe )
		goto _test_eof284;
case 284:
	if ( (*p) == 43 )
		goto st285;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st285;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st285;
	} else
		goto st285;
	goto tr283;
st285:
	if ( ++p == pe )
		goto _test_eof285;
case 285:
	if ( (*p) == 43 )
		goto st286;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st286;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st286;
	} else
		goto st286;
	goto tr283;
st286:
	if ( ++p == pe )
		goto _test_eof286;
case 286:
	if ( (*p) == 43 )
		goto st287;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st287;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st287;
	} else
		goto st287;
	goto tr283;
st287:
	if ( ++p == pe )
		goto _test_eof287;
case 287:
	if ( (*p) == 61 )
		goto st288;
	goto tr283;
st288:
	if ( ++p == pe )
		goto _test_eof288;
case 288:
	if ( (*p) == 61 )
		goto st289;
	goto tr283;
st289:
	if ( ++p == pe )
		goto _test_eof289;
case 289:
	switch( (*p) ) {
		case 9: goto tr311;
		case 13: goto tr312;
		case 32: goto tr311;
	}
	goto tr283;
tr311:
#line 17 "http_common.rl"
	{
			uint16_t tmp_end = to_u16(p - buf);
			char *in = buf + http->tmp_tip;
			size_t inlen = tmp_end - http->tmp_tip;
			char *out = (char *)http->expect_md5;
			size_t outlen = sizeof(http->expect_md5);
			bool rc;

			rc = base64_decode_ctx(NULL, in, inlen, out, &outlen);
			assert(rc == true && outlen == 16
			       && "base64_decoder broke for HTTP");
			http->has_expect_md5 = 1;
		  }
	goto st290;
st290:
	if ( ++p == pe )
		goto _test_eof290;
case 290:
#line 3698 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st290;
		case 13: goto st291;
		case 32: goto st290;
	}
	goto tr283;
tr312:
#line 17 "http_common.rl"
	{
			uint16_t tmp_end = to_u16(p - buf);
			char *in = buf + http->tmp_tip;
			size_t inlen = tmp_end - http->tmp_tip;
			char *out = (char *)http->expect_md5;
			size_t outlen = sizeof(http->expect_md5);
			bool rc;

			rc = base64_decode_ctx(NULL, in, inlen, out, &outlen);
			assert(rc == true && outlen == 16
			       && "base64_decoder broke for HTTP");
			http->has_expect_md5 = 1;
		  }
	goto st291;
st291:
	if ( ++p == pe )
		goto _test_eof291;
case 291:
#line 3725 "http_parser.c"
	if ( (*p) == 10 )
		goto tr315;
	goto tr283;
tr315:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st292;
st292:
	if ( ++p == pe )
		goto _test_eof292;
case 292:
#line 3737 "http_parser.c"
	switch( (*p) ) {
		case 13: goto st250;
		case 67: goto st251;
		case 99: goto st251;
	}
	goto tr283;
st293:
	if ( ++p == pe )
		goto _test_eof293;
case 293:
	if ( (*p) == 45 )
		goto st294;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st294;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st294;
	} else
		goto st294;
	goto st0;
st294:
	if ( ++p == pe )
		goto _test_eof294;
case 294:
	switch( (*p) ) {
		case 45: goto st294;
		case 58: goto st295;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st294;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st294;
	} else
		goto st294;
	goto st0;
st295:
	if ( ++p == pe )
		goto _test_eof295;
case 295:
	switch( (*p) ) {
		case 9: goto st295;
		case 13: goto st299;
		case 32: goto st295;
		case 127: goto st0;
	}
	if ( 0 <= (*p) && (*p) <= 31 )
		goto st0;
	goto st296;
st296:
	if ( ++p == pe )
		goto _test_eof296;
case 296:
	switch( (*p) ) {
		case 9: goto st297;
		case 13: goto st298;
		case 127: goto st0;
	}
	if ( 0 <= (*p) && (*p) <= 31 )
		goto st0;
	goto st296;
st297:
	if ( ++p == pe )
		goto _test_eof297;
case 297:
	switch( (*p) ) {
		case 9: goto st297;
		case 13: goto st298;
		case 32: goto st297;
	}
	goto st0;
st298:
	if ( ++p == pe )
		goto _test_eof298;
case 298:
	if ( (*p) == 10 )
		goto tr322;
	goto st0;
tr322:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
#line 28 "http_parser.rl"
	{
		{goto st302;}
	}
	goto st471;
st471:
	if ( ++p == pe )
		goto _test_eof471;
case 471:
#line 3830 "http_parser.c"
	goto st0;
st299:
	if ( ++p == pe )
		goto _test_eof299;
case 299:
	if ( (*p) == 10 )
		goto tr323;
	goto st0;
tr323:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st300;
st300:
	if ( ++p == pe )
		goto _test_eof300;
case 300:
#line 3847 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st301;
		case 32: goto st301;
	}
	goto st0;
st301:
	if ( ++p == pe )
		goto _test_eof301;
case 301:
	switch( (*p) ) {
		case 9: goto st301;
		case 32: goto st301;
		case 127: goto st0;
	}
	if ( 0 <= (*p) && (*p) <= 31 )
		goto st0;
	goto st296;
tr352:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st302;
tr433:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
#line 86 "http_parser.rl"
	{ http->has_range = 1; }
	goto st302;
st302:
	if ( ++p == pe )
		goto _test_eof302;
case 302:
#line 3879 "http_parser.c"
	switch( (*p) ) {
		case 13: goto st303;
		case 67: goto st304;
		case 82: goto st383;
		case 84: goto st401;
		case 99: goto st304;
		case 114: goto st383;
		case 116: goto st401;
	}
	goto tr27;
st303:
	if ( ++p == pe )
		goto _test_eof303;
case 303:
	if ( (*p) == 10 )
		goto tr329;
	goto st0;
tr329:
#line 113 "http_parser.rl"
	{ really_done = 1; {p++; cs = 472; goto _out;} }
	goto st472;
st472:
	if ( ++p == pe )
		goto _test_eof472;
case 472:
#line 3905 "http_parser.c"
	goto st0;
st304:
	if ( ++p == pe )
		goto _test_eof304;
case 304:
	switch( (*p) ) {
		case 79: goto st305;
		case 111: goto st305;
	}
	goto tr27;
st305:
	if ( ++p == pe )
		goto _test_eof305;
case 305:
	switch( (*p) ) {
		case 78: goto st306;
		case 110: goto st306;
	}
	goto tr27;
st306:
	if ( ++p == pe )
		goto _test_eof306;
case 306:
	switch( (*p) ) {
		case 78: goto st307;
		case 84: goto st333;
		case 110: goto st307;
		case 116: goto st333;
	}
	goto tr27;
st307:
	if ( ++p == pe )
		goto _test_eof307;
case 307:
	switch( (*p) ) {
		case 69: goto st308;
		case 101: goto st308;
	}
	goto tr27;
st308:
	if ( ++p == pe )
		goto _test_eof308;
case 308:
	switch( (*p) ) {
		case 67: goto st309;
		case 99: goto st309;
	}
	goto tr27;
st309:
	if ( ++p == pe )
		goto _test_eof309;
case 309:
	switch( (*p) ) {
		case 84: goto st310;
		case 116: goto st310;
	}
	goto tr27;
st310:
	if ( ++p == pe )
		goto _test_eof310;
case 310:
	switch( (*p) ) {
		case 73: goto st311;
		case 105: goto st311;
	}
	goto tr27;
st311:
	if ( ++p == pe )
		goto _test_eof311;
case 311:
	switch( (*p) ) {
		case 79: goto st312;
		case 111: goto st312;
	}
	goto tr27;
st312:
	if ( ++p == pe )
		goto _test_eof312;
case 312:
	switch( (*p) ) {
		case 78: goto st313;
		case 110: goto st313;
	}
	goto tr27;
st313:
	if ( ++p == pe )
		goto _test_eof313;
case 313:
	if ( (*p) == 58 )
		goto st314;
	goto tr27;
st314:
	if ( ++p == pe )
		goto _test_eof314;
case 314:
	switch( (*p) ) {
		case 9: goto st314;
		case 13: goto st315;
		case 32: goto st314;
		case 67: goto st318;
		case 75: goto st324;
		case 99: goto st318;
		case 107: goto st324;
	}
	goto tr27;
st315:
	if ( ++p == pe )
		goto _test_eof315;
case 315:
	if ( (*p) == 10 )
		goto tr344;
	goto tr27;
tr344:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st316;
st316:
	if ( ++p == pe )
		goto _test_eof316;
case 316:
#line 4026 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st317;
		case 32: goto st317;
	}
	goto tr27;
st317:
	if ( ++p == pe )
		goto _test_eof317;
case 317:
	switch( (*p) ) {
		case 9: goto st317;
		case 32: goto st317;
		case 67: goto st318;
		case 75: goto st324;
		case 99: goto st318;
		case 107: goto st324;
	}
	goto tr27;
st318:
	if ( ++p == pe )
		goto _test_eof318;
case 318:
	switch( (*p) ) {
		case 76: goto st319;
		case 108: goto st319;
	}
	goto tr27;
st319:
	if ( ++p == pe )
		goto _test_eof319;
case 319:
	switch( (*p) ) {
		case 79: goto st320;
		case 111: goto st320;
	}
	goto tr27;
st320:
	if ( ++p == pe )
		goto _test_eof320;
case 320:
	switch( (*p) ) {
		case 83: goto st321;
		case 115: goto st321;
	}
	goto tr27;
st321:
	if ( ++p == pe )
		goto _test_eof321;
case 321:
	switch( (*p) ) {
		case 69: goto tr349;
		case 101: goto tr349;
	}
	goto tr27;
tr349:
#line 94 "http_parser.rl"
	{ http->persistent = 0; }
	goto st322;
tr361:
#line 95 "http_parser.rl"
	{ http->persistent = 1; }
	goto st322;
tr481:
#line 88 "http_parser.rl"
	{ http->chunked = 1; }
	goto st322;
tr502:
#line 67 "http_parser.rl"
	{ http->has_content_range = 1; }
	goto st322;
st322:
	if ( ++p == pe )
		goto _test_eof322;
case 322:
#line 4101 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st322;
		case 13: goto st323;
		case 32: goto st322;
	}
	goto tr27;
tr482:
#line 88 "http_parser.rl"
	{ http->chunked = 1; }
	goto st323;
tr503:
#line 67 "http_parser.rl"
	{ http->has_content_range = 1; }
	goto st323;
st323:
	if ( ++p == pe )
		goto _test_eof323;
case 323:
#line 4120 "http_parser.c"
	if ( (*p) == 10 )
		goto tr352;
	goto tr27;
st324:
	if ( ++p == pe )
		goto _test_eof324;
case 324:
	switch( (*p) ) {
		case 69: goto st325;
		case 101: goto st325;
	}
	goto tr27;
st325:
	if ( ++p == pe )
		goto _test_eof325;
case 325:
	switch( (*p) ) {
		case 69: goto st326;
		case 101: goto st326;
	}
	goto tr27;
st326:
	if ( ++p == pe )
		goto _test_eof326;
case 326:
	switch( (*p) ) {
		case 80: goto st327;
		case 112: goto st327;
	}
	goto tr27;
st327:
	if ( ++p == pe )
		goto _test_eof327;
case 327:
	if ( (*p) == 45 )
		goto st328;
	goto tr27;
st328:
	if ( ++p == pe )
		goto _test_eof328;
case 328:
	switch( (*p) ) {
		case 65: goto st329;
		case 97: goto st329;
	}
	goto tr27;
st329:
	if ( ++p == pe )
		goto _test_eof329;
case 329:
	switch( (*p) ) {
		case 76: goto st330;
		case 108: goto st330;
	}
	goto tr27;
st330:
	if ( ++p == pe )
		goto _test_eof330;
case 330:
	switch( (*p) ) {
		case 73: goto st331;
		case 105: goto st331;
	}
	goto tr27;
st331:
	if ( ++p == pe )
		goto _test_eof331;
case 331:
	switch( (*p) ) {
		case 86: goto st332;
		case 118: goto st332;
	}
	goto tr27;
st332:
	if ( ++p == pe )
		goto _test_eof332;
case 332:
	switch( (*p) ) {
		case 69: goto tr361;
		case 101: goto tr361;
	}
	goto tr27;
st333:
	if ( ++p == pe )
		goto _test_eof333;
case 333:
	switch( (*p) ) {
		case 69: goto st334;
		case 101: goto st334;
	}
	goto tr27;
st334:
	if ( ++p == pe )
		goto _test_eof334;
case 334:
	switch( (*p) ) {
		case 78: goto st335;
		case 110: goto st335;
	}
	goto tr27;
st335:
	if ( ++p == pe )
		goto _test_eof335;
case 335:
	switch( (*p) ) {
		case 84: goto st336;
		case 116: goto st336;
	}
	goto tr27;
st336:
	if ( ++p == pe )
		goto _test_eof336;
case 336:
	if ( (*p) == 45 )
		goto st337;
	goto tr27;
st337:
	if ( ++p == pe )
		goto _test_eof337;
case 337:
	switch( (*p) ) {
		case 76: goto st338;
		case 77: goto st349;
		case 82: goto st448;
		case 108: goto st338;
		case 109: goto st349;
		case 114: goto st448;
	}
	goto tr27;
st338:
	if ( ++p == pe )
		goto _test_eof338;
case 338:
	switch( (*p) ) {
		case 69: goto st339;
		case 101: goto st339;
	}
	goto tr27;
st339:
	if ( ++p == pe )
		goto _test_eof339;
case 339:
	switch( (*p) ) {
		case 78: goto st340;
		case 110: goto st340;
	}
	goto tr27;
st340:
	if ( ++p == pe )
		goto _test_eof340;
case 340:
	switch( (*p) ) {
		case 71: goto st341;
		case 103: goto st341;
	}
	goto tr27;
st341:
	if ( ++p == pe )
		goto _test_eof341;
case 341:
	switch( (*p) ) {
		case 84: goto st342;
		case 116: goto st342;
	}
	goto tr27;
st342:
	if ( ++p == pe )
		goto _test_eof342;
case 342:
	switch( (*p) ) {
		case 72: goto st343;
		case 104: goto st343;
	}
	goto tr27;
st343:
	if ( ++p == pe )
		goto _test_eof343;
case 343:
	if ( (*p) == 58 )
		goto st344;
	goto tr27;
st344:
	if ( ++p == pe )
		goto _test_eof344;
case 344:
	switch( (*p) ) {
		case 9: goto st344;
		case 13: goto st345;
		case 32: goto st344;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr376;
	goto tr77;
st345:
	if ( ++p == pe )
		goto _test_eof345;
case 345:
	if ( (*p) == 10 )
		goto tr377;
	goto tr27;
tr377:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st346;
st346:
	if ( ++p == pe )
		goto _test_eof346;
case 346:
#line 4329 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st347;
		case 32: goto st347;
	}
	goto tr27;
st347:
	if ( ++p == pe )
		goto _test_eof347;
case 347:
	switch( (*p) ) {
		case 9: goto st347;
		case 32: goto st347;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr376;
	goto tr77;
tr376:
#line 48 "http_parser.rl"
	{
			if (!length_incr(&http->content_len, (*p)))
				{p++; cs = 348; goto _out;}
		}
	goto st348;
st348:
	if ( ++p == pe )
		goto _test_eof348;
case 348:
#line 4357 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st322;
		case 13: goto st323;
		case 32: goto st322;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr376;
	goto tr77;
st349:
	if ( ++p == pe )
		goto _test_eof349;
case 349:
	switch( (*p) ) {
		case 68: goto st350;
		case 100: goto st350;
	}
	goto tr27;
st350:
	if ( ++p == pe )
		goto _test_eof350;
case 350:
	if ( (*p) == 53 )
		goto st351;
	goto tr27;
st351:
	if ( ++p == pe )
		goto _test_eof351;
case 351:
	if ( (*p) == 58 )
		goto st352;
	goto tr27;
st352:
	if ( ++p == pe )
		goto _test_eof352;
case 352:
	switch( (*p) ) {
		case 9: goto st352;
		case 13: goto st353;
		case 32: goto st352;
		case 43: goto tr383;
	}
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto tr383;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr383;
	} else
		goto tr383;
	goto tr85;
st353:
	if ( ++p == pe )
		goto _test_eof353;
case 353:
	if ( (*p) == 10 )
		goto tr384;
	goto tr27;
tr384:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st354;
st354:
	if ( ++p == pe )
		goto _test_eof354;
case 354:
#line 4423 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st355;
		case 32: goto st355;
	}
	goto tr27;
st355:
	if ( ++p == pe )
		goto _test_eof355;
case 355:
	switch( (*p) ) {
		case 9: goto st355;
		case 32: goto st355;
		case 43: goto tr383;
	}
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto tr383;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr383;
	} else
		goto tr383;
	goto tr85;
tr383:
#line 15 "http_common.rl"
	{ http->tmp_tip = to_u16(p - buf); }
	goto st356;
st356:
	if ( ++p == pe )
		goto _test_eof356;
case 356:
#line 4455 "http_parser.c"
	if ( (*p) == 43 )
		goto st357;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st357;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st357;
	} else
		goto st357;
	goto tr85;
st357:
	if ( ++p == pe )
		goto _test_eof357;
case 357:
	if ( (*p) == 43 )
		goto st358;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st358;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st358;
	} else
		goto st358;
	goto tr85;
st358:
	if ( ++p == pe )
		goto _test_eof358;
case 358:
	if ( (*p) == 43 )
		goto st359;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st359;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st359;
	} else
		goto st359;
	goto tr85;
st359:
	if ( ++p == pe )
		goto _test_eof359;
case 359:
	if ( (*p) == 43 )
		goto st360;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st360;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st360;
	} else
		goto st360;
	goto tr85;
st360:
	if ( ++p == pe )
		goto _test_eof360;
case 360:
	if ( (*p) == 43 )
		goto st361;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st361;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st361;
	} else
		goto st361;
	goto tr85;
st361:
	if ( ++p == pe )
		goto _test_eof361;
case 361:
	if ( (*p) == 43 )
		goto st362;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st362;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st362;
	} else
		goto st362;
	goto tr85;
st362:
	if ( ++p == pe )
		goto _test_eof362;
case 362:
	if ( (*p) == 43 )
		goto st363;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st363;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st363;
	} else
		goto st363;
	goto tr85;
st363:
	if ( ++p == pe )
		goto _test_eof363;
case 363:
	if ( (*p) == 43 )
		goto st364;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st364;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st364;
	} else
		goto st364;
	goto tr85;
st364:
	if ( ++p == pe )
		goto _test_eof364;
case 364:
	if ( (*p) == 43 )
		goto st365;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st365;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st365;
	} else
		goto st365;
	goto tr85;
st365:
	if ( ++p == pe )
		goto _test_eof365;
case 365:
	if ( (*p) == 43 )
		goto st366;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st366;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st366;
	} else
		goto st366;
	goto tr85;
st366:
	if ( ++p == pe )
		goto _test_eof366;
case 366:
	if ( (*p) == 43 )
		goto st367;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st367;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st367;
	} else
		goto st367;
	goto tr85;
st367:
	if ( ++p == pe )
		goto _test_eof367;
case 367:
	if ( (*p) == 43 )
		goto st368;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st368;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st368;
	} else
		goto st368;
	goto tr85;
st368:
	if ( ++p == pe )
		goto _test_eof368;
case 368:
	if ( (*p) == 43 )
		goto st369;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st369;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st369;
	} else
		goto st369;
	goto tr85;
st369:
	if ( ++p == pe )
		goto _test_eof369;
case 369:
	if ( (*p) == 43 )
		goto st370;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st370;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st370;
	} else
		goto st370;
	goto tr85;
st370:
	if ( ++p == pe )
		goto _test_eof370;
case 370:
	if ( (*p) == 43 )
		goto st371;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st371;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st371;
	} else
		goto st371;
	goto tr85;
st371:
	if ( ++p == pe )
		goto _test_eof371;
case 371:
	if ( (*p) == 43 )
		goto st372;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st372;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st372;
	} else
		goto st372;
	goto tr85;
st372:
	if ( ++p == pe )
		goto _test_eof372;
case 372:
	if ( (*p) == 43 )
		goto st373;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st373;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st373;
	} else
		goto st373;
	goto tr85;
st373:
	if ( ++p == pe )
		goto _test_eof373;
case 373:
	if ( (*p) == 43 )
		goto st374;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st374;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st374;
	} else
		goto st374;
	goto tr85;
st374:
	if ( ++p == pe )
		goto _test_eof374;
case 374:
	if ( (*p) == 43 )
		goto st375;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st375;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st375;
	} else
		goto st375;
	goto tr85;
st375:
	if ( ++p == pe )
		goto _test_eof375;
case 375:
	if ( (*p) == 43 )
		goto st376;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st376;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st376;
	} else
		goto st376;
	goto tr85;
st376:
	if ( ++p == pe )
		goto _test_eof376;
case 376:
	if ( (*p) == 43 )
		goto st377;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st377;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st377;
	} else
		goto st377;
	goto tr85;
st377:
	if ( ++p == pe )
		goto _test_eof377;
case 377:
	if ( (*p) == 61 )
		goto st378;
	goto tr85;
st378:
	if ( ++p == pe )
		goto _test_eof378;
case 378:
	if ( (*p) == 61 )
		goto st379;
	goto tr85;
st379:
	if ( ++p == pe )
		goto _test_eof379;
case 379:
	switch( (*p) ) {
		case 9: goto tr409;
		case 13: goto tr410;
		case 32: goto tr409;
	}
	goto tr85;
tr409:
#line 17 "http_common.rl"
	{
			uint16_t tmp_end = to_u16(p - buf);
			char *in = buf + http->tmp_tip;
			size_t inlen = tmp_end - http->tmp_tip;
			char *out = (char *)http->expect_md5;
			size_t outlen = sizeof(http->expect_md5);
			bool rc;

			rc = base64_decode_ctx(NULL, in, inlen, out, &outlen);
			assert(rc == true && outlen == 16
			       && "base64_decoder broke for HTTP");
			http->has_expect_md5 = 1;
		  }
	goto st380;
st380:
	if ( ++p == pe )
		goto _test_eof380;
case 380:
#line 4811 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st380;
		case 13: goto st381;
		case 32: goto st380;
	}
	goto tr85;
tr410:
#line 17 "http_common.rl"
	{
			uint16_t tmp_end = to_u16(p - buf);
			char *in = buf + http->tmp_tip;
			size_t inlen = tmp_end - http->tmp_tip;
			char *out = (char *)http->expect_md5;
			size_t outlen = sizeof(http->expect_md5);
			bool rc;

			rc = base64_decode_ctx(NULL, in, inlen, out, &outlen);
			assert(rc == true && outlen == 16
			       && "base64_decoder broke for HTTP");
			http->has_expect_md5 = 1;
		  }
	goto st381;
st381:
	if ( ++p == pe )
		goto _test_eof381;
case 381:
#line 4838 "http_parser.c"
	if ( (*p) == 10 )
		goto tr413;
	goto tr85;
tr413:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st382;
st382:
	if ( ++p == pe )
		goto _test_eof382;
case 382:
#line 4850 "http_parser.c"
	switch( (*p) ) {
		case 13: goto st303;
		case 67: goto st304;
		case 82: goto st383;
		case 84: goto st401;
		case 99: goto st304;
		case 114: goto st383;
		case 116: goto st401;
	}
	goto tr85;
st383:
	if ( ++p == pe )
		goto _test_eof383;
case 383:
	switch( (*p) ) {
		case 65: goto st384;
		case 97: goto st384;
	}
	goto tr27;
st384:
	if ( ++p == pe )
		goto _test_eof384;
case 384:
	switch( (*p) ) {
		case 78: goto st385;
		case 110: goto st385;
	}
	goto tr27;
st385:
	if ( ++p == pe )
		goto _test_eof385;
case 385:
	switch( (*p) ) {
		case 71: goto st386;
		case 103: goto st386;
	}
	goto tr27;
st386:
	if ( ++p == pe )
		goto _test_eof386;
case 386:
	switch( (*p) ) {
		case 69: goto st387;
		case 101: goto st387;
	}
	goto tr27;
st387:
	if ( ++p == pe )
		goto _test_eof387;
case 387:
	if ( (*p) == 58 )
		goto st388;
	goto tr27;
st388:
	if ( ++p == pe )
		goto _test_eof388;
case 388:
	switch( (*p) ) {
		case 9: goto st388;
		case 13: goto st389;
		case 32: goto st388;
		case 98: goto tr420;
	}
	goto tr123;
st389:
	if ( ++p == pe )
		goto _test_eof389;
case 389:
	if ( (*p) == 10 )
		goto tr421;
	goto tr27;
tr421:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st390;
st390:
	if ( ++p == pe )
		goto _test_eof390;
case 390:
#line 4930 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st391;
		case 32: goto st391;
	}
	goto tr27;
st391:
	if ( ++p == pe )
		goto _test_eof391;
case 391:
	switch( (*p) ) {
		case 9: goto st391;
		case 32: goto st391;
		case 98: goto tr420;
	}
	goto tr123;
tr420:
#line 69 "http_parser.rl"
	{
				http->range_beg = http->range_end = -1;
			}
	goto st392;
st392:
	if ( ++p == pe )
		goto _test_eof392;
case 392:
#line 4956 "http_parser.c"
	if ( (*p) == 121 )
		goto st393;
	goto tr123;
st393:
	if ( ++p == pe )
		goto _test_eof393;
case 393:
	if ( (*p) == 116 )
		goto st394;
	goto tr123;
st394:
	if ( ++p == pe )
		goto _test_eof394;
case 394:
	if ( (*p) == 101 )
		goto st395;
	goto tr123;
st395:
	if ( ++p == pe )
		goto _test_eof395;
case 395:
	if ( (*p) == 115 )
		goto st396;
	goto tr123;
st396:
	if ( ++p == pe )
		goto _test_eof396;
case 396:
	if ( (*p) == 61 )
		goto st397;
	goto tr123;
tr429:
#line 72 "http_parser.rl"
	{
				if (http->range_beg < 0)
					http->range_beg = 0;
				if (!length_incr(&http->range_beg, (*p)))
					{p++; cs = 397; goto _out;}
			}
	goto st397;
st397:
	if ( ++p == pe )
		goto _test_eof397;
case 397:
#line 5001 "http_parser.c"
	if ( (*p) == 45 )
		goto st398;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr429;
	goto tr123;
tr432:
#line 79 "http_parser.rl"
	{
				if (http->range_end < 0)
					http->range_end = 0;
				if (!length_incr(&http->range_end, (*p)))
					{p++; cs = 398; goto _out;}
			}
	goto st398;
st398:
	if ( ++p == pe )
		goto _test_eof398;
case 398:
#line 5020 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st399;
		case 13: goto st400;
		case 32: goto st399;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr432;
	goto tr123;
st399:
	if ( ++p == pe )
		goto _test_eof399;
case 399:
	switch( (*p) ) {
		case 9: goto st399;
		case 13: goto st400;
		case 32: goto st399;
	}
	goto tr27;
st400:
	if ( ++p == pe )
		goto _test_eof400;
case 400:
	if ( (*p) == 10 )
		goto tr433;
	goto tr27;
st401:
	if ( ++p == pe )
		goto _test_eof401;
case 401:
	switch( (*p) ) {
		case 82: goto st402;
		case 114: goto st402;
	}
	goto tr27;
st402:
	if ( ++p == pe )
		goto _test_eof402;
case 402:
	switch( (*p) ) {
		case 65: goto st403;
		case 97: goto st403;
	}
	goto tr27;
st403:
	if ( ++p == pe )
		goto _test_eof403;
case 403:
	switch( (*p) ) {
		case 73: goto st404;
		case 78: goto st423;
		case 105: goto st404;
		case 110: goto st423;
	}
	goto tr27;
st404:
	if ( ++p == pe )
		goto _test_eof404;
case 404:
	switch( (*p) ) {
		case 76: goto st405;
		case 108: goto st405;
	}
	goto tr27;
st405:
	if ( ++p == pe )
		goto _test_eof405;
case 405:
	switch( (*p) ) {
		case 69: goto st406;
		case 101: goto st406;
	}
	goto tr27;
st406:
	if ( ++p == pe )
		goto _test_eof406;
case 406:
	switch( (*p) ) {
		case 82: goto st407;
		case 114: goto st407;
	}
	goto tr27;
st407:
	if ( ++p == pe )
		goto _test_eof407;
case 407:
	if ( (*p) == 58 )
		goto st408;
	goto tr27;
st408:
	if ( ++p == pe )
		goto _test_eof408;
case 408:
	switch( (*p) ) {
		case 9: goto st408;
		case 13: goto st409;
		case 32: goto st408;
		case 44: goto st322;
		case 45: goto st412;
		case 67: goto st413;
		case 99: goto st413;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st412;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st412;
	} else
		goto st412;
	goto tr27;
st409:
	if ( ++p == pe )
		goto _test_eof409;
case 409:
	if ( (*p) == 10 )
		goto tr445;
	goto tr27;
tr445:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st410;
st410:
	if ( ++p == pe )
		goto _test_eof410;
case 410:
#line 5146 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st411;
		case 32: goto st411;
	}
	goto tr27;
st411:
	if ( ++p == pe )
		goto _test_eof411;
case 411:
	switch( (*p) ) {
		case 9: goto st411;
		case 32: goto st411;
		case 44: goto st322;
		case 45: goto st412;
		case 67: goto st413;
		case 99: goto st413;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st412;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st412;
	} else
		goto st412;
	goto tr27;
tr456:
#line 90 "http_parser.rl"
	{ http->has_trailer_md5 = 1; }
	goto st412;
st412:
	if ( ++p == pe )
		goto _test_eof412;
case 412:
#line 5181 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st322;
		case 13: goto st323;
		case 32: goto st322;
		case 45: goto st412;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st412;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st412;
	} else
		goto st412;
	goto tr27;
st413:
	if ( ++p == pe )
		goto _test_eof413;
case 413:
	switch( (*p) ) {
		case 9: goto st322;
		case 13: goto st323;
		case 32: goto st322;
		case 45: goto st412;
		case 79: goto st414;
		case 111: goto st414;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st412;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st412;
	} else
		goto st412;
	goto tr27;
st414:
	if ( ++p == pe )
		goto _test_eof414;
case 414:
	switch( (*p) ) {
		case 9: goto st322;
		case 13: goto st323;
		case 32: goto st322;
		case 45: goto st412;
		case 78: goto st415;
		case 110: goto st415;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st412;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st412;
	} else
		goto st412;
	goto tr27;
st415:
	if ( ++p == pe )
		goto _test_eof415;
case 415:
	switch( (*p) ) {
		case 9: goto st322;
		case 13: goto st323;
		case 32: goto st322;
		case 45: goto st412;
		case 84: goto st416;
		case 116: goto st416;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st412;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st412;
	} else
		goto st412;
	goto tr27;
st416:
	if ( ++p == pe )
		goto _test_eof416;
case 416:
	switch( (*p) ) {
		case 9: goto st322;
		case 13: goto st323;
		case 32: goto st322;
		case 45: goto st412;
		case 69: goto st417;
		case 101: goto st417;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st412;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st412;
	} else
		goto st412;
	goto tr27;
st417:
	if ( ++p == pe )
		goto _test_eof417;
case 417:
	switch( (*p) ) {
		case 9: goto st322;
		case 13: goto st323;
		case 32: goto st322;
		case 45: goto st412;
		case 78: goto st418;
		case 110: goto st418;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st412;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st412;
	} else
		goto st412;
	goto tr27;
st418:
	if ( ++p == pe )
		goto _test_eof418;
case 418:
	switch( (*p) ) {
		case 9: goto st322;
		case 13: goto st323;
		case 32: goto st322;
		case 45: goto st412;
		case 84: goto st419;
		case 116: goto st419;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st412;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st412;
	} else
		goto st412;
	goto tr27;
st419:
	if ( ++p == pe )
		goto _test_eof419;
case 419:
	switch( (*p) ) {
		case 9: goto st322;
		case 13: goto st323;
		case 32: goto st322;
		case 45: goto st420;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st412;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st412;
	} else
		goto st412;
	goto tr27;
st420:
	if ( ++p == pe )
		goto _test_eof420;
case 420:
	switch( (*p) ) {
		case 9: goto st322;
		case 13: goto st323;
		case 32: goto st322;
		case 45: goto st412;
		case 77: goto st421;
		case 109: goto st421;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st412;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st412;
	} else
		goto st412;
	goto tr27;
st421:
	if ( ++p == pe )
		goto _test_eof421;
case 421:
	switch( (*p) ) {
		case 9: goto st322;
		case 13: goto st323;
		case 32: goto st322;
		case 45: goto st412;
		case 68: goto st422;
		case 100: goto st422;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st412;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st412;
	} else
		goto st412;
	goto tr27;
st422:
	if ( ++p == pe )
		goto _test_eof422;
case 422:
	switch( (*p) ) {
		case 9: goto st322;
		case 13: goto st323;
		case 32: goto st322;
		case 45: goto st412;
		case 53: goto tr456;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st412;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st412;
	} else
		goto st412;
	goto tr27;
st423:
	if ( ++p == pe )
		goto _test_eof423;
case 423:
	switch( (*p) ) {
		case 83: goto st424;
		case 115: goto st424;
	}
	goto tr27;
st424:
	if ( ++p == pe )
		goto _test_eof424;
case 424:
	switch( (*p) ) {
		case 70: goto st425;
		case 102: goto st425;
	}
	goto tr27;
st425:
	if ( ++p == pe )
		goto _test_eof425;
case 425:
	switch( (*p) ) {
		case 69: goto st426;
		case 101: goto st426;
	}
	goto tr27;
st426:
	if ( ++p == pe )
		goto _test_eof426;
case 426:
	switch( (*p) ) {
		case 82: goto st427;
		case 114: goto st427;
	}
	goto tr27;
st427:
	if ( ++p == pe )
		goto _test_eof427;
case 427:
	if ( (*p) == 45 )
		goto st428;
	goto tr27;
st428:
	if ( ++p == pe )
		goto _test_eof428;
case 428:
	switch( (*p) ) {
		case 69: goto st429;
		case 101: goto st429;
	}
	goto tr27;
st429:
	if ( ++p == pe )
		goto _test_eof429;
case 429:
	switch( (*p) ) {
		case 78: goto st430;
		case 110: goto st430;
	}
	goto tr27;
st430:
	if ( ++p == pe )
		goto _test_eof430;
case 430:
	switch( (*p) ) {
		case 67: goto st431;
		case 99: goto st431;
	}
	goto tr27;
st431:
	if ( ++p == pe )
		goto _test_eof431;
case 431:
	switch( (*p) ) {
		case 79: goto st432;
		case 111: goto st432;
	}
	goto tr27;
st432:
	if ( ++p == pe )
		goto _test_eof432;
case 432:
	switch( (*p) ) {
		case 68: goto st433;
		case 100: goto st433;
	}
	goto tr27;
st433:
	if ( ++p == pe )
		goto _test_eof433;
case 433:
	switch( (*p) ) {
		case 73: goto st434;
		case 105: goto st434;
	}
	goto tr27;
st434:
	if ( ++p == pe )
		goto _test_eof434;
case 434:
	switch( (*p) ) {
		case 78: goto st435;
		case 110: goto st435;
	}
	goto tr27;
st435:
	if ( ++p == pe )
		goto _test_eof435;
case 435:
	switch( (*p) ) {
		case 71: goto st436;
		case 103: goto st436;
	}
	goto tr27;
st436:
	if ( ++p == pe )
		goto _test_eof436;
case 436:
	if ( (*p) == 58 )
		goto st437;
	goto tr27;
st437:
	if ( ++p == pe )
		goto _test_eof437;
case 437:
	switch( (*p) ) {
		case 9: goto st437;
		case 13: goto st438;
		case 32: goto st437;
		case 67: goto st441;
		case 99: goto st441;
	}
	goto tr27;
st438:
	if ( ++p == pe )
		goto _test_eof438;
case 438:
	if ( (*p) == 10 )
		goto tr473;
	goto tr27;
tr473:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st439;
st439:
	if ( ++p == pe )
		goto _test_eof439;
case 439:
#line 5553 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st440;
		case 32: goto st440;
	}
	goto tr27;
st440:
	if ( ++p == pe )
		goto _test_eof440;
case 440:
	switch( (*p) ) {
		case 9: goto st440;
		case 32: goto st440;
		case 67: goto st441;
		case 99: goto st441;
	}
	goto tr27;
st441:
	if ( ++p == pe )
		goto _test_eof441;
case 441:
	switch( (*p) ) {
		case 72: goto st442;
		case 104: goto st442;
	}
	goto tr27;
st442:
	if ( ++p == pe )
		goto _test_eof442;
case 442:
	switch( (*p) ) {
		case 85: goto st443;
		case 117: goto st443;
	}
	goto tr27;
st443:
	if ( ++p == pe )
		goto _test_eof443;
case 443:
	switch( (*p) ) {
		case 78: goto st444;
		case 110: goto st444;
	}
	goto tr27;
st444:
	if ( ++p == pe )
		goto _test_eof444;
case 444:
	switch( (*p) ) {
		case 75: goto st445;
		case 107: goto st445;
	}
	goto tr27;
st445:
	if ( ++p == pe )
		goto _test_eof445;
case 445:
	switch( (*p) ) {
		case 69: goto st446;
		case 101: goto st446;
	}
	goto tr27;
st446:
	if ( ++p == pe )
		goto _test_eof446;
case 446:
	switch( (*p) ) {
		case 68: goto st447;
		case 100: goto st447;
	}
	goto tr27;
st447:
	if ( ++p == pe )
		goto _test_eof447;
case 447:
	switch( (*p) ) {
		case 9: goto tr481;
		case 13: goto tr482;
		case 32: goto tr481;
	}
	goto tr27;
st448:
	if ( ++p == pe )
		goto _test_eof448;
case 448:
	switch( (*p) ) {
		case 65: goto st449;
		case 97: goto st449;
	}
	goto tr27;
st449:
	if ( ++p == pe )
		goto _test_eof449;
case 449:
	switch( (*p) ) {
		case 78: goto st450;
		case 110: goto st450;
	}
	goto tr27;
st450:
	if ( ++p == pe )
		goto _test_eof450;
case 450:
	switch( (*p) ) {
		case 71: goto st451;
		case 103: goto st451;
	}
	goto tr27;
st451:
	if ( ++p == pe )
		goto _test_eof451;
case 451:
	switch( (*p) ) {
		case 69: goto st452;
		case 101: goto st452;
	}
	goto tr27;
st452:
	if ( ++p == pe )
		goto _test_eof452;
case 452:
	if ( (*p) == 58 )
		goto st453;
	goto tr27;
st453:
	if ( ++p == pe )
		goto _test_eof453;
case 453:
	switch( (*p) ) {
		case 9: goto st453;
		case 13: goto st454;
		case 32: goto st453;
		case 98: goto st457;
	}
	goto tr27;
st454:
	if ( ++p == pe )
		goto _test_eof454;
case 454:
	if ( (*p) == 10 )
		goto tr490;
	goto tr27;
tr490:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st455;
st455:
	if ( ++p == pe )
		goto _test_eof455;
case 455:
#line 5703 "http_parser.c"
	switch( (*p) ) {
		case 9: goto st456;
		case 32: goto st456;
	}
	goto tr27;
st456:
	if ( ++p == pe )
		goto _test_eof456;
case 456:
	switch( (*p) ) {
		case 9: goto st456;
		case 32: goto st456;
		case 98: goto st457;
	}
	goto tr27;
st457:
	if ( ++p == pe )
		goto _test_eof457;
case 457:
	if ( (*p) == 121 )
		goto st458;
	goto tr27;
st458:
	if ( ++p == pe )
		goto _test_eof458;
case 458:
	if ( (*p) == 116 )
		goto st459;
	goto tr27;
st459:
	if ( ++p == pe )
		goto _test_eof459;
case 459:
	if ( (*p) == 101 )
		goto st460;
	goto tr27;
st460:
	if ( ++p == pe )
		goto _test_eof460;
case 460:
	if ( (*p) == 115 )
		goto st461;
	goto tr27;
st461:
	if ( ++p == pe )
		goto _test_eof461;
case 461:
	switch( (*p) ) {
		case 9: goto st462;
		case 32: goto st462;
	}
	goto tr27;
st462:
	if ( ++p == pe )
		goto _test_eof462;
case 462:
	switch( (*p) ) {
		case 9: goto st462;
		case 32: goto st462;
	}
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr497;
	goto tr202;
tr497:
#line 55 "http_parser.rl"
	{
			if (!length_incr(&http->range_beg, (*p)))
				{p++; cs = 463; goto _out;}
		}
	goto st463;
st463:
	if ( ++p == pe )
		goto _test_eof463;
case 463:
#line 5778 "http_parser.c"
	if ( (*p) == 45 )
		goto st464;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr497;
	goto tr202;
st464:
	if ( ++p == pe )
		goto _test_eof464;
case 464:
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr499;
	goto tr205;
tr499:
#line 61 "http_parser.rl"
	{
			if (!length_incr(&http->range_end, (*p)))
				{p++; cs = 465; goto _out;}
		}
	goto st465;
st465:
	if ( ++p == pe )
		goto _test_eof465;
case 465:
#line 5802 "http_parser.c"
	if ( (*p) == 47 )
		goto st466;
	if ( 48 <= (*p) && (*p) <= 57 )
		goto tr499;
	goto tr205;
st466:
	if ( ++p == pe )
		goto _test_eof466;
case 466:
	if ( (*p) == 42 )
		goto st467;
	goto tr27;
st467:
	if ( ++p == pe )
		goto _test_eof467;
case 467:
	switch( (*p) ) {
		case 9: goto tr502;
		case 13: goto tr503;
		case 32: goto tr502;
	}
	goto tr27;
	}
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
	_test_eof13: cs = 13; goto _test_eof; 
	_test_eof14: cs = 14; goto _test_eof; 
	_test_eof15: cs = 15; goto _test_eof; 
	_test_eof16: cs = 16; goto _test_eof; 
	_test_eof17: cs = 17; goto _test_eof; 
	_test_eof18: cs = 18; goto _test_eof; 
	_test_eof19: cs = 19; goto _test_eof; 
	_test_eof20: cs = 20; goto _test_eof; 
	_test_eof21: cs = 21; goto _test_eof; 
	_test_eof468: cs = 468; goto _test_eof; 
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
	_test_eof124: cs = 124; goto _test_eof; 
	_test_eof125: cs = 125; goto _test_eof; 
	_test_eof126: cs = 126; goto _test_eof; 
	_test_eof127: cs = 127; goto _test_eof; 
	_test_eof128: cs = 128; goto _test_eof; 
	_test_eof129: cs = 129; goto _test_eof; 
	_test_eof130: cs = 130; goto _test_eof; 
	_test_eof131: cs = 131; goto _test_eof; 
	_test_eof132: cs = 132; goto _test_eof; 
	_test_eof133: cs = 133; goto _test_eof; 
	_test_eof134: cs = 134; goto _test_eof; 
	_test_eof135: cs = 135; goto _test_eof; 
	_test_eof136: cs = 136; goto _test_eof; 
	_test_eof137: cs = 137; goto _test_eof; 
	_test_eof138: cs = 138; goto _test_eof; 
	_test_eof139: cs = 139; goto _test_eof; 
	_test_eof140: cs = 140; goto _test_eof; 
	_test_eof141: cs = 141; goto _test_eof; 
	_test_eof142: cs = 142; goto _test_eof; 
	_test_eof143: cs = 143; goto _test_eof; 
	_test_eof144: cs = 144; goto _test_eof; 
	_test_eof145: cs = 145; goto _test_eof; 
	_test_eof146: cs = 146; goto _test_eof; 
	_test_eof147: cs = 147; goto _test_eof; 
	_test_eof148: cs = 148; goto _test_eof; 
	_test_eof149: cs = 149; goto _test_eof; 
	_test_eof150: cs = 150; goto _test_eof; 
	_test_eof151: cs = 151; goto _test_eof; 
	_test_eof152: cs = 152; goto _test_eof; 
	_test_eof153: cs = 153; goto _test_eof; 
	_test_eof154: cs = 154; goto _test_eof; 
	_test_eof155: cs = 155; goto _test_eof; 
	_test_eof156: cs = 156; goto _test_eof; 
	_test_eof157: cs = 157; goto _test_eof; 
	_test_eof158: cs = 158; goto _test_eof; 
	_test_eof159: cs = 159; goto _test_eof; 
	_test_eof160: cs = 160; goto _test_eof; 
	_test_eof161: cs = 161; goto _test_eof; 
	_test_eof162: cs = 162; goto _test_eof; 
	_test_eof163: cs = 163; goto _test_eof; 
	_test_eof164: cs = 164; goto _test_eof; 
	_test_eof165: cs = 165; goto _test_eof; 
	_test_eof166: cs = 166; goto _test_eof; 
	_test_eof167: cs = 167; goto _test_eof; 
	_test_eof168: cs = 168; goto _test_eof; 
	_test_eof169: cs = 169; goto _test_eof; 
	_test_eof170: cs = 170; goto _test_eof; 
	_test_eof171: cs = 171; goto _test_eof; 
	_test_eof172: cs = 172; goto _test_eof; 
	_test_eof173: cs = 173; goto _test_eof; 
	_test_eof174: cs = 174; goto _test_eof; 
	_test_eof175: cs = 175; goto _test_eof; 
	_test_eof176: cs = 176; goto _test_eof; 
	_test_eof177: cs = 177; goto _test_eof; 
	_test_eof178: cs = 178; goto _test_eof; 
	_test_eof179: cs = 179; goto _test_eof; 
	_test_eof180: cs = 180; goto _test_eof; 
	_test_eof181: cs = 181; goto _test_eof; 
	_test_eof182: cs = 182; goto _test_eof; 
	_test_eof183: cs = 183; goto _test_eof; 
	_test_eof184: cs = 184; goto _test_eof; 
	_test_eof185: cs = 185; goto _test_eof; 
	_test_eof186: cs = 186; goto _test_eof; 
	_test_eof187: cs = 187; goto _test_eof; 
	_test_eof188: cs = 188; goto _test_eof; 
	_test_eof189: cs = 189; goto _test_eof; 
	_test_eof190: cs = 190; goto _test_eof; 
	_test_eof191: cs = 191; goto _test_eof; 
	_test_eof192: cs = 192; goto _test_eof; 
	_test_eof193: cs = 193; goto _test_eof; 
	_test_eof194: cs = 194; goto _test_eof; 
	_test_eof195: cs = 195; goto _test_eof; 
	_test_eof196: cs = 196; goto _test_eof; 
	_test_eof197: cs = 197; goto _test_eof; 
	_test_eof198: cs = 198; goto _test_eof; 
	_test_eof199: cs = 199; goto _test_eof; 
	_test_eof200: cs = 200; goto _test_eof; 
	_test_eof201: cs = 201; goto _test_eof; 
	_test_eof202: cs = 202; goto _test_eof; 
	_test_eof203: cs = 203; goto _test_eof; 
	_test_eof204: cs = 204; goto _test_eof; 
	_test_eof205: cs = 205; goto _test_eof; 
	_test_eof206: cs = 206; goto _test_eof; 
	_test_eof207: cs = 207; goto _test_eof; 
	_test_eof208: cs = 208; goto _test_eof; 
	_test_eof209: cs = 209; goto _test_eof; 
	_test_eof210: cs = 210; goto _test_eof; 
	_test_eof211: cs = 211; goto _test_eof; 
	_test_eof212: cs = 212; goto _test_eof; 
	_test_eof213: cs = 213; goto _test_eof; 
	_test_eof214: cs = 214; goto _test_eof; 
	_test_eof215: cs = 215; goto _test_eof; 
	_test_eof216: cs = 216; goto _test_eof; 
	_test_eof217: cs = 217; goto _test_eof; 
	_test_eof218: cs = 218; goto _test_eof; 
	_test_eof219: cs = 219; goto _test_eof; 
	_test_eof220: cs = 220; goto _test_eof; 
	_test_eof221: cs = 221; goto _test_eof; 
	_test_eof222: cs = 222; goto _test_eof; 
	_test_eof223: cs = 223; goto _test_eof; 
	_test_eof224: cs = 224; goto _test_eof; 
	_test_eof225: cs = 225; goto _test_eof; 
	_test_eof226: cs = 226; goto _test_eof; 
	_test_eof227: cs = 227; goto _test_eof; 
	_test_eof228: cs = 228; goto _test_eof; 
	_test_eof229: cs = 229; goto _test_eof; 
	_test_eof230: cs = 230; goto _test_eof; 
	_test_eof231: cs = 231; goto _test_eof; 
	_test_eof232: cs = 232; goto _test_eof; 
	_test_eof233: cs = 233; goto _test_eof; 
	_test_eof234: cs = 234; goto _test_eof; 
	_test_eof235: cs = 235; goto _test_eof; 
	_test_eof236: cs = 236; goto _test_eof; 
	_test_eof237: cs = 237; goto _test_eof; 
	_test_eof238: cs = 238; goto _test_eof; 
	_test_eof239: cs = 239; goto _test_eof; 
	_test_eof240: cs = 240; goto _test_eof; 
	_test_eof241: cs = 241; goto _test_eof; 
	_test_eof242: cs = 242; goto _test_eof; 
	_test_eof243: cs = 243; goto _test_eof; 
	_test_eof244: cs = 244; goto _test_eof; 
	_test_eof245: cs = 245; goto _test_eof; 
	_test_eof469: cs = 469; goto _test_eof; 
	_test_eof246: cs = 246; goto _test_eof; 
	_test_eof247: cs = 247; goto _test_eof; 
	_test_eof248: cs = 248; goto _test_eof; 
	_test_eof249: cs = 249; goto _test_eof; 
	_test_eof250: cs = 250; goto _test_eof; 
	_test_eof470: cs = 470; goto _test_eof; 
	_test_eof251: cs = 251; goto _test_eof; 
	_test_eof252: cs = 252; goto _test_eof; 
	_test_eof253: cs = 253; goto _test_eof; 
	_test_eof254: cs = 254; goto _test_eof; 
	_test_eof255: cs = 255; goto _test_eof; 
	_test_eof256: cs = 256; goto _test_eof; 
	_test_eof257: cs = 257; goto _test_eof; 
	_test_eof258: cs = 258; goto _test_eof; 
	_test_eof259: cs = 259; goto _test_eof; 
	_test_eof260: cs = 260; goto _test_eof; 
	_test_eof261: cs = 261; goto _test_eof; 
	_test_eof262: cs = 262; goto _test_eof; 
	_test_eof263: cs = 263; goto _test_eof; 
	_test_eof264: cs = 264; goto _test_eof; 
	_test_eof265: cs = 265; goto _test_eof; 
	_test_eof266: cs = 266; goto _test_eof; 
	_test_eof267: cs = 267; goto _test_eof; 
	_test_eof268: cs = 268; goto _test_eof; 
	_test_eof269: cs = 269; goto _test_eof; 
	_test_eof270: cs = 270; goto _test_eof; 
	_test_eof271: cs = 271; goto _test_eof; 
	_test_eof272: cs = 272; goto _test_eof; 
	_test_eof273: cs = 273; goto _test_eof; 
	_test_eof274: cs = 274; goto _test_eof; 
	_test_eof275: cs = 275; goto _test_eof; 
	_test_eof276: cs = 276; goto _test_eof; 
	_test_eof277: cs = 277; goto _test_eof; 
	_test_eof278: cs = 278; goto _test_eof; 
	_test_eof279: cs = 279; goto _test_eof; 
	_test_eof280: cs = 280; goto _test_eof; 
	_test_eof281: cs = 281; goto _test_eof; 
	_test_eof282: cs = 282; goto _test_eof; 
	_test_eof283: cs = 283; goto _test_eof; 
	_test_eof284: cs = 284; goto _test_eof; 
	_test_eof285: cs = 285; goto _test_eof; 
	_test_eof286: cs = 286; goto _test_eof; 
	_test_eof287: cs = 287; goto _test_eof; 
	_test_eof288: cs = 288; goto _test_eof; 
	_test_eof289: cs = 289; goto _test_eof; 
	_test_eof290: cs = 290; goto _test_eof; 
	_test_eof291: cs = 291; goto _test_eof; 
	_test_eof292: cs = 292; goto _test_eof; 
	_test_eof293: cs = 293; goto _test_eof; 
	_test_eof294: cs = 294; goto _test_eof; 
	_test_eof295: cs = 295; goto _test_eof; 
	_test_eof296: cs = 296; goto _test_eof; 
	_test_eof297: cs = 297; goto _test_eof; 
	_test_eof298: cs = 298; goto _test_eof; 
	_test_eof471: cs = 471; goto _test_eof; 
	_test_eof299: cs = 299; goto _test_eof; 
	_test_eof300: cs = 300; goto _test_eof; 
	_test_eof301: cs = 301; goto _test_eof; 
	_test_eof302: cs = 302; goto _test_eof; 
	_test_eof303: cs = 303; goto _test_eof; 
	_test_eof472: cs = 472; goto _test_eof; 
	_test_eof304: cs = 304; goto _test_eof; 
	_test_eof305: cs = 305; goto _test_eof; 
	_test_eof306: cs = 306; goto _test_eof; 
	_test_eof307: cs = 307; goto _test_eof; 
	_test_eof308: cs = 308; goto _test_eof; 
	_test_eof309: cs = 309; goto _test_eof; 
	_test_eof310: cs = 310; goto _test_eof; 
	_test_eof311: cs = 311; goto _test_eof; 
	_test_eof312: cs = 312; goto _test_eof; 
	_test_eof313: cs = 313; goto _test_eof; 
	_test_eof314: cs = 314; goto _test_eof; 
	_test_eof315: cs = 315; goto _test_eof; 
	_test_eof316: cs = 316; goto _test_eof; 
	_test_eof317: cs = 317; goto _test_eof; 
	_test_eof318: cs = 318; goto _test_eof; 
	_test_eof319: cs = 319; goto _test_eof; 
	_test_eof320: cs = 320; goto _test_eof; 
	_test_eof321: cs = 321; goto _test_eof; 
	_test_eof322: cs = 322; goto _test_eof; 
	_test_eof323: cs = 323; goto _test_eof; 
	_test_eof324: cs = 324; goto _test_eof; 
	_test_eof325: cs = 325; goto _test_eof; 
	_test_eof326: cs = 326; goto _test_eof; 
	_test_eof327: cs = 327; goto _test_eof; 
	_test_eof328: cs = 328; goto _test_eof; 
	_test_eof329: cs = 329; goto _test_eof; 
	_test_eof330: cs = 330; goto _test_eof; 
	_test_eof331: cs = 331; goto _test_eof; 
	_test_eof332: cs = 332; goto _test_eof; 
	_test_eof333: cs = 333; goto _test_eof; 
	_test_eof334: cs = 334; goto _test_eof; 
	_test_eof335: cs = 335; goto _test_eof; 
	_test_eof336: cs = 336; goto _test_eof; 
	_test_eof337: cs = 337; goto _test_eof; 
	_test_eof338: cs = 338; goto _test_eof; 
	_test_eof339: cs = 339; goto _test_eof; 
	_test_eof340: cs = 340; goto _test_eof; 
	_test_eof341: cs = 341; goto _test_eof; 
	_test_eof342: cs = 342; goto _test_eof; 
	_test_eof343: cs = 343; goto _test_eof; 
	_test_eof344: cs = 344; goto _test_eof; 
	_test_eof345: cs = 345; goto _test_eof; 
	_test_eof346: cs = 346; goto _test_eof; 
	_test_eof347: cs = 347; goto _test_eof; 
	_test_eof348: cs = 348; goto _test_eof; 
	_test_eof349: cs = 349; goto _test_eof; 
	_test_eof350: cs = 350; goto _test_eof; 
	_test_eof351: cs = 351; goto _test_eof; 
	_test_eof352: cs = 352; goto _test_eof; 
	_test_eof353: cs = 353; goto _test_eof; 
	_test_eof354: cs = 354; goto _test_eof; 
	_test_eof355: cs = 355; goto _test_eof; 
	_test_eof356: cs = 356; goto _test_eof; 
	_test_eof357: cs = 357; goto _test_eof; 
	_test_eof358: cs = 358; goto _test_eof; 
	_test_eof359: cs = 359; goto _test_eof; 
	_test_eof360: cs = 360; goto _test_eof; 
	_test_eof361: cs = 361; goto _test_eof; 
	_test_eof362: cs = 362; goto _test_eof; 
	_test_eof363: cs = 363; goto _test_eof; 
	_test_eof364: cs = 364; goto _test_eof; 
	_test_eof365: cs = 365; goto _test_eof; 
	_test_eof366: cs = 366; goto _test_eof; 
	_test_eof367: cs = 367; goto _test_eof; 
	_test_eof368: cs = 368; goto _test_eof; 
	_test_eof369: cs = 369; goto _test_eof; 
	_test_eof370: cs = 370; goto _test_eof; 
	_test_eof371: cs = 371; goto _test_eof; 
	_test_eof372: cs = 372; goto _test_eof; 
	_test_eof373: cs = 373; goto _test_eof; 
	_test_eof374: cs = 374; goto _test_eof; 
	_test_eof375: cs = 375; goto _test_eof; 
	_test_eof376: cs = 376; goto _test_eof; 
	_test_eof377: cs = 377; goto _test_eof; 
	_test_eof378: cs = 378; goto _test_eof; 
	_test_eof379: cs = 379; goto _test_eof; 
	_test_eof380: cs = 380; goto _test_eof; 
	_test_eof381: cs = 381; goto _test_eof; 
	_test_eof382: cs = 382; goto _test_eof; 
	_test_eof383: cs = 383; goto _test_eof; 
	_test_eof384: cs = 384; goto _test_eof; 
	_test_eof385: cs = 385; goto _test_eof; 
	_test_eof386: cs = 386; goto _test_eof; 
	_test_eof387: cs = 387; goto _test_eof; 
	_test_eof388: cs = 388; goto _test_eof; 
	_test_eof389: cs = 389; goto _test_eof; 
	_test_eof390: cs = 390; goto _test_eof; 
	_test_eof391: cs = 391; goto _test_eof; 
	_test_eof392: cs = 392; goto _test_eof; 
	_test_eof393: cs = 393; goto _test_eof; 
	_test_eof394: cs = 394; goto _test_eof; 
	_test_eof395: cs = 395; goto _test_eof; 
	_test_eof396: cs = 396; goto _test_eof; 
	_test_eof397: cs = 397; goto _test_eof; 
	_test_eof398: cs = 398; goto _test_eof; 
	_test_eof399: cs = 399; goto _test_eof; 
	_test_eof400: cs = 400; goto _test_eof; 
	_test_eof401: cs = 401; goto _test_eof; 
	_test_eof402: cs = 402; goto _test_eof; 
	_test_eof403: cs = 403; goto _test_eof; 
	_test_eof404: cs = 404; goto _test_eof; 
	_test_eof405: cs = 405; goto _test_eof; 
	_test_eof406: cs = 406; goto _test_eof; 
	_test_eof407: cs = 407; goto _test_eof; 
	_test_eof408: cs = 408; goto _test_eof; 
	_test_eof409: cs = 409; goto _test_eof; 
	_test_eof410: cs = 410; goto _test_eof; 
	_test_eof411: cs = 411; goto _test_eof; 
	_test_eof412: cs = 412; goto _test_eof; 
	_test_eof413: cs = 413; goto _test_eof; 
	_test_eof414: cs = 414; goto _test_eof; 
	_test_eof415: cs = 415; goto _test_eof; 
	_test_eof416: cs = 416; goto _test_eof; 
	_test_eof417: cs = 417; goto _test_eof; 
	_test_eof418: cs = 418; goto _test_eof; 
	_test_eof419: cs = 419; goto _test_eof; 
	_test_eof420: cs = 420; goto _test_eof; 
	_test_eof421: cs = 421; goto _test_eof; 
	_test_eof422: cs = 422; goto _test_eof; 
	_test_eof423: cs = 423; goto _test_eof; 
	_test_eof424: cs = 424; goto _test_eof; 
	_test_eof425: cs = 425; goto _test_eof; 
	_test_eof426: cs = 426; goto _test_eof; 
	_test_eof427: cs = 427; goto _test_eof; 
	_test_eof428: cs = 428; goto _test_eof; 
	_test_eof429: cs = 429; goto _test_eof; 
	_test_eof430: cs = 430; goto _test_eof; 
	_test_eof431: cs = 431; goto _test_eof; 
	_test_eof432: cs = 432; goto _test_eof; 
	_test_eof433: cs = 433; goto _test_eof; 
	_test_eof434: cs = 434; goto _test_eof; 
	_test_eof435: cs = 435; goto _test_eof; 
	_test_eof436: cs = 436; goto _test_eof; 
	_test_eof437: cs = 437; goto _test_eof; 
	_test_eof438: cs = 438; goto _test_eof; 
	_test_eof439: cs = 439; goto _test_eof; 
	_test_eof440: cs = 440; goto _test_eof; 
	_test_eof441: cs = 441; goto _test_eof; 
	_test_eof442: cs = 442; goto _test_eof; 
	_test_eof443: cs = 443; goto _test_eof; 
	_test_eof444: cs = 444; goto _test_eof; 
	_test_eof445: cs = 445; goto _test_eof; 
	_test_eof446: cs = 446; goto _test_eof; 
	_test_eof447: cs = 447; goto _test_eof; 
	_test_eof448: cs = 448; goto _test_eof; 
	_test_eof449: cs = 449; goto _test_eof; 
	_test_eof450: cs = 450; goto _test_eof; 
	_test_eof451: cs = 451; goto _test_eof; 
	_test_eof452: cs = 452; goto _test_eof; 
	_test_eof453: cs = 453; goto _test_eof; 
	_test_eof454: cs = 454; goto _test_eof; 
	_test_eof455: cs = 455; goto _test_eof; 
	_test_eof456: cs = 456; goto _test_eof; 
	_test_eof457: cs = 457; goto _test_eof; 
	_test_eof458: cs = 458; goto _test_eof; 
	_test_eof459: cs = 459; goto _test_eof; 
	_test_eof460: cs = 460; goto _test_eof; 
	_test_eof461: cs = 461; goto _test_eof; 
	_test_eof462: cs = 462; goto _test_eof; 
	_test_eof463: cs = 463; goto _test_eof; 
	_test_eof464: cs = 464; goto _test_eof; 
	_test_eof465: cs = 465; goto _test_eof; 
	_test_eof466: cs = 466; goto _test_eof; 
	_test_eof467: cs = 467; goto _test_eof; 

	_test_eof: {}
	if ( p == eof )
	{
	switch ( cs ) {
	case 249: 
	case 251: 
	case 252: 
	case 253: 
	case 254: 
	case 255: 
	case 256: 
	case 257: 
	case 258: 
	case 259: 
	case 260: 
	case 261: 
	case 263: 
	case 264: 
#line 40 "http_common.rl"
	{
			if (http->line_end > 0) {
				assert(buf[http->line_end] == '\n'
				       && "bad http->line_end");
				p = buf + http->line_end + 1;
			} else {
				p = buf;
			}
			assert(p <= pe && "overflow");
			{goto st240;}
		}
	break;
	case 20: 
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
	case 63: 
	case 64: 
	case 67: 
	case 68: 
	case 69: 
	case 71: 
	case 72: 
	case 101: 
	case 102: 
	case 103: 
	case 104: 
	case 105: 
	case 107: 
	case 108: 
	case 117: 
	case 118: 
	case 119: 
	case 120: 
	case 121: 
	case 122: 
	case 123: 
	case 124: 
	case 125: 
	case 126: 
	case 127: 
	case 128: 
	case 129: 
	case 130: 
	case 131: 
	case 132: 
	case 133: 
	case 134: 
	case 135: 
	case 136: 
	case 137: 
	case 138: 
	case 139: 
	case 140: 
	case 141: 
	case 142: 
	case 143: 
	case 144: 
	case 145: 
	case 146: 
	case 147: 
	case 148: 
	case 149: 
	case 150: 
	case 151: 
	case 152: 
	case 153: 
	case 154: 
	case 155: 
	case 156: 
	case 157: 
	case 158: 
	case 159: 
	case 160: 
	case 161: 
	case 162: 
	case 163: 
	case 164: 
	case 165: 
	case 166: 
	case 167: 
	case 168: 
	case 169: 
	case 170: 
	case 171: 
	case 172: 
	case 173: 
	case 174: 
	case 175: 
	case 176: 
	case 177: 
	case 178: 
	case 179: 
	case 184: 
	case 185: 
	case 302: 
	case 304: 
	case 305: 
	case 306: 
	case 307: 
	case 308: 
	case 309: 
	case 310: 
	case 311: 
	case 312: 
	case 313: 
	case 314: 
	case 315: 
	case 316: 
	case 317: 
	case 318: 
	case 319: 
	case 320: 
	case 321: 
	case 322: 
	case 323: 
	case 324: 
	case 325: 
	case 326: 
	case 327: 
	case 328: 
	case 329: 
	case 330: 
	case 331: 
	case 332: 
	case 333: 
	case 334: 
	case 335: 
	case 336: 
	case 337: 
	case 338: 
	case 339: 
	case 340: 
	case 341: 
	case 342: 
	case 343: 
	case 345: 
	case 346: 
	case 349: 
	case 350: 
	case 351: 
	case 353: 
	case 354: 
	case 383: 
	case 384: 
	case 385: 
	case 386: 
	case 387: 
	case 389: 
	case 390: 
	case 399: 
	case 400: 
	case 401: 
	case 402: 
	case 403: 
	case 404: 
	case 405: 
	case 406: 
	case 407: 
	case 408: 
	case 409: 
	case 410: 
	case 411: 
	case 412: 
	case 413: 
	case 414: 
	case 415: 
	case 416: 
	case 417: 
	case 418: 
	case 419: 
	case 420: 
	case 421: 
	case 422: 
	case 423: 
	case 424: 
	case 425: 
	case 426: 
	case 427: 
	case 428: 
	case 429: 
	case 430: 
	case 431: 
	case 432: 
	case 433: 
	case 434: 
	case 435: 
	case 436: 
	case 437: 
	case 438: 
	case 439: 
	case 440: 
	case 441: 
	case 442: 
	case 443: 
	case 444: 
	case 445: 
	case 446: 
	case 447: 
	case 448: 
	case 449: 
	case 450: 
	case 451: 
	case 452: 
	case 453: 
	case 454: 
	case 455: 
	case 456: 
	case 457: 
	case 458: 
	case 459: 
	case 460: 
	case 461: 
	case 466: 
	case 467: 
#line 104 "http_parser.rl"
	{
			assert(http->line_end > 0 &&
			       "no previous request/header line");
			assert(buf[http->line_end] == '\n' &&
			       "bad http->line_end");
			p = buf + http->line_end + 1;
			assert(p <= pe && "overflow");
			{goto st293;}
		}
	break;
	case 262: 
	case 265: 
	case 266: 
	case 267: 
	case 268: 
	case 269: 
	case 270: 
	case 271: 
	case 272: 
	case 273: 
	case 274: 
	case 275: 
	case 276: 
	case 277: 
	case 278: 
	case 279: 
	case 280: 
	case 281: 
	case 282: 
	case 283: 
	case 284: 
	case 285: 
	case 286: 
	case 287: 
	case 288: 
	case 289: 
	case 290: 
	case 291: 
	case 292: 
#line 30 "http_common.rl"
	{
				if (!http->has_expect_md5) {
					errno = EINVAL;
					{p++; cs = 0; goto _out;}
				}
			}
#line 40 "http_common.rl"
	{
			if (http->line_end > 0) {
				assert(buf[http->line_end] == '\n'
				       && "bad http->line_end");
				p = buf + http->line_end + 1;
			} else {
				p = buf;
			}
			assert(p <= pe && "overflow");
			{goto st240;}
		}
	break;
	case 70: 
	case 73: 
	case 74: 
	case 75: 
	case 76: 
	case 77: 
	case 78: 
	case 79: 
	case 80: 
	case 81: 
	case 82: 
	case 83: 
	case 84: 
	case 85: 
	case 86: 
	case 87: 
	case 88: 
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
	case 352: 
	case 355: 
	case 356: 
	case 357: 
	case 358: 
	case 359: 
	case 360: 
	case 361: 
	case 362: 
	case 363: 
	case 364: 
	case 365: 
	case 366: 
	case 367: 
	case 368: 
	case 369: 
	case 370: 
	case 371: 
	case 372: 
	case 373: 
	case 374: 
	case 375: 
	case 376: 
	case 377: 
	case 378: 
	case 379: 
	case 380: 
	case 381: 
	case 382: 
#line 30 "http_common.rl"
	{
				if (!http->has_expect_md5) {
					errno = EINVAL;
					{p++; cs = 0; goto _out;}
				}
			}
#line 104 "http_parser.rl"
	{
			assert(http->line_end > 0 &&
			       "no previous request/header line");
			assert(buf[http->line_end] == '\n' &&
			       "bad http->line_end");
			p = buf + http->line_end + 1;
			assert(p <= pe && "overflow");
			{goto st293;}
		}
	break;
	case 62: 
	case 65: 
	case 66: 
	case 344: 
	case 347: 
	case 348: 
#line 52 "http_parser.rl"
	{ errno = EINVAL; {p++; cs = 0; goto _out;} }
#line 104 "http_parser.rl"
	{
			assert(http->line_end > 0 &&
			       "no previous request/header line");
			assert(buf[http->line_end] == '\n' &&
			       "bad http->line_end");
			p = buf + http->line_end + 1;
			assert(p <= pe && "overflow");
			{goto st293;}
		}
	break;
	case 180: 
	case 181: 
	case 462: 
	case 463: 
#line 59 "http_parser.rl"
	{ errno = EINVAL; {p++; cs = 0; goto _out;} }
#line 104 "http_parser.rl"
	{
			assert(http->line_end > 0 &&
			       "no previous request/header line");
			assert(buf[http->line_end] == '\n' &&
			       "bad http->line_end");
			p = buf + http->line_end + 1;
			assert(p <= pe && "overflow");
			{goto st293;}
		}
	break;
	case 182: 
	case 183: 
	case 464: 
	case 465: 
#line 65 "http_parser.rl"
	{ errno = EINVAL; {p++; cs = 0; goto _out;} }
#line 104 "http_parser.rl"
	{
			assert(http->line_end > 0 &&
			       "no previous request/header line");
			assert(buf[http->line_end] == '\n' &&
			       "bad http->line_end");
			p = buf + http->line_end + 1;
			assert(p <= pe && "overflow");
			{goto st293;}
		}
	break;
	case 106: 
	case 109: 
	case 110: 
	case 111: 
	case 112: 
	case 113: 
	case 114: 
	case 115: 
	case 116: 
	case 388: 
	case 391: 
	case 392: 
	case 393: 
	case 394: 
	case 395: 
	case 396: 
	case 397: 
	case 398: 
#line 85 "http_parser.rl"
	{ errno = EINVAL; {p++; cs = 0; goto _out;} }
#line 104 "http_parser.rl"
	{
			assert(http->line_end > 0 &&
			       "no previous request/header line");
			assert(buf[http->line_end] == '\n' &&
			       "bad http->line_end");
			p = buf + http->line_end + 1;
			assert(p <= pe && "overflow");
			{goto st293;}
		}
	break;
#line 6789 "http_parser.c"
	}
	}

	_out: {}
	}

#line 159 "http_parser.rl"

	if (really_done)
		cs = http_parser_first_final;

	http->cs = cs;
	http->offset = p - buf;

	if (cs == http_parser_error || errno)
		return MOG_PARSER_ERROR;

	assert(p <= pe && "buffer overflow after http parse");
	assert(http->offset <= len && "offset longer than len");

	if (http->cs == http_parser_first_final) return MOG_PARSER_DONE;
	return MOG_PARSER_CONTINUE;
}
