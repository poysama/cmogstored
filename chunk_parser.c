
#line 1 "chunk_parser.rl"
/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "http_util.h"
static inline off_t hexchar2off(int xdigit)
{
	if (xdigit >= '0' && xdigit <= '9')
		return xdigit - '0';
	if (xdigit >= 'a' && xdigit <= 'f')
		return xdigit - 'a' + 10;
	if (xdigit >= 'A' && xdigit <= 'F')
		return xdigit - 'A' + 10;

	/* Ragel already does runtime range checking for us  */
	assert(0 && "invalid digit character");
	return (off_t)LLONG_MIN;
}


#line 94 "chunk_parser.rl"



#line 29 "chunk_parser.c"
static const int chunk_parser_start = 1;
static const int chunk_parser_first_final = 60;
static const int chunk_parser_error = 0;

static const int chunk_parser_en_ignored_trailer = 7;
static const int chunk_parser_en_more_trailers = 16;
static const int chunk_parser_en_main = 1;


#line 97 "chunk_parser.rl"

void mog_chunk_init(struct mog_http *http)
{
	int cs;

	
#line 46 "chunk_parser.c"
	{
	cs = chunk_parser_start;
	}

#line 103 "chunk_parser.rl"
	assert(http->chunked && "not chunked");
	http->cs = cs;
	http->line_end = 0;
	http->content_len = 0;
	http->offset = 0;
	http->chunk_state = MOG_CHUNK_STATE_SIZE;
}

enum mog_parser_state
mog_chunk_parse(struct mog_http *http, char *buf, size_t len)
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
	
#line 79 "chunk_parser.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
	if ( (*p) == 13 )
		goto st2;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr2;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr2;
	} else
		goto tr2;
	goto st0;
tr16:
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
			{goto st7;}
		}
	goto st0;
tr31:
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
			{goto st7;}
		}
	goto st0;
#line 135 "chunk_parser.c"
st0:
cs = 0;
	goto _out;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	if ( (*p) == 10 )
		goto st3;
	goto st0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr2;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr2;
	} else
		goto tr2;
	goto st0;
tr2:
#line 79 "chunk_parser.rl"
	{
			off_t prev = http->content_len;

			http->content_len *= 16;
			http->content_len += hexchar2off((*p));
			if (http->content_len < prev) {
				errno = ERANGE;
				http->content_len = -1;
				{p++; cs = 4; goto _out;}
			}
		}
	goto st4;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
#line 177 "chunk_parser.c"
	switch( (*p) ) {
		case 10: goto st0;
		case 13: goto st6;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr2;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr2;
	} else
		goto tr2;
	goto st5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	switch( (*p) ) {
		case 10: goto st0;
		case 13: goto st6;
	}
	goto st5;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	if ( (*p) == 10 )
		goto tr6;
	goto st0;
tr6:
#line 91 "chunk_parser.rl"
	{ http->chunk_state = MOG_CHUNK_STATE_DATA; }
	goto st60;
st60:
	if ( ++p == pe )
		goto _test_eof60;
case 60:
#line 215 "chunk_parser.c"
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr65;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr65;
	} else
		goto tr65;
	goto tr64;
tr64:
#line 25 "chunk_parser.rl"
	{
		off_t buf_remain;
		size_t wr_len;

		if (http->content_len == 0) { /* final chunk */
			http->chunk_state = MOG_CHUNK_STATE_TRAILER;
			p--;

			/* XXX this feels wrong ... */
			if (p >= buf) {
				assert((*p) == '\n' && "bad chunk end");
				http->line_end = to_u16(p - buf);
			}
			{goto st16;}
		}

		assert(http->content_len > 0 && "impossible content_len");

		buf_remain = len - (p - buf);
		if (buf_remain == 0)
			{p++; cs = 61; goto _out;}

		assert(buf_remain > 0 && "impossible buf_remain");
		wr_len = MIN((size_t)http->content_len, (size_t)buf_remain);
		assert(wr_len != 0 && "invalid wr_len");
		if (! mog_http_write_full(http->forward, p, wr_len))
			{p++; cs = 61; goto _out;}

		http->content_len -= wr_len;
		p += wr_len - 1;
		assert(p < pe && "buffer overrun");

		if (http->content_len > 0) {
			really_done = 1;
			/* let caller handle reading the rest of the body */
			{p++; cs = 61; goto _out;}
		}

		/* next chunk header */
		http->chunk_state = MOG_CHUNK_STATE_SIZE;
		if (wr_len == buf_remain) {
			if (http->content_len == 0)
				{goto st1;}
			really_done = 1;
			{p++; cs = 61; goto _out;}
		}

		/* more chunks in this buffer */
		assert(http->content_len == 0 &&
		       "bad content_len at chunk end");

		{goto st1;}
	}
	goto st61;
