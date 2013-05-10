/*
 * Copyright (C) 2012-2013, Eric Wong <normalperson@yhbt.net>
 * License: GPLv3 or later (see COPYING for details)
 */
struct mog_queue;
struct mog_fd;
#define MOG_IOUTIL_LEN (sizeof("1666.00"))
struct mog_iostat {
	int cs;
	bool ready;
	uint8_t util_tip;
	uint8_t dev_tip;
	struct mog_queue *queue;
	char util[MOG_IOUTIL_LEN];
	char dev[72]; /* this is way larger than it needs to be... */
};

void mog_iostat_init(struct mog_iostat *);
enum mog_parser_state
mog_iostat_parse(struct mog_iostat *, char *buf, size_t len);
void mog_iostat_commit(void);
void mog_iostat_line_done(struct mog_iostat *);
void mog_iostat_queue_step(struct mog_fd *);
