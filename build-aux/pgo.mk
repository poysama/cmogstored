# profile-guided optimization snippets
# This isn't tested or verified for correctness at all, yet.
# Help is greatly appreciated.

PGO_DIR = $(top_builddir)
pgo_gen_cflags = $(CFLAGS) -O0 -fprofile-arcs -fprofile-generate=$(PGO_DIR)
pgo_gen_ldflags = $(LDFLAGS) -O0 -lgcov

pgo_build_cflags = $(CFLAGS) -fprofile-use=$(PGO_DIR) -fprofile-correction

PGO_CHECK = check
pgo-gen: export CCACHE_DISABLE=t
pgo-gen:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(pgo_gen_cflags)" LDFLAGS="$(pgo_gen_ldflags)"
	$(MAKE) CFLAGS="$(pgo_gen_cflags)" LDFLAGS="$(pgo_gen_ldflags)" \
	  -j1 $(PGO_CHECK)

pgo-build: export CCACHE_DISABLE=t
pgo-build: pgo-gen
	$(MAKE) clean COVERAGE_JUNK=
	git ls-files -o
	$(MAKE) CFLAGS="$(pgo_build_cflags)"

pgo-install: export CCACHE_DISABLE=t
pgo-install: pgo-build
	$(MAKE) CFLAGS="$(pgo_build_cflags)" install
