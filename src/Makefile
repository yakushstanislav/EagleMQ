all: EagleMQ

CFLAGS?=-std=c99 -pedantic -Wall -W -O3
CCLINK?=-lpthread
OBJ=eagle.o event.o network.o xmalloc.o utils.o object.o handlers.o list.o queue.o user.o queue_t.o

EagleMQ: $(OBJ)
	gcc $(OBJ) -o eaglemq $(CFLAGS) $(CCLINK)

eagle.o: eagle.c
	gcc -c eagle.c

event.o: event.c
	gcc -c event.c

network.o: network.c
	gcc -c network.c

xmalloc.o: xmalloc.c
	gcc -c xmalloc.c

utils.o: utils.c
	gcc -c utils.c

object.o: object.c
	gcc -c object.c

handlers.o: handlers.c
	gcc -c handlers.c

list.o: list.c
	gcc -c list.c

queue.o: queue.c
	gcc -c queue.c

user.o: user.c
	gcc -c user.c

queue_t.o: queue_t.c
	gcc -c queue_t.c

run:
	./eaglemq

clean:
	rm -rf *o eaglemq
