#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080
#define MAX_GAMES 10

struct Game {
    int port;
    int players;
    char code[8];
};

struct Game games[MAX_GAMES];
int game_count = 0;

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
        games[game_count].players = 0;
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

int main() {
    srand(time(NULL));

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
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

    char buffer[1024];
    
    while (1) {
        new_socket = accept(server_fd, NULL, NULL);
        if (new_socket<0){
            perror("Errore nell'accettazione del client");
            exit(EXIT_FAILURE);
        }
        printf("[MATCHMAKER] Connessione del client accettata\n");

        memset(buffer, 0, sizeof(buffer));
        int valread = recv(new_socket, buffer, sizeof(buffer), 0);
        

        if (valread <= 0) {
            close(new_socket);
            continue;
        }

        buffer[valread] = '\0';
        printf("[MATCHMAKER] Ricevuto: %s\n", buffer);

        int port;
        char code[8] = "";

        // CREATE
        if (strncmp(buffer, "CREATE", 6) == 0) {
            port = start_new_game(code);

        }
        // JOIN <CODE>
        else if (strncmp(buffer, "JOIN", 4) == 0) {
            char join_code[8];
            sscanf(buffer, "JOIN %7s", join_code);

            int idx = find_game_by_code(join_code);

            if (idx == -1) {
                char *err = "[MATCHMAKER] ERROR: CODE_NOT_FOUND\n";
                send(new_socket, err, strlen(err), 0);
                close(new_socket);
                continue;
            }

            if (games[idx].players >= 4) {
                char *err = "[MATCHMAKER] ERROR: FULL\n";
                send(new_socket, err, strlen(err), 0);
                close(new_socket);
                continue;
            }

            port = games[idx].port;
            strcpy(code, games[idx].code);
            games[idx].players++;
        }
        else {
            char *err = "[MATCHMAKER] ERROR: UNKNOWN_COMMAND\n";
            send(new_socket, err, strlen(err), 0);
            close(new_socket);
            continue;
        }

        // Risposta
        char msg[64];
        sprintf(msg, "[MATCHMAKER] START %d %s\n", port, code);

        
        send(new_socket, msg, strlen(msg), 0);
        close(new_socket);
    }

    return 0;
}