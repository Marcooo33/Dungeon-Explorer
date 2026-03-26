#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#include "matchmaker.h"
#include "matchmakerUtils.h"

#define PORT 8080

Game games[MAX_GAMES];
int game_count = 0;

// Prototipi
int start_new_game(char *out_code);
void *handle_client(void *arg);

int main() {
    srand(time(NULL));
    
    // Evita processi zombie dai game server figli
    signal(SIGCHLD, SIG_IGN);

    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket fallita");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind fallita");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen fallita");
        exit(EXIT_FAILURE);
    }

    printf("[MATCHMAKER] Server avviato sulla porta %d...\n", PORT);

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        printf("[MATCHMAKER DEBUG] Nuova connessione accettata\n");
        
        if (client_socket_fd < 0) continue;

        // ALLOCAZIONE DINAMICA: fondamentale per evitare SegFault tra i thread
        int *client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_socket_fd;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_sock_ptr) != 0) {
            fprintf(stderr, "Errore creazione thread\n");
            close(client_socket_fd);
            free(client_sock_ptr);
        }
        pthread_detach(tid); 
    }

    return 0;
}

void *handle_client(void *arg){
    printf("[MATCHMAKER DEBUG] Gestione client\n");
    int client_socket_fd = *(int *)arg;
    free(arg); // Libero la memoria allocata nel main

    char client_response[1024] = {0};
    int valread = recv(client_socket_fd, client_response, sizeof(client_response) - 1, 0);

    if (valread <= 0) {
        close(client_socket_fd);
        return NULL;
    }

    // COMANDO: CREATE
    if (strncmp(client_response, "CREATE", 6) == 0) {
        printf("[MATCHMAKER DEBUG] Comando CREATE ricevuto\n");
        pthread_mutex_lock(&games_mutex);
        
        generate_code(games[game_count].code);
        games[game_count].num_players = 1;
        Player host = games[game_count].players[0];
        host.socket_fd = client_socket_fd;
        host.status = CONNECTED;
        int game_idx = game_count;
        game_count++;
        
        pthread_mutex_unlock(&games_mutex);
        
        handle_host_loop(game_idx);
    } 

    // else if Join
     while (1) {
        /* code */
     }
    
}
