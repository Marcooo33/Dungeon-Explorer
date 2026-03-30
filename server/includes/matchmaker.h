#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#include <stdbool.h>

#define MAX_GAMES 10
#define MAX_PLAYERS 4

typedef enum {
    DISCONNECTED,
    PENDING,
    CONNECTED,
} Status;

typedef struct {
    int socket_fd;
    Status status;
} Player;

typedef struct {
    int port;
    char code[7];
    Player players[MAX_PLAYERS];
    int num_players;
    bool started;
} Game;


extern Game games[MAX_GAMES];
extern int game_count;

#endif