st61:
	if ( ++p == pe )
		goto _test_eof61;
case 61:
#line 285 "chunk_parser.c"
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr67;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr67;
	} else
		goto tr67;
	goto st61;
tr67:
#line 79 "chunk_parser.rl"
	{
			off_t prev = http->content_len;

			http->content_len *= 16;
			http->content_len += hexchar2off((*p));
			if (http->content_len < prev) {
				errno = ERANGE;
				http->content_len = -1;
				{p++; cs = 62; goto _out;}
			}
		}
	goto st62;
tr65:
#line 25 "chunk_parser.rl"
	{
		off_t buf_remain;
		size_t wr_len;

		if (http->content_len == 0) { /* final chunk */
			http->chunk_state = MOG_CHUNK_STATE_TRAILER;
			p--;

			/* XXX this feels wrong ... */
			if (p >= buf) {
				assert((*p) == '\n' && "bad chunk end");
				http->line_end = to_u16(p - buf);
			}
			{goto st16;}
		}

		assert(http->content_len > 0 && "impossible content_len");

		buf_remain = len - (p - buf);
		if (buf_remain == 0)
			{p++; cs = 62; goto _out;}

		assert(buf_remain > 0 && "impossible buf_remain");
		wr_len = MIN((size_t)http->content_len, (size_t)buf_remain);
		assert(wr_len != 0 && "invalid wr_len");
		if (! mog_http_write_full(http->forward, p, wr_len))
			{p++; cs = 62; goto _out;}

		http->content_len -= wr_len;
		p += wr_len - 1;
		assert(p < pe && "buffer overrun");

		if (http->content_len > 0) {
			really_done = 1;
			/* let caller handle reading the rest of the body */
			{p++; cs = 62; goto _out;}
		}

		/* next chunk header */
		http->chunk_state = MOG_CHUNK_STATE_SIZE;
		if (wr_len == buf_remain) {
			if (http->content_len == 0)
				{goto st1;}
			really_done = 1;
			{p++; cs = 62; goto _out;}
		}

		/* more chunks in this buffer */
		assert(http->content_len == 0 &&
		       "bad content_len at chunk end");

		{goto st1;}
	}
#line 79 "chunk_parser.rl"
	{
			off_t prev = http->content_len;

			http->content_len *= 16;
			http->content_len += hexchar2off((*p));
			if (http->content_len < prev) {
				errno = ERANGE;
				http->content_len = -1;
				{p++; cs = 62; goto _out;}
			}
		}
	goto st62;
tr71:
#line 79 "chunk_parser.rl"
	{
			off_t prev = http->content_len;

			http->content_len *= 16;
			http->content_len += hexchar2off((*p));
			if (http->content_len < prev) {
				errno = ERANGE;
				http->content_len = -1;
				{p++; cs = 62; goto _out;}
			}
		}
#line 25 "chunk_parser.rl"
	{
		off_t buf_remain;
		size_t wr_len;

		if (http->content_len == 0) { /* final chunk */
			http->chunk_state = MOG_CHUNK_STATE_TRAILER;
			p--;

			/* XXX this feels wrong ... */
			if (p >= buf) {
				assert((*p) == '\n' && "bad chunk end");
				http->line_end = to_u16(p - buf);
			}
			{goto st16;}
		}

		assert(http->content_len > 0 && "impossible content_len");

		buf_remain = len - (p - buf);
		if (buf_remain == 0)
			{p++; cs = 62; goto _out;}

		assert(buf_remain > 0 && "impossible buf_remain");
		wr_len = MIN((size_t)http->content_len, (size_t)buf_remain);
		assert(wr_len != 0 && "invalid wr_len");
		if (! mog_http_write_full(http->forward, p, wr_len))
			{p++; cs = 62; goto _out;}

		http->content_len -= wr_len;
		p += wr_len - 1;
		assert(p < pe && "buffer overrun");

		if (http->content_len > 0) {
			really_done = 1;
			/* let caller handle reading the rest of the body */
			{p++; cs = 62; goto _out;}
		}

		/* next chunk header */
		http->chunk_state = MOG_CHUNK_STATE_SIZE;
		if (wr_len == buf_remain) {
			if (http->content_len == 0)
				{goto st1;}
			really_done = 1;
			{p++; cs = 62; goto _out;}
		}

		/* more chunks in this buffer */
		assert(http->content_len == 0 &&
		       "bad content_len at chunk end");

		{goto st1;}
	}
	goto st62;
