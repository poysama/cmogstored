cov_cflags = $(CFLAGS) -O0 -ftest-coverage -fprofile-arcs
cov_ldflags = $(LDFLAGS) -lgcov
GCOV = gcov
gcovflags = --preserve-paths --branch-probabilities --all-blocks \
  --function-summaries
.PHONY: coverage-clean coverage-build coverage-show coverage-gen
cov_src = cmogstored.c $(mog_src) $(RL_MAIN)

# this doesn't work in out-of-tree builds, yet...
cover_db = $(top_srcdir)/cover_db

COVERAGE_JUNK = *.gcov *.gcda *.gcno
CLEANFILES += $(COVERAGE_JUNK)
CLEANFILES += $(addprefix bsd/, $(COVERAGE_JUNK))
CLEANFILES += $(addprefix lib/, $(COVERAGE_JUNK))
CLEANFILES += $(addprefix nostd/, $(COVERAGE_JUNK))
CLEANFILES += $(addprefix test/, $(COVERAGE_JUNK))

coverage-clean:
	$(MAKE) clean
	$(RM) -r $(cover_db)

COVERAGE_CHECK = check
coverage-build: coverage-clean
	$(MAKE) CFLAGS="$(cov_cflags)" LDFLAGS="$(cov_ldflags)"
coverage-check:
	$(MAKE) CFLAGS="$(cov_cflags)" LDFLAGS="$(cov_ldflags)" \
	  -j1 $(COVERAGE_CHECK)
	$(MAKE) coverage-gen

COVER_IGNORE = $(addprefix -ignore ,$(RL_CGEN)) -ignore_re ^lib/
coverage-show:
	@cover $(COVER_IGNORE) -summary -report text $(cover_db)

coverage-gen:
	$(GCOV) $(gcovflags) --object-directory=$(top_builddir) \
		$(addprefix $(top_srcdir)/,$(cov_src))
	$(AM_V_GEN)gcov2perl -db $(cover_db) *.gcov >/dev/null 2>&1
	@echo Run '"'make coverage-show'"' to view summary

coverage: coverage-build
	$(MAKE) coverage-check
	$(MAKE) coverage-show
	@echo Run '"'make coverage-show'"' to view summary
