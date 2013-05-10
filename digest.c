/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
#include "cmogstored.h"
#include "digest.h"

__attribute__((constructor)) static void digest_init(void)
{
	CHECK(Gc_rc, GC_OK, gc_init());
	atexit(gc_done);
}

void mog_digest_init(struct mog_digest *digest, enum Gc_hash alg)
{
	digest->alg = alg;
	CHECK(Gc_rc, GC_OK, gc_hash_open(alg, 0, &digest->ctx));
}

enum mog_digest_next mog_digest_read(struct mog_digest *digest, int fd)
{
	size_t len;
	char *buf = mog_fsbuf_get(&len);
	size_t i = 1024;

	while (--i > 0) {
		ssize_t r = read(fd, buf, len);

		if (r > 0) { /* most likely */
			gc_hash_write(digest->ctx, r, buf);
		} else if (r == 0) {
			/* wait for user to call mog_digest_hex() */
			return MOG_DIGEST_EOF;
		} else {
			assert(r < 0 && errno && "buggy read(2)?");
			/* may happen on crazy FSes */
			if (errno == EINTR)
				continue;
			/* bail on EAGAIN, not possible on regular files */
			return MOG_DIGEST_ERROR;
		}
	}

	return MOG_DIGEST_CONTINUE;
}

void mog_digest_hex(struct mog_digest *digest, char *buf, size_t len)
{
	static const char hex[] = "0123456789abcdef";
	char *out = buf;
	size_t hashlen = gc_hash_digest_length(digest->alg);
	union { const char *s; const unsigned char *u; } result;

	result.s = gc_hash_read(digest->ctx);

	/* hashlen = 16 for MD5, 20 for SHA-1 */
	if (digest->alg == GC_MD5)
		assert(hashlen == 16 && "bad hashlen");
	assert(len >= (hashlen * 2) && "hex buffer too small");
	assert(hashlen != 0 && "bad hashlen");

	while (hashlen--) {
		*out++ = hex[*result.u >> 4];
		*out++ = hex[*result.u & 0x0f];
		result.u++;
	}
}

void mog_digest_destroy(struct mog_digest *digest)
{
	if (digest->ctx)
		gc_hash_close(digest->ctx);
}
