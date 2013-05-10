/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#ifdef HAVE_FSTATAT
static inline int
mog_stat(struct mog_svc *svc, const char *path, struct stat *sb)
{
	return fstatat(svc->docroot_fd, path + 1, sb, 0);
}
#else  /* HAVE_FSTATAT */
int mog_stat(struct mog_svc *svc, const char *path, struct stat *sb);
#endif /* HAVE_FSTATAT */

#ifdef HAVE_UNLINKAT
static inline int mog_unlink(struct mog_svc *svc, const char *path)
{
	return unlinkat(svc->docroot_fd, path + 1, 0);
}
#else /* HAVE_UNLINKAT */
int mog_unlink(struct mog_svc *svc, const char *path);
#endif /* HAVE_UNLINKAT */

#ifdef HAVE_RENAMEAT
static inline
int mog_rename(struct mog_svc *svc, const char *old, const char *new)
{
	return renameat(svc->docroot_fd, old + 1, svc->docroot_fd, new + 1);
}
#else  /* !HAVE_RENAMEAT */
int mog_rename(struct mog_svc *svc, const char *old, const char *new);
#endif /* !HAVE_RENAMEAT */

#ifdef HAVE_MKDIRAT
static inline int mog_mkdir(struct mog_svc *svc, const char *path, mode_t mode)
{
	return mkdirat(svc->docroot_fd, path + 1, mode);
}
#else  /* !HAVE_MKDIRAT */
int mog_mkdir(struct mog_svc *svc, const char *path, mode_t mode);
#endif /* !HAVE_MKDIRAT */

int mog_open_put(struct mog_svc *svc, const char *path, int flags);
int mog_open_read(struct mog_svc *svc, const char *path);
#ifdef O_CLOEXEC
void mog_cloexec_works(void);
#endif