st62:
	if ( ++p == pe )
		goto _test_eof62;
case 62:
#line 449 "chunk_parser.c"
	switch( (*p) ) {
		case 10: goto st61;
		case 13: goto st63;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr67;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr67;
	} else
		goto tr67;
	goto st62;
st63:
	if ( ++p == pe )
		goto _test_eof63;
case 63:
	if ( (*p) == 10 )
		goto tr70;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr67;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr67;
	} else
		goto tr67;
	goto st61;
tr70:
#line 91 "chunk_parser.rl"
	{ http->chunk_state = MOG_CHUNK_STATE_DATA; }
	goto st64;
st64:
	if ( ++p == pe )
		goto _test_eof64;
case 64:
#line 486 "chunk_parser.c"
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto tr71;
	} else if ( (*p) > 70 ) {
		if ( 97 <= (*p) && (*p) <= 102 )
			goto tr71;
	} else
		goto tr71;
	goto tr64;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	if ( (*p) == 45 )
		goto st8;
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st8;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st8;
	} else
		goto st8;
	goto st0;
st8:
	if ( ++p == pe )
		goto _test_eof8;
case 8:
	switch( (*p) ) {
		case 45: goto st8;
		case 58: goto st9;
	}
	if ( (*p) < 65 ) {
		if ( 48 <= (*p) && (*p) <= 57 )
			goto st8;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st8;
	} else
		goto st8;
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	switch( (*p) ) {
		case 9: goto st9;
		case 13: goto st13;
		case 32: goto st9;
		case 127: goto st0;
	}
	if ( 0 <= (*p) && (*p) <= 31 )
		goto st0;
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
	switch( (*p) ) {
		case 9: goto st11;
		case 13: goto st12;
		case 127: goto st0;
	}
	if ( 0 <= (*p) && (*p) <= 31 )
		goto st0;
	goto st10;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	switch( (*p) ) {
		case 9: goto st11;
		case 13: goto st12;
		case 32: goto st11;
	}
	goto st0;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	if ( (*p) == 10 )
		goto tr13;
	goto st0;
tr13:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
#line 36 "http_common.rl"
	{
		{goto st16;}
	}
	goto st65;
st65:
	if ( ++p == pe )
		goto _test_eof65;
case 65:
#line 582 "chunk_parser.c"
	goto st0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	if ( (*p) == 10 )
		goto tr14;
	goto st0;
tr14:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st14;
st14:
	if ( ++p == pe )
		goto _test_eof14;
case 14:
#line 599 "chunk_parser.c"
	switch( (*p) ) {
		case 9: goto st15;
		case 32: goto st15;
	}
	goto st0;
st15:
	if ( ++p == pe )
		goto _test_eof15;
case 15:
	switch( (*p) ) {
		case 9: goto st15;
		case 32: goto st15;
		case 127: goto st0;
	}
	if ( 0 <= (*p) && (*p) <= 31 )
		goto st0;
	goto st10;
st16:
	if ( ++p == pe )
		goto _test_eof16;
case 16:
	switch( (*p) ) {
		case 13: goto st17;
		case 67: goto st18;
		case 99: goto st18;
	}
	goto tr16;
st17:
	if ( ++p == pe )
		goto _test_eof17;
case 17:
	if ( (*p) == 10 )
		goto tr19;
	goto st0;
tr19:
#line 51 "http_common.rl"
	{
		http->chunk_state = MOG_CHUNK_STATE_DONE;
		http->line_end = to_u16(p - buf);
		really_done = 1;
		{p++; cs = 66; goto _out;}
	}
	goto st66;
