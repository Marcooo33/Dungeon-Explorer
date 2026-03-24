#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#include <stdbool.h>

#define MAX_GAMES 10

typedef struct {
int port;
int players;
char code[8];
bool started;
} Game;

extern Game games[MAX_GAMES];
extern int game_count;

#endif