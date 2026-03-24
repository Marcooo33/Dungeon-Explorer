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

int main(int argc, char *argv[]) {
    if (argc < 3) exit(1);

    int port = atoi(argv[1]);
    char *game_code = argv[2];

    int game_fd;
    struct sockaddr_in address;
    
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind game fallita");
        exit(1);
    }

    listen(server_fd, MAX_PLAYERS);
    printf("[GAME %s] In ascolto sulla porta %d\n", game_code, port);

    // Inizializza giocatori
    for (int i = 0; i < MAX_PLAYERS; i++) players[i].connected = false;

    // FASE DI ATTESA: Accettiamo i giocatori (es. aspettiamo che arrivino tutti)
    while (!game_started) {
        int client_socket_fd = accept(server_fd, NULL, NULL);
        
        players[connected_count].socket = client_socket_fd;
        players[connected_count].id = connected_count;
        players[connected_count].x = 0 + (connected_count * 50); // Posizioni diverse
        players[connected_count].y = 0;
        players[connected_count].connected = true;

        char spawn_msg[64];
        sprintf(spawn_msg, "SPAWN %d %d %d\n", players[connected_count].id, players[connected_count].x, players[connected_count].y);
        send(client_socket_fd, spawn_msg, strlen(spawn_msg), 0);
        
        printf("Giocatore %d connesso alla partita %s\n", connected_count, game_code);
        connected_count++;
        
        // Se abbiamo raggiunto il numero, inviamo lo SPAWN a tutti
        if (connected_count >= 2) {
            char msg[64];
            // Invia a tutti la posizione di ogni giocatore connesso
            for (int i = 0; i < connected_count; i++) {
                sprintf(msg, "SPAWN %d %d %d\n", players[i].id, players[i].x, players[i].y);
                broadcast(msg, i);
                sleep(1); // Piccolo delay per non saturare il buffer socket
            }
        }
    }

    // Qui inizieresti il ciclo dei turni (Ricevi mossa da tutti -> Calcola -> Invia Update)
    //...

    return 0;
}