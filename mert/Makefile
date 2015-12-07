CC=gcc
OBJS=mert.o data.o point.o score.o vector.o strstrsep.o
CFLAGS=-O3 -g -Wall
LDFLAGS=-pg
LDLIBS=-lm 

all: mert sentserver sentclient

mert: $(OBJS)
	gcc $(OBJS) $(LDLIBS) -o $@

sentserver: sentserver.c
	gcc sentserver.c -lpthread -o $@

sentclient: sentclient.c
	gcc sentclient.c -lpthread -o $@

