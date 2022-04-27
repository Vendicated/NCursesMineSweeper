all: build

SOURCES=main.c
FLAGS=-lncurses -o ncms

build:
	gcc ${SOURCES} ${FLAGS}

run: build
	./ncms

debug:
	gcc ${SOURCES} -g -rdynamic ${FLAGS} -DDEBUG=true
