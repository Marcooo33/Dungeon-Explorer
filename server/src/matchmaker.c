#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>

#define PORT 8080
#define MAX_GAMES 10

struct Game {
    int port;
    int players;
    char code[8];
    bool started;
};

// Global State
struct Game games[MAX_GAMES];
int game_count = 0;
pthread_mutex_t games_mutex = PTHREAD_MUTEX_INITIALIZER;

// Prototipi
void generate_code(char *code);
int start_new_game(char *out_code);
int find_game_by_code(const char *code);
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
        int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        
        if (new_socket < 0) continue;

        // ALLOCAZIONE DINAMICA: fondamentale per evitare SegFault tra i thread
        int *client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = new_socket;

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_sock_ptr) != 0) {
            fprintf(stderr, "Errore creazione thread\n");
            close(new_socket);
            free(client_sock_ptr);
        }
        pthread_detach(tid); 
    }

    return 0;
}

void generate_code(char *code) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < 6; i++) {
        code[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    code[6] = '\0';
}

// Funzione chiamata dentro il Mutex nel thread
int start_new_game(char *out_code) {
    if (game_count >= MAX_GAMES) return -1;

    int port = 6000 + (rand() % 1000);
    char code[8];
    generate_code(code);

    pid_t pid = fork();

    if (pid == 0) {
        // PROCESSO FIGLIO (Game Server)
        char port_str[10];
        sprintf(port_str, "%d", port);
        
        printf("[GAME-CHILD] Avvio Game Server su porta %s con codice %s\n", port_str, code);
        
        // Esegue il binario del gioco passando porta e codice come argomenti
        execl("./bin/game", "game", port_str, code, NULL);
        
        perror("Errore execl");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // PROCESSO PADRE
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

int find_game_by_code(const char *code) {
    for (int i = 0; i < game_count; i++) {
        if (strcmp(games[i].code, code) == 0) return i;
    }
    return -1;
}

void *handle_client(void *arg) {
    int sock = *(int *)arg;
    free(arg); // Libero la memoria allocata nel main

    char buffer[1024] = {0};
    int valread = recv(sock, buffer, sizeof(buffer) - 1, 0);

    if (valread <= 0) {
        close(sock);
        return NULL;
    }

    char response[128];
    
    // COMANDO: CREATE
    if (strncmp(buffer, "CREATE", 6) == 0) {
        char new_code[8];
        pthread_mutex_lock(&games_mutex);
        int port = start_new_game(new_code);
        pthread_mutex_unlock(&games_mutex);

        if (port != -1) {
            sprintf(response, "START %d %s\n", port, new_code);
        } else {
            sprintf(response, "ERROR: SERVER_FULL\n");
        }
    } 
    // COMANDO: JOIN <code>
    else if (strncmp(buffer, "JOIN", 4) == 0) {
        char join_code[8];
        sscanf(buffer, "JOIN %7s", join_code);

        pthread_mutex_lock(&games_mutex);
        int idx = find_game_by_code(join_code);

        if (idx != -1 && !games[idx].started && games[idx].players < 4) {
            games[idx].players++;
            sprintf(response, "START %d %s\n", games[idx].port, games[idx].code);
        } else {
            sprintf(response, "ERROR: NOT_FOUND_OR_FULL\n");
        }
        pthread_mutex_unlock(&games_mutex);
    }
    else {
        sprintf(response, "ERROR: INVALID_COMMAND\n");
    }

    send(sock, response, strlen(response), 0);
    close(sock); // Importante: il client ora deve connettersi alla porta del gioco
    return NULL;
}