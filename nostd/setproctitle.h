#ifndef BSD_STDLIB_SETPROCTITLE_H
#define BSD_STDLIB_SETPROCTITLE_H

#ifndef HAVE_SETPROCTITLE
#if defined __FreeBSD__ \
 || defined __NetBSD__  \
 || defined __OpenBSD__
#define HAVE_SETPROCTITLE 1
#else
#define HAVE_SETPROCTITLE 0
#endif
#endif


#if HAVE_SETPROCTITLE
static inline void spt_init(int argc, char *argv[], char *envp[]) {}
#else
#if __GNUC__

void setproctitle(const char *, ...) __attribute__((format (printf, 1, 2)));

#else

void setproctitle(const char *, ...);

#endif

/*
 * XXX hack for older gcov + gcc.  This isn't needed for newer gcov + gcc
 * in Debian testing/unstable, but gcov/gcc 4.4.5-8 on Debian squeeze
 * fails to pass argc/argv/envp to ((constructor)) functions when using
 * gcov.
 *
 * We'll drop this hack when support for Debian squeeze is terminated
 * (probably 2014-2015).
 */
void spt_init(int argc, char *argv[], char *envp[]);

#endif /* !HAVE_SETPROCTITLE */

#endif /* BSD_STDLIB_SETPROCTITLE_H */
