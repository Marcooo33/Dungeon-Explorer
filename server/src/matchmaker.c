#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>

#define PORT 8080
#define MAX_GAMES 10

struct Game {
    int port;
    int players;
    char code[8];
    bool started;
};

struct Game games[MAX_GAMES];
int game_count = 0;
pthread_mutex_t games_mutex = PTHREAD_MUTEX_INITIALIZER;

void generate_code(char *code);
int start_new_game(char *out_code);
int find_game_by_code(const char *code);
void *handle_client(void *arg);

int main() {
    srand(time(NULL));

    int server_fd;
    struct sockaddr_in address;
    int opt=1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Evita l'errore "Address already in use" al riavvio del server
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind fallita");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen fallita");
        exit(EXIT_FAILURE);
    }

    printf("[MATCHMAKER] In ascolto su porta %d...\n", PORT);

    
    while (true) {
        int new_socket = accept(server_fd, NULL, NULL);

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, new_socket);
        pthread_detach(tid);
        printf("il thread %d ha finito\n", (int)tid);
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

int start_new_game(char *out_code) {
    int port = 6000 + rand() % 1000;

    pid_t pid = fork();

    if (pid == 0) {
        char port_str[10];
        sprintf(port_str, "%d", port);

        execl("./bin/game", "game", port_str, NULL);
        perror("execl fallita");
        exit(1);

    } else {

        char code[8];
        generate_code(code);
    
        printf("[MATCHMAKER] Nuova partita %s su porta %d\n", code, port);
    
        games[game_count].port = port;
        games[game_count].players = 1;
        strcpy(games[game_count].code, code);
    
        strcpy(out_code, code);
    
        game_count++;
    
        return port;
    }

}

int find_game_by_code(const char *code) {
    for (int i = 0; i < game_count; i++) {
        if (strcmp(games[i].code, code) == 0) {
            return i;
        }
    }
    return -1;
}

void *handle_client(void *arg){
    int new_socket = *(int *)arg;
    free(arg);

    char buffer[1024];
    int valread = recv(new_socket, buffer, sizeof(buffer), 0);

    if (valread <= 0) {
        close(new_socket);
        return NULL;
    }

    buffer[valread] = '\0';

    int port;
    char code[8] = "";

    // CREATE
    if (strncmp(buffer, "CREATE", 6) == 0) {
        pthread_mutex_lock(&games_mutex);
        port = start_new_game(code);
        pthread_mutex_unlock(&games_mutex);
    }

    // JOIN
    else if (strncmp(buffer, "JOIN", 4) == 0) {
        char join_code[8];
        sscanf(buffer, "JOIN %7s", join_code);

        pthread_mutex_lock(&games_mutex);

        int idx = find_game_by_code(join_code);

        if (idx == -1) {
            pthread_mutex_unlock(&games_mutex);
            send(new_socket, "[MATCHMAKER] ERROR: CODE_NOT_FOUND\n", 23, 0);
            close(new_socket);
            return NULL;
        }

        if (games[idx].players >= 4 || games[idx].started) {
            pthread_mutex_unlock(&games_mutex);
            send(new_socket, "[MATCHMAKER] ERROR: FULL\n", 12, 0);
            close(new_socket);
            return NULL;
        }

        port = games[idx].port;
        strcpy(code, games[idx].code);
        games[idx].players++;

        pthread_mutex_unlock(&games_mutex);
    }

    // STARTED (dal game server)
    else if (strncmp(buffer, "STARTED", 7) == 0) {
        char started_code[8];
        sscanf(buffer, "STARTED %7s", started_code);

        pthread_mutex_lock(&games_mutex);

        int idx = find_game_by_code(started_code);
        if (idx != -1) {
            games[idx].started = 1;
            printf("[MATCHMAKER] Partita %s iniziata\n", started_code);
        }

        pthread_mutex_unlock(&games_mutex);

        close(new_socket);
        return NULL;
    }

    else {
        send(new_socket, "[MATCHMAKER] ERROR: UNKNOWN_COMMAND\n", 23, 0);
        close(new_socket);
        return NULL;
    }

    // risposta al client
    char msg[64];
    sprintf(msg, "START %d %s\n", port, code);

    send(new_socket, msg, strlen(msg), 0);
    close(new_socket);

    return NULL;
}