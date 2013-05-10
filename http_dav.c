/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 *
 * TODO: ensure compliance with relevant RFCs.
 *       This is only enough to work with MogileFS.
 */
#include "cmogstored.h"
#include "http.h"

void mog_http_delete(struct mog_http *http, char *buf)
{
	int rc;
	char *path;

	if (mog_fd_of(http)->fd_type == MOG_FD_TYPE_HTTPGET) {
		mog_http_resp(http, "405 Method Not Allowed", true);
		return;
	}

	path = mog_http_path(http, buf);
	if (!path) goto forbidden; /* path traversal attack */
	assert(path[0] == '/' && "bad path");
	if (path[1] == '\0') goto forbidden;

	rc = mog_unlink(http->svc, path);
	if (rc == 0) {
		mog_http_resp(http, "204 No Content", true);
		return;
	}

	switch (errno) {
	case EPERM:
	case EISDIR:
	case EACCES:
forbidden:
		mog_http_resp(http, "403 Forbidden", true);
		return;
	case ENOENT:
		mog_http_resp(http, "404 Not Found", true);
		return;
	}
	PRESERVE_ERRNO(do {
		mog_http_resp(http, "500 Internal Server Error", true);
	} while(0));
}

void mog_http_mkcol(struct mog_http *http, char *buf)
{
	char *path;

	if (mog_fd_of(http)->fd_type == MOG_FD_TYPE_HTTPGET) {
		mog_http_resp(http, "405 Method Not Allowed", true);
		return;
	}
	path = mog_http_path(http, buf);

	/*
	 * Do not do anything on MKCOL and rely on PUT to create
	 * directories.  This stops MogileFS trackers from trying
	 * (expensive) MKCOL requests.  Perlbal/mogstored also does
	 * this.
	 */
	if (path)
		mog_http_resp(http, "400 Bad Request", true);
	else /* path traversal attack */
		mog_http_resp(http, "403 Forbidden", true);
}
