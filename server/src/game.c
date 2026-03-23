#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_PLAYERS 4
#define MIN_PLAYERS 2

int clients[MAX_PLAYERS];
int client_count = 0;
int game_started = 0;

void broadcast(const char *msg, int sender) {
    for (int i = 0; i < client_count; i++) {
        if (i != sender) {
            send(clients[i], msg, strlen(msg), 0);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <porta>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    int game_fd;
    struct sockaddr_in address;

    game_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(game_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[GAME] Bind fallita\n");
        exit(EXIT_FAILURE);
    }

    if (listen(game_fd, 10) < 0) {
        perror("[GAME] Listen fallita\n");
        exit(EXIT_FAILURE);
    }

    printf("[GAME] Partita su porta %d\n", port);

    char buffer[1024];

    while (1) {
        // Accetta nuovi player (non bloccare troppo)
        if (client_count < MAX_PLAYERS) {
            int new_socket = accept(game_fd, NULL, NULL);
            if (new_socket >= 0) {
                printf("[GAME] Player connesso\n");
                clients[client_count++] = new_socket;

                if (client_count >= MIN_PLAYERS && !game_started) {
                    printf("[GAME] Partita iniziata!\n");
                    game_started = 1;
                }

            }else{
                perror("[GAME] Player non connesso\n");
                exit(EXIT_FAILURE);
            }
        }

        // Gestione messaggi
        for (int i = 0; i < client_count; i++) {
            int valread = recv(clients[i], buffer, sizeof(buffer), MSG_DONTWAIT);

            if (valread > 0) {
                buffer[valread] = '\0';
                broadcast(buffer, i);
            }
        }
    }

    return 0;
}