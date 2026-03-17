CC=gcc
CFLAGS=-Wall -Wextra -O2 -I../shared

SRC=src/server.c
OUT=server

compile:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)
	mv $(OUT) bin/

exec:
	./bin/$(OUT)

all: compile exec

clean:
	rm -rf bin/* 