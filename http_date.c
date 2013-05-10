/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

/* avoiding strftime() since it's locale-aware */

#define DATELEN (MOG_HTTPDATE_CAPA - 1)
static const char week[] = "Sun\0Mon\0Tue\0Wed\0Thu\0Fri\0Sat";
static const char months[] = "Jan\0Feb\0Mar\0Apr\0May\0Jun\0"
                             "Jul\0Aug\0Sep\0Oct\0Nov\0Dec";

__attribute__((constructor)) static void http_date_init(void)
{
	time_t now = time(NULL);
	struct tm tm;

	/*
	 * call gmtime_r once in the main loop to load, TZ info.
	 * Initial run uses a lot of stack on *BSDs
	 */
	gmtime_r(&now, &tm);
}

char *mog_http_date(char *dst, size_t len, const time_t *timep)
{
	int rc;
	struct tm tm;

	assert(len >= MOG_HTTPDATE_CAPA && "date length incorrect");
	gmtime_r(timep, &tm);

	/* snprintf eats stack :( */
	rc = snprintf(dst, len, "%s, %02d %s %4d %02d:%02d:%02d GMT",
			 week + (tm.tm_wday * 4),
			 tm.tm_mday,
			 months + (tm.tm_mon * 4),
			 tm.tm_year + 1900,
			 tm.tm_hour,
			 tm.tm_min,
			 tm.tm_sec);
	assert(rc == DATELEN && "bad sprintf return value");

	return dst + rc;
}

struct mog_now *mog_now(void)
{
	static __thread struct mog_now now;
	time_t tnow = time(NULL); /* not a syscall on modern 64-bit systems */

	if (now.ntime == tnow)
		return &now;

	now.ntime = tnow;
	mog_http_date(now.httpdate, sizeof(now.httpdate), &tnow);

	return &now;
}
