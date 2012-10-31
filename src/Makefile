EAGLEMQ_BIN=eaglemq
EAGLEMQ_LDFLAGS=-pthread
EAGLEMQ_OBJ=eagle.o event.o network.o xmalloc.o utils.o object.o handlers.o list.o queue.o user.o queue_t.o

CFLAGS=-std=c99 -pedantic -Wall -O2

INSTALL_DIR=/usr/local/bin
INSTALL_CMD=cp -pf

ifeq ($(USE_SELECT),yes)
	CFLAGS+=-D_EVENT_SELECT_
endif

ifeq ($(MALLOC),libc)
	CFLAGS+=-D_USE_MALLOC_
else
	CFLAGS+=-D_USE_JEMALLOC_ -I../deps/jemalloc/include
	EAGLEMQ_LDFLAGS+=../deps/jemalloc/lib/libjemalloc.a -ldl
	DEPS+=jemalloc
endif

all: $(DEPS) $(EAGLEMQ_BIN)

jemalloc:
	cd ../deps && $(MAKE) JEMALLOC_CFLAGS="-std=gnu99 -Wall -pipe -g3 -O3 -funroll-loops" jemalloc

$(EAGLEMQ_BIN): $(EAGLEMQ_OBJ)
	gcc -o $@ $^ $(EAGLEMQ_LDFLAGS)

%.o: %.c
	gcc -c $< $(CFLAGS)

dep:
	gcc -MM *.c > Makefile.dep

run:
	./$(EAGLEMQ_BIN)

install: all
	mkdir -p $(INSTALL_DIR)
	$(INSTALL_CMD) $(EAGLEMQ_BIN) $(INSTALL_DIR)

uninstall:
	rm -rf $(INSTALL_DIR)/$(EAGLEMQ_BIN)

clean:
	rm -rf *o $(EAGLEMQ_BIN)

distclean: clean
	cd ../deps && $(MAKE) distclean

.PHONY: all dep run install uninstall clean
