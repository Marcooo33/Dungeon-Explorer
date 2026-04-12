CC=gcc
CFLAGS=-Wall -Wextra -O2 -I./includes -I../shared -pthread

SRC1=src/matchmaker.c src/utils/matchmakerUtils.c
SRC2=src/game.c src/utils/gameUtils.c
SRC3=src/dungeon_generator.c

OUT1=matchmaker
OUT2=game
OUT3=dungeon_generator

compile_matchmaker:
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC1) -o $(OUT1)
	mv $(OUT1) bin/

compile_game:
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC2) -o $(OUT2)
	mv $(OUT2) bin/

compile_dungeon_generator:
	mkdir -p bin
	$(CC) $(CFLAGS) $(SRC3) -o $(OUT3)
	mv $(OUT3) bin/

exec:
	./bin/$(OUT1)

all: compile_matchmaker compile_game compile_dungeon_generator exec

test: compile_dungeon_generator 
	./bin/$(OUT3)

clean:
	rm -rf bin/* 