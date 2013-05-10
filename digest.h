/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
enum mog_digest_next {
	MOG_DIGEST_CONTINUE = 0,
	MOG_DIGEST_EOF,
	MOG_DIGEST_ERROR
};

/* XXX gc_hash_handle is a typedef which hides a pointer, ugh... */
void mog_digest_init(struct mog_digest *, enum Gc_hash);
enum mog_digest_next mog_digest_read(struct mog_digest *, int fd);
void mog_digest_hex(struct mog_digest *, char *buf, size_t len);
void mog_digest_destroy(struct mog_digest *);
