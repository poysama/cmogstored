#include "iov_str.h"
void mog_http_resp0(
	struct mog_http *, struct iovec *status, bool alive);

#define mog_http_resp(http,conststr,alive) do { \
	struct iovec statustmp; \
	IOV_STR(&statustmp, (conststr)); \
	mog_http_resp0((http), &statustmp, (alive)); \
} while (0)