st66:
	if ( ++p == pe )
		goto _test_eof66;
case 66:
#line 647 "chunk_parser.c"
	goto st0;
st18:
	if ( ++p == pe )
		goto _test_eof18;
case 18:
	switch( (*p) ) {
		case 79: goto st19;
		case 111: goto st19;
	}
	goto tr16;
st19:
	if ( ++p == pe )
		goto _test_eof19;
case 19:
	switch( (*p) ) {
		case 78: goto st20;
		case 110: goto st20;
	}
	goto tr16;
st20:
	if ( ++p == pe )
		goto _test_eof20;
case 20:
	switch( (*p) ) {
		case 84: goto st21;
		case 116: goto st21;
	}
	goto tr16;
st21:
	if ( ++p == pe )
		goto _test_eof21;
case 21:
	switch( (*p) ) {
		case 69: goto st22;
		case 101: goto st22;
	}
	goto tr16;
st22:
	if ( ++p == pe )
		goto _test_eof22;
case 22:
	switch( (*p) ) {
		case 78: goto st23;
		case 110: goto st23;
	}
	goto tr16;
st23:
	if ( ++p == pe )
		goto _test_eof23;
case 23:
	switch( (*p) ) {
		case 84: goto st24;
		case 116: goto st24;
	}
	goto tr16;
st24:
	if ( ++p == pe )
		goto _test_eof24;
case 24:
	if ( (*p) == 45 )
		goto st25;
	goto tr16;
st25:
	if ( ++p == pe )
		goto _test_eof25;
case 25:
	switch( (*p) ) {
		case 77: goto st26;
		case 109: goto st26;
	}
	goto tr16;
st26:
	if ( ++p == pe )
		goto _test_eof26;
case 26:
	switch( (*p) ) {
		case 68: goto st27;
		case 100: goto st27;
	}
	goto tr16;
st27:
	if ( ++p == pe )
		goto _test_eof27;
case 27:
	if ( (*p) == 53 )
		goto st28;
	goto tr16;
st28:
	if ( ++p == pe )
		goto _test_eof28;
case 28:
	if ( (*p) == 58 )
		goto st29;
	goto tr16;
st29:
	if ( ++p == pe )
		goto _test_eof29;
case 29:
	switch( (*p) ) {
		case 9: goto st29;
		case 13: goto st30;
		case 32: goto st29;
		case 43: goto tr33;
	}
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto tr33;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr33;
	} else
		goto tr33;
	goto tr31;
st30:
	if ( ++p == pe )
		goto _test_eof30;
case 30:
	if ( (*p) == 10 )
		goto tr34;
	goto tr16;
tr34:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st31;
st31:
	if ( ++p == pe )
		goto _test_eof31;
case 31:
#line 776 "chunk_parser.c"
	switch( (*p) ) {
		case 9: goto st32;
		case 32: goto st32;
	}
	goto tr16;
st32:
	if ( ++p == pe )
		goto _test_eof32;
case 32:
	switch( (*p) ) {
		case 9: goto st32;
		case 32: goto st32;
		case 43: goto tr33;
	}
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto tr33;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto tr33;
	} else
		goto tr33;
	goto tr31;
tr33:
#line 15 "http_common.rl"
	{ http->tmp_tip = to_u16(p - buf); }
	goto st33;
st33:
	if ( ++p == pe )
		goto _test_eof33;
case 33:
#line 808 "chunk_parser.c"
	if ( (*p) == 43 )
		goto st34;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st34;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st34;
	} else
		goto st34;
	goto tr31;
st34:
	if ( ++p == pe )
		goto _test_eof34;
case 34:
	if ( (*p) == 43 )
		goto st35;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st35;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st35;
	} else
		goto st35;
	goto tr31;
st35:
	if ( ++p == pe )
		goto _test_eof35;
case 35:
	if ( (*p) == 43 )
		goto st36;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st36;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st36;
	} else
		goto st36;
	goto tr31;
st36:
	if ( ++p == pe )
		goto _test_eof36;
case 36:
	if ( (*p) == 43 )
		goto st37;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st37;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st37;
	} else
		goto st37;
	goto tr31;
