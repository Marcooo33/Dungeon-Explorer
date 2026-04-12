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
    int id;
    Status status;
} Player;

typedef struct {
    char code[7];
    Player players[MAX_PLAYERS];
    int num_players;
    bool started;
} Game;


extern Game games[MAX_GAMES];
extern int game_count;
extern char **environ;

#endif