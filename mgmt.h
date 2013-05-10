/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
struct mog_svc;
struct mog_wbuf;
struct mog_mgmt;

/* mgmt_parser.rl */
void mog_mgmt_init(struct mog_mgmt *, struct mog_svc *);
enum mog_parser_state mog_mgmt_parse(struct mog_mgmt *, char *buf, size_t len);
void mog_mgmt_reset_parser(struct mog_mgmt *);

/* mgmt_fn.c */
void mog_mgmt_fn_digest(struct mog_mgmt *, char *buf);
void mog_mgmt_fn_size(struct mog_mgmt *, char *buf);
void mog_mgmt_fn_blank(struct mog_mgmt *);
void mog_mgmt_fn_unknown(struct mog_mgmt *, char *buf);
void mog_mgmt_fn_watch_err(struct mog_mgmt *);
void mog_mgmt_fn_digest_emit(struct mog_mgmt *);
void mog_mgmt_fn_digest_err(struct mog_mgmt *);
void mog_mgmt_fn_aio_threads(struct mog_mgmt *, char *);
