all:
	cd src && $(MAKE) $@

run:
	cd src && $(MAKE) $@

install:
	cd src && $(MAKE) $@

uninstall:
	cd src && $(MAKE) $@

clean:
	cd src && $(MAKE) $@

changelog:
	@echo "Changelog created!"
	@git log --no-merges --format="%cd %s (%an)" --date=short > Changelog

.PHONY: all run clean
