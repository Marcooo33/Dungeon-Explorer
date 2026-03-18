CC=gcc
CFLAGS=-Wall -Wextra -O2 -I./includes -I../shared -pthread

SRC=src/server.c
OUT=server

compile:
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)
	mv $(OUT) bin/

exec:
	./bin/$(OUT)

all: compile exec

clean:
	rm -rf bin/* 