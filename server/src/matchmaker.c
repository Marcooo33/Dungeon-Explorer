#include <stdio.h>
#include <stdlib.h>
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
pthread_mutex_t games_mutex = PTHREAD_MUTEX_INITIALIZER;

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


void *handle_client(void *arg) {
    int client_socket_fd = *(int *)arg;
    free(arg); // Libero la memoria allocata nel main

    char client_response[1024] = {0};
    int valread = recv(client_socket_fd, client_response, sizeof(client_response) - 1, 0);

    if (valread <= 0) {
        close(client_socket_fd);
        return NULL;
    }

    char server_response[128];
    
    // COMANDO: CREATE
    if (strncmp(client_response, "CREATE", 6) == 0) {
        char new_code[8];
        pthread_mutex_lock(&games_mutex);
        int port = start_new_game(new_code);
        pthread_mutex_unlock(&games_mutex);

        if (port != -1) {
            sprintf(server_response, "START %d %s\n", port, new_code);
        } else {
            sprintf(server_response, "ERROR: SERVER_FULL\n");
        }
    } 
    // COMANDO: JOIN <code>
    else if (strncmp(client_response, "JOIN", 4) == 0) {
        char join_code[8];
        sscanf(client_response, "JOIN %7s", join_code);

        pthread_mutex_lock(&games_mutex);
        int idx = find_game_by_code(join_code);

        if (idx != -1 && !games[idx].started && games[idx].players < 4) {
            games[idx].players++;
            sprintf(server_response, "START %d %s\n", games[idx].port, games[idx].code);
        } else {
            sprintf(server_response, "ERROR: NOT_FOUND_OR_FULL\n");
        }
        pthread_mutex_unlock(&games_mutex);
    }
    else {
        sprintf(server_response, "ERROR: INVALID_COMMAND\n");
    }

    send(client_socket_fd, server_response, strlen(server_response), 0);
    close(client_socket_fd); // Importante: il client ora deve connettersi alla porta del gioco
    return NULL;
}

// Funzione chiamata dentro il Mutex nel thread
int start_new_game(char *out_code) {
    if (game_count >= MAX_GAMES) return -1;

    int fd[2];
    if (pipe(fd) == -1) return -1;

    int port = 6000 + (rand() % 1000);
    char code[8];
    generate_code(code);

    pid_t pid = fork();

    if (pid == 0) {
        // PROCESSO FIGLIO (Game Server)

        close(fd[0]); // Chiude il lato lettura
        char port_str[10];
        char pipe_fd_str[10];
        sprintf(port_str, "%d", port);
        sprintf(pipe_fd_str, "%d", fd[1]);
        
        execl("./bin/game", "game", port_str, code, pipe_fd_str, NULL);
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // PROCESSO PADRE

        close(fd[1]); // Chiude il lato scrittura
        char buffer[3];
        read(fd[0], buffer, 2); // Padre aspetta qui finché il figlio non scrive sulla pipe
        close(fd[0]);

        games[game_count].port = port;
        games[game_count].players = 1;
        games[game_count].started = false;
        strcpy(games[game_count].code, code);
        strcpy(out_code, code);
        
        game_count++;
        return port;
    } else {
        perror("Fork fallita");
        return -1;
    }
}