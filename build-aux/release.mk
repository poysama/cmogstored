release-check:
# git clean won't nuke read-only directories
	$(RM) -rf cmogstored-*
	git clean -f -d -x -q
	./autogen.sh
	./configure
	$(MAKE)
	$(MAKE) distcheck
	$(MAKE) distcheck-harder

distcheck-harder:
	$(RAKE) distcheck_git TGZ=$(DIST_ARCHIVES)
