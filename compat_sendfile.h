#ifndef HAVE_SENDFILE
static ssize_t compat_sendfile(int sockfd, int filefd, off_t *off, size_t count)
{
	size_t max_pread;
	void *buf = mog_fsbuf_get(&max_pread);
	ssize_t r;
	ssize_t w;

	max_pread = MIN(max_pread, count);
	do {
		r = pread(filefd, buf, max_pread, *off);
	} while (r < 0 && errno == EINTR);

	if (r <= 0)
		return r;

	w = write(sockfd, buf, r);
	if (w > 0)
		*off += w;
	return w;
}
# define sendfile(out_fd, in_fd, offset, count) \
         compat_sendfile((out_fd),(in_fd),(offset),(count))
#endif
