CC=gcc
LDFLAGS=-pthread $(shell pkg-config --cflags --libs gtk+-3.0)
CFLAGS=-g -ggdb -Wall

all: 
	${CC} ${CFLAGS} gtk-wrap.c -o gtk-wrap ${LDFLAGS}

clean: 
	rm gtk-bash

strip:
	strip -s gtk-bash

update: pull all 

pull:
	git pull git@github.com:abecadel/gtkwrap.git
