CC=gcc
CFLAGS=-Wall -Wextra -O2 -I./includes -I../shared -pthread

SRC1=src/matchmaker.c
SRC2=src/game.c
OUT1=matchmaker
OUT2=game

compile_matchmaker:
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC1) -o $(OUT1)
	mv $(OUT1) bin/

compile_game:
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC2) -o $(OUT2)
	mv $(OUT2) bin/

exec:
	./bin/$(OUT1)

all: compile_matchmaker compile_game exec

clean:
	rm -rf bin/* 