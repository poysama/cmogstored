/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
%%{
	machine http_common;

	LWS = (' ' | '\t');
	LF = '\n' > { http->line_end = to_u16(fpc - buf); };
	eor = LWS*'\r'LF;
	CTL = (cntrl | 127);
	header_name = [a-zA-Z0-9\-]+;
	header_value = (any -- (LWS|CTL))(any -- CTL)*;
	sep = (LWS*)|(eor LWS+);
	b64_val = ([a-zA-Z0-9/+]{22}) > { http->tmp_tip = to_u16(fpc - buf); }
	          "=="
		  eor > {
			uint16_t tmp_end = to_u16(fpc - buf);
			char *in = buf + http->tmp_tip;
			size_t inlen = tmp_end - http->tmp_tip;
			char *out = (char *)http->expect_md5;
			size_t outlen = sizeof(http->expect_md5);
			bool rc;

			rc = base64_decode_ctx(NULL, in, inlen, out, &outlen);
			assert(rc == true && outlen == 16
			       && "base64_decoder broke for HTTP");
			http->has_expect_md5 = 1;
		  };
	content_md5 = "Content-MD5:"i sep ( b64_val ) $!  {
				if (!http->has_expect_md5) {
					errno = EINVAL;
					fbreak;
				}
			};
	ignored_trailer := header_name ':' sep header_value eor @ {
		fgoto more_trailers;
	};
	trailer_line = ( content_md5 ) $!
		{
			if (http->line_end > 0) {
				assert(buf[http->line_end] == '\n'
				       && "bad http->line_end");
				p = buf + http->line_end + 1;
			} else {
				p = buf;
			}
			assert(p <= pe && "overflow");
			fgoto ignored_trailer;
		};
	trailers = trailer_line* '\r''\n' > {
		http->chunk_state = MOG_CHUNK_STATE_DONE;
		http->line_end = to_u16(fpc - buf);
		really_done = 1;
		fbreak;
	};
	more_trailers := trailers;
}%%
