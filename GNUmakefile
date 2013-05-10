# This GNUmakefile is only required to build targets of interest
# to the maintainers, not users building from the source tarball

include test/ruby.mk
_check_slow_mk := $(shell test -s test/slow.mk || $(RM) test/slow.mk)
-include test/slow.mk
-include Makefile
test/slow.mk: $(RB_TESTS_SLOW) test/gen-slow.sh
	$(AM_V_GEN)test/gen-slow.sh $(RB_TESTS_SLOW) > $@.$$$$ && \
	mv $@.$$$$ $@
# manpage-hack.mk is intended for maintainers and _not_ distributed with
# the tarball.  This hack is to avoid recursively calling make inside
# the cmogstored.1 rule, as recursively building "cmogstored" breaks
# parallel builds.  We also don't want to rely on users having help2man
# installed, so we distribute the generated manpage from the tarball.
# manpage-hack.mk is only in the git repo: git://bogomips.org/cmogstored
-include build-aux/manpage-hack.mk
