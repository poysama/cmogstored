# sparse warns a little too much by default, but it's better than nothing...
SPARSE_FLAGS = -Wdefault-bitfield-sign
sparse:
	$(RM) $(cmogstored_OBJECTS)
	$(MAKE) CC=cgcc CFLAGS="$(CFLAGS) $(SPARSE_FLAGS)"
