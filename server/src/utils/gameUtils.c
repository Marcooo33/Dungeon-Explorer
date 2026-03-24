#include <string.h>
#include <sys/socket.h>
#include "game.h"

void broadcast(const char *message, int root_id) {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].connected && i != root_id) {
            send(players[i].socket, message, strlen(message), 0);
        }
    }
}