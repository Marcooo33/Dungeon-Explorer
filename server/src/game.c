#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "game.h"
#include "gameUtils.h"

Player players[MAX_PLAYERS];
int connected_count = 0;
bool game_started = false;
char *game_code = NULL;

int main(int argc, char *argv[]) {
    if(argc < 3){
        printf("Uso: %s <game_code> <fds>\n", argv[0]);
    }

    game_code = argv[1];
    printf("[GAME %s] Partita creata\n", game_code);

    connected_count=0;
    char *token = strtok(argv[2], " ");
    while (token != NULL) {
        players[connected_count++].socket_fd = atoi(token);
        token = strtok(NULL, " ");
    }

    printf("[GAME %s] Ricevuti %d socket\n", game_code, connected_count);
    broadcast("START\n");
}