st37:
	if ( ++p == pe )
		goto _test_eof37;
case 37:
	if ( (*p) == 43 )
		goto st38;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st38;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st38;
	} else
		goto st38;
	goto tr31;
st38:
	if ( ++p == pe )
		goto _test_eof38;
case 38:
	if ( (*p) == 43 )
		goto st39;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st39;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st39;
	} else
		goto st39;
	goto tr31;
st39:
	if ( ++p == pe )
		goto _test_eof39;
case 39:
	if ( (*p) == 43 )
		goto st40;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st40;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st40;
	} else
		goto st40;
	goto tr31;
st40:
	if ( ++p == pe )
		goto _test_eof40;
case 40:
	if ( (*p) == 43 )
		goto st41;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st41;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st41;
	} else
		goto st41;
	goto tr31;
st41:
	if ( ++p == pe )
		goto _test_eof41;
case 41:
	if ( (*p) == 43 )
		goto st42;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st42;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st42;
	} else
		goto st42;
	goto tr31;
st42:
	if ( ++p == pe )
		goto _test_eof42;
case 42:
	if ( (*p) == 43 )
		goto st43;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st43;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st43;
	} else
		goto st43;
	goto tr31;
st43:
	if ( ++p == pe )
		goto _test_eof43;
case 43:
	if ( (*p) == 43 )
		goto st44;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st44;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st44;
	} else
		goto st44;
	goto tr31;
st44:
	if ( ++p == pe )
		goto _test_eof44;
case 44:
	if ( (*p) == 43 )
		goto st45;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st45;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st45;
	} else
		goto st45;
	goto tr31;
st45:
	if ( ++p == pe )
		goto _test_eof45;
case 45:
	if ( (*p) == 43 )
		goto st46;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st46;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st46;
	} else
		goto st46;
	goto tr31;
st46:
	if ( ++p == pe )
		goto _test_eof46;
case 46:
	if ( (*p) == 43 )
		goto st47;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st47;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st47;
	} else
		goto st47;
	goto tr31;
st47:
	if ( ++p == pe )
		goto _test_eof47;
case 47:
	if ( (*p) == 43 )
		goto st48;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st48;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st48;
	} else
		goto st48;
	goto tr31;
st48:
	if ( ++p == pe )
		goto _test_eof48;
case 48:
	if ( (*p) == 43 )
		goto st49;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st49;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st49;
	} else
		goto st49;
	goto tr31;
st49:
	if ( ++p == pe )
		goto _test_eof49;
case 49:
	if ( (*p) == 43 )
		goto st50;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st50;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st50;
	} else
		goto st50;
	goto tr31;
st50:
	if ( ++p == pe )
		goto _test_eof50;
case 50:
	if ( (*p) == 43 )
		goto st51;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st51;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st51;
	} else
		goto st51;
	goto tr31;
st51:
	if ( ++p == pe )
		goto _test_eof51;
case 51:
	if ( (*p) == 43 )
		goto st52;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st52;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st52;
	} else
		goto st52;
	goto tr31;
st52:
	if ( ++p == pe )
		goto _test_eof52;
case 52:
	if ( (*p) == 43 )
		goto st53;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st53;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st53;
	} else
		goto st53;
	goto tr31;
st53:
	if ( ++p == pe )
		goto _test_eof53;
case 53:
	if ( (*p) == 43 )
		goto st54;
	if ( (*p) < 65 ) {
		if ( 47 <= (*p) && (*p) <= 57 )
			goto st54;
	} else if ( (*p) > 90 ) {
		if ( 97 <= (*p) && (*p) <= 122 )
			goto st54;
	} else
		goto st54;
	goto tr31;
st54:
	if ( ++p == pe )
		goto _test_eof54;
case 54:
	if ( (*p) == 61 )
		goto st55;
	goto tr31;
st55:
	if ( ++p == pe )
		goto _test_eof55;
case 55:
	if ( (*p) == 61 )
		goto st56;
	goto tr31;
st56:
	if ( ++p == pe )
		goto _test_eof56;
case 56:
	switch( (*p) ) {
		case 9: goto tr59;
		case 13: goto tr60;
		case 32: goto tr59;
	}
	goto tr31;
tr59:
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
	goto st57;
