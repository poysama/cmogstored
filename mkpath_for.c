/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"

/*
 * "path" is the pathname for a file (not a directory)
 * returns -1 on error, sets errno to EACCES on invalid paths
 */
int mog_mkpath_for(struct mog_svc *svc, char *path)
{
	char *tip = path + 1;
	char *c = strchr(tip, '/');
	int rc = 0;
	int i;

	/* refuse to create files at the top-level */
	if (c == NULL) {
		errno = EACCES;
		return -1;
	}

	for (i = 0; c != NULL; i++) {
		if (i > 0) {
			*c = 0;
			rc = mog_mkdir(svc, path, svc->mkcol_perms);
			*c = '/';
		}

		if (rc != 0 && errno != EEXIST)
			return rc;

		tip = c + 1;
		c = strchr(tip, '/');
	}

	return 0;
}
