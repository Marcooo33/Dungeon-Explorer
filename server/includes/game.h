#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#define MAX_PLAYERS 4

typedef struct {
    int socket_fd;
    int id;
    int x, y;
    int hp;
    bool alive;
} Player;

extern Player players[MAX_PLAYERS];
extern int connected_count;
extern bool game_started;
extern char *game_code;

#endif