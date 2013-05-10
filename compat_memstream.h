/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#ifdef HAVE_OPEN_MEMSTREAM
# define my_memstream_close(fp,dst,bytes) fclose((fp))
# define my_memstream_errclose(fp) ((void)fclose((fp)))
#else
static FILE * my_open_memstream(char **ptr, size_t *sizeloc)
{
	FILE *fp;

	do {
		errno = 0;
		fp = tmpfile();
		if (fp != NULL)
			return fp;
	} while (errno == EINTR);

	return NULL;
}

#define open_memstream(ptr,sizeloc) my_open_memstream((ptr),(sizeloc))

/* EBADF is fatal in MT applications like ours */
static void my_memstream_errclose(FILE *fp)
{
	errno = 0;
	if (fclose(fp) != 0)
		assert(errno != EBADF
		       && "EBADF in stdio/fclose(memstream) replacement");
}

static int my_memstream_close(FILE *fp, char **dst, size_t *bytes)
{
	long pos;

	errno = 0;
	pos = ftell(fp);

	if (pos >= 0) {
		rewind(fp);
		*bytes = (size_t)pos;
		*dst = xmalloc(*bytes);
		if (fread(*dst, 1, *bytes, fp) == *bytes)
			goto out;
	} else {
		assert(errno != EBADF
		       && "EBADF in stdio/open_memstream replacement");
	}

	*bytes = 0;
	syslog(LOG_ERR, "stdio/open_memstream replacement failed: %m");

out:
	/*
	 * if ftell() fails, fclose() may fail due to an I/O error,
	 * too, but at least hope we release memory..
	 */
	my_memstream_errclose(fp);

	return 0;
}
#endif /* !HAVE_OPEN_MEMSTREAM */
