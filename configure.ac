AC_INIT([cmogstored],
	m4_esyscmd([build-aux/git-version-gen .tarball-version]),
	[normalperson@yhbt.net])
AC_CONFIG_SRCDIR([cmogstored.c])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign silent-rules parallel-tests subdir-objects])
AM_SILENT_RULES([yes])
AC_PREREQ(2.59)

AC_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

dnl we use C99 struct initializers
AC_PROG_CC_C99

gl_EARLY
gl_INIT
gl_WARN_ADD([-Wall])
gl_WARN_ADD([-Wcast-qual])
gl_WARN_ADD([-Wstrict-prototypes])
gl_WARN_ADD([-Wredundant-decls])
gl_WARN_ADD([-Wshadow])
gl_WARN_ADD([-Werror=write-strings])
gl_WARN_ADD([-Werror=aggregate-return])
gl_WARN_ADD([-Werror=char-subscripts])
gl_WARN_ADD([-Werror=pointer-arith])
gl_WARN_ADD([-Werror-implicit-function-declaration])
gl_WARN_ADD([-Werror=declaration-after-statement]) dnl I hate C99 for this :P
gl_WARN_ADD([-ggdb3],[CFLAGS])
AX_PTHREAD(true)
AC_SYS_LARGEFILE
AC_C_FLEXIBLE_ARRAY_MEMBER

dnl gnulib *at functions aren't thread-safe, ask for the real thing
AC_CHECK_FUNCS([openat renameat mkdirat fstatat unlinkat])

AC_CHECK_FUNCS([epoll_wait epoll_pwait ppoll])

dnl libkqueue should work in the future
AC_CHECK_FUNCS([kqueue])

AC_CHECK_FUNCS([ioctl])
AC_CHECK_FUNCS([sendfile])
AC_CHECK_FUNCS([open_memstream])
AC_CHECK_FUNCS([posix_fadvise])

dnl gnulib doesn't actually define SOCK_NONBLOCK/SOCK_CLOEXEC, and
dnl even if it did, emulation wouldn't be thread-safe
AC_CHECK_FUNCS([accept4])

AC_SUBST(NOSTD_CFLAGS)

# This works for all platforms we care about:
# GNU/Linux, FreeBSD, Debian GNU/kFreeBSD
AC_CHECK_FUNCS([setproctitle], [NOSTD_CFLAGS=-DHAVE_SETPROCTITLE=1],
[
case $build_os in
*linux*) ;;
*) NOSTD_CFLAGS=-DSETPROCTITLE_NOOP;;
esac
])

AC_CONFIG_FILES([Makefile lib/Makefile])
AC_OUTPUT
