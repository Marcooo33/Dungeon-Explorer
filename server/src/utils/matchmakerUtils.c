#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <poll.h>

#include "matchmaker.h"
#include "matchmakerUtils.h"

void generate_code(char *code) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < 6; i++) {
        code[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    code[6] = '\0';
}

int find_game_by_code(const char *code) {
    for (int i = 0; i < game_count; i++) {
        if (strcmp(games[i].code, code) == 0) return i;
    }
    return -1;
}


pthread_mutex_t games_mutex = PTHREAD_MUTEX_INITIALIZER;
void handle_host_loop(int game_idx) {
    Player host = games[game_idx].players[0];
    
    struct pollfd fds[1];
    fds[0].fd = host.socket_fd;
    fds[0].events = POLLIN;

    printf("[MATCHMAKER DEBUG] In attesa di giocatori per la partita %s\n", games[game_idx].code);

    // FEAT: possibilità di mettere un limite di tempo affinché i giocatori si uniscano, dopo di che si inizia automaticamente o si annulla la partita
    while (games[game_idx].started == false) {

        // Scorro tra i giocatori in attesa e invio loro una richiesta di join se passa da DISCONNECTED a PENDING
        for (int i = 1; i < MAX_PLAYERS - 1; i++) {
            Player player = games[game_idx].players[i];
            if (player.status == PENDING) {
                char buffer[128];
                sprintf(buffer, "JOIN_REQUEST %d", i);
                send(host.socket_fd, buffer, strlen(buffer), 0);
                recv(host.socket_fd, buffer, sizeof(buffer), 0);

                pthread_mutex_lock(&games_mutex);

                if (strcmp(buffer, "ACCEPT") == 0) {  
                    games[game_idx].players[i].status = CONNECTED;
                    games[game_idx].num_players++;
                } else 
                    games[game_idx].players[i].status = DISCONNECTED;

                pthread_mutex_unlock(&games_mutex);
            }
        }

        int ret = poll(fds, 1, 100); 

        if (ret > 0 && (fds[0].revents & POLLIN)) {
            char buffer[128];
            int n = recv(host.socket_fd, buffer, sizeof(buffer) - 1, 0);
            if (n > 0) {
                buffer[n] = '\0';
                
                // Se ricevo START_GAME, aggiorno lo stato ed esco dal while
                if (strcmp(buffer, "START_GAME") == 0) {
                    pthread_mutex_lock(&games_mutex);
                    games[game_idx].started = true;
                    pthread_mutex_unlock(&games_mutex);
                    break; 
                }
            }
        }
    }
    
}