default:
	@echo "Explicit target required"

jemalloc:
	cd jemalloc && ./configure --with-jemalloc-prefix=je_ --enable-cc-silence CFLAGS="$(JEMALLOC_CFLAGS)" LDFLAGS="$(JEMALLOC_LDFLAGS)"
	cd jemalloc && $(MAKE) lib/libjemalloc.a

distclean:
	-(cd jemalloc && [ -f Makefile ] && $(MAKE) distclean) > /dev/null || true

.PHONY: default jemalloc distclean
