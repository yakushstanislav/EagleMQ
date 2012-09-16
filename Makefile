all:
	cd src && $(MAKE) $@

run:
	cd src && $(MAKE) $@

clean:
	cd src && $(MAKE) $@

.PHONY: all run clean