st57:
	if ( ++p == pe )
		goto _test_eof57;
case 57:
#line 1164 "chunk_parser.c"
	switch( (*p) ) {
		case 9: goto st57;
		case 13: goto st58;
		case 32: goto st57;
	}
	goto tr31;
tr60:
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
	goto st58;
st58:
	if ( ++p == pe )
		goto _test_eof58;
case 58:
#line 1191 "chunk_parser.c"
	if ( (*p) == 10 )
		goto tr63;
	goto tr31;
tr63:
#line 9 "http_common.rl"
	{ http->line_end = to_u16(p - buf); }
	goto st59;
st59:
	if ( ++p == pe )
		goto _test_eof59;
case 59:
#line 1203 "chunk_parser.c"
	switch( (*p) ) {
		case 13: goto st17;
		case 67: goto st18;
		case 99: goto st18;
	}
	goto tr31;
	}
	_test_eof1: cs = 1; goto _test_eof; 
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof60: cs = 60; goto _test_eof; 
	_test_eof61: cs = 61; goto _test_eof; 
	_test_eof62: cs = 62; goto _test_eof; 
	_test_eof63: cs = 63; goto _test_eof; 
	_test_eof64: cs = 64; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 
	_test_eof11: cs = 11; goto _test_eof; 
	_test_eof12: cs = 12; goto _test_eof; 
	_test_eof65: cs = 65; goto _test_eof; 
	_test_eof13: cs = 13; goto _test_eof; 
	_test_eof14: cs = 14; goto _test_eof; 
	_test_eof15: cs = 15; goto _test_eof; 
	_test_eof16: cs = 16; goto _test_eof; 
	_test_eof17: cs = 17; goto _test_eof; 
	_test_eof66: cs = 66; goto _test_eof; 
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

	_test_eof: {}
	if ( p == eof )
	{
	switch ( cs ) {
	case 16: 
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
	case 30: 
	case 31: 
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
			{goto st7;}
		}
	break;
	case 60: 
	case 64: 
#line 25 "chunk_parser.rl"
	{
		off_t buf_remain;
		size_t wr_len;

		if (http->content_len == 0) { /* final chunk */
			http->chunk_state = MOG_CHUNK_STATE_TRAILER;
			p--;

			/* XXX this feels wrong ... */
			if (p >= buf) {
				assert((*p) == '\n' && "bad chunk end");
				http->line_end = to_u16(p - buf);
			}
			{goto st16;}
		}

		assert(http->content_len > 0 && "impossible content_len");

		buf_remain = len - (p - buf);
		if (buf_remain == 0)
			{p++; cs = 0; goto _out;}

		assert(buf_remain > 0 && "impossible buf_remain");
		wr_len = MIN((size_t)http->content_len, (size_t)buf_remain);
		assert(wr_len != 0 && "invalid wr_len");
		if (! mog_http_write_full(http->forward, p, wr_len))
			{p++; cs = 0; goto _out;}

		http->content_len -= wr_len;
		p += wr_len - 1;
		assert(p < pe && "buffer overrun");

		if (http->content_len > 0) {
			really_done = 1;
			/* let caller handle reading the rest of the body */
			{p++; cs = 0; goto _out;}
		}

		/* next chunk header */
		http->chunk_state = MOG_CHUNK_STATE_SIZE;
		if (wr_len == buf_remain) {
			if (http->content_len == 0)
				{goto st1;}
			really_done = 1;
			{p++; cs = 0; goto _out;}
		}

		/* more chunks in this buffer */
		assert(http->content_len == 0 &&
		       "bad content_len at chunk end");

		{goto st1;}
	}
	break;
	case 29: 
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
			{goto st7;}
		}
	break;
#line 1415 "chunk_parser.c"
	}
	}

	_out: {}
	}

#line 130 "chunk_parser.rl"

	if (really_done)
		cs = chunk_parser_first_final;

	http->cs = cs;
	http->offset = p - buf;

	if (cs == chunk_parser_error || errno)
		return MOG_PARSER_ERROR;

	assert(p <= pe && "buffer overflow after chunk parse");
	assert(http->offset <= len && "offset longer than len");

	if (http->cs == chunk_parser_first_final) return MOG_PARSER_DONE;
	return MOG_PARSER_CONTINUE;
}
