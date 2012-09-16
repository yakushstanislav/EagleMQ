EAGLEMQ_BIN=eaglemq
EAGLEMQ_CFLAGS=-std=c99 -pedantic -Wall -W -O3
EAGLEMQ_LDFLAGS=-lpthread
EAGLEMQ_OBJ=eagle.o event.o network.o xmalloc.o utils.o object.o handlers.o list.o queue.o user.o queue_t.o

all: $(EAGLEMQ_BIN)

$(EAGLEMQ_BIN): $(EAGLEMQ_OBJ)
	gcc -o $@ $^ $(EAGLEMQ_LDFLAGS)

%.o: %.c
	gcc -c $< $(EAGLEMQ_CFLAGS)

dep:
	gcc -MM *.c > Makefile.dep

run:
	./$(EAGLEMQ_BIN)

clean:
	rm -rf *o $(EAGLEMQ_BIN)

.PHONY: all dep run clean
