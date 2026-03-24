#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#define MAX_PLAYERS 4

typedef struct {
    int socket;
    int id;
    int x, y;
    bool connected;
} Player;

extern Player players[MAX_PLAYERS];
extern int connected_count;
extern bool game_started;

#endif