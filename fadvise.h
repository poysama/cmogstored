#if defined(HAVE_POSIX_FADVISE)
static inline void mog_fadv_sequential(int fd, off_t offset, off_t len)
{
	int rc = posix_fadvise(fd, offset, len, POSIX_FADV_SEQUENTIAL);

	if (rc == 0) return;

	assert(errno != EBADF && errno != ESPIPE &&
	       "BUG: posix_fadvise(POSIX_FADV_SEQUENTIAL) failed");
}

static inline void mog_fadv_noreuse(int fd, off_t offset, off_t len)
{
	int rc = posix_fadvise(fd, offset, len, POSIX_FADV_NOREUSE);

	if (rc == 0) return;

	assert(errno != EBADF && errno != ESPIPE &&
	       "BUG: posix_fadvise(POSIX_FADV_NOREUSE) failed");
}
#else
static inline void mog_fadv_sequential(int fd, off_t offset, off_t len)
{
}
static inline void mog_fadv_noreuse(int fd, off_t offset, off_t len)
{
}
#endif
