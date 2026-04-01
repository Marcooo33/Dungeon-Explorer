#include <string.h>
#include <sys/socket.h>
#include "game.h"

void broadcast(const char *message) {
    for (int i = 0; i < connected_count; i++) {
        send(players[i].socket_fd, message, strlen(message), 0);
    }
}