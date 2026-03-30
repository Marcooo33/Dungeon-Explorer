#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
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
        if (strncmp(games[i].code, code, 6) == 0) return i;  
    }
    printf("[MATCHMAKER] Codice partita non trovato: %s\n", code);
    return -1;
}


pthread_mutex_t games_mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_host_loop(int game_idx) {
    Player* host = &games[game_idx].players[0];
    char buffer[1024];

    printf("[MATCHMAKER DEBUG] In attesa di comandi dall'host per la partita %s\n", games[game_idx].code);

    while (games[game_idx].started == false) {
        // La recv blocca il thread finché l'host non invia un messaggio
        memset(buffer, 0, sizeof(buffer));
        int valread = recv(host->socket_fd, buffer, sizeof(buffer) - 1, 0);

        if (valread <= 0) {
            printf("[MATCHMAKER] L'host si è disconnesso.\n");
            // Qui potresti gestire l'eliminazione della partita o la chiusura dei socket
            break;
        }

        buffer[valread] = '\0';
        printf("[MATCHMAKER DEBUG] Comando ricevuto dall'host: %s\n", buffer);
        pthread_mutex_lock(&games_mutex);

        // GESTIONE DEL TCP FRAMING: Dividiamo il buffer usando '\n' come separatore
        char *command = strtok(buffer, "\n");
        
        while (command != NULL) {
            // Se l'host invia "ACCEPT <id>"
            if (strncmp(command, "ACCEPT", 6) == 0) {
                int player_index;
                if (sscanf(command, "ACCEPT %d", &player_index) == 1 && player_index > 0 && player_index < MAX_PLAYERS) {
                    if (games[game_idx].players[player_index].status == PENDING) {
                        games[game_idx].players[player_index].status = CONNECTED;
                        games[game_idx].num_players++;
                        printf("[MATCHMAKER] Giocatore %d accettato nella partita %s\n", player_index, games[game_idx].code);
                        
                        Player* accepted_player = &games[game_idx].players[player_index];
                        send(accepted_player->socket_fd, "JOIN_ACCEPTED\n", 14, 0);
                    }
                }
            } 
            // Se l'host invia "REJECT <id>"
            else if (strncmp(command, "REJECT", 6) == 0) {
                int player_index;
                if (sscanf(command, "REJECT %d", &player_index) == 1 && player_index > 0 && player_index < MAX_PLAYERS) {
                    if (games[game_idx].players[player_index].status == PENDING) {
                        games[game_idx].players[player_index].status = DISCONNECTED;
                        printf("[MATCHMAKER] Giocatore %d rifiutato nella partita %s\n", player_index, games[game_idx].code);
                        
                        // NOTA: Qui dovresti notificare al socket del client che è stato rifiutato
                        Player* rejected_player = &games[game_idx].players[player_index];
                        send(rejected_player->socket_fd, "JOIN_REJECTED\n", 14, 0);
                    }
                }
            }
            // Se l'host invia "START_GAME"
            else if (strncmp(command, "START_GAME", 10) == 0) {
                if (games[game_idx].num_players >= 2) {
                    games[game_idx].started = true;
                    printf("[MATCHMAKER] Partita %s avviata!\n", games[game_idx].code);
                } else {
                    send(host->socket_fd, "ERROR MIN_PLAYERS_NOT_MET\n", 27, 0);
                }
            }

            // Passiamo al comando successivo estratto dal buffer (se esiste)
            command = strtok(NULL, "\n");
        }

        pthread_mutex_unlock(&games_mutex);
        print_info_game(game_idx);
    }
}


void handle_join(int client_socket_fd) {
    bool accepted = false;
    char buffer[1024];
    char join_code[7];

    while (!accepted) {
        memset(buffer, 0, sizeof(buffer));
        int valread = recv(client_socket_fd, buffer, sizeof(buffer) - 1, 0);
        
        if (valread <= 0) {
            printf("[MATCHMAKER] Client disconnesso durante la join.\n");
            close(client_socket_fd);
            return;
        }

        if (sscanf(buffer, "JOIN %7s", join_code) != 1) {
            send(client_socket_fd, "ERROR INVALID_FORMAT\n", 22, 0);
            continue; 
        }

        pthread_mutex_lock(&games_mutex);
        int game_idx = find_game_by_code(join_code);

        if (game_idx == -1) {
            printf("[MATCHMAKER] Codice errato: %s. Richiedo nuovo inserimento.\n", join_code);
            send(client_socket_fd, "ERROR NOT_FOUND\n", 17, 0);
            pthread_mutex_unlock(&games_mutex);
            continue;
        } 
        else if (games[game_idx].started) {
            printf("[MATCHMAKER] Partita %s piena.\n", join_code);
            send(client_socket_fd, "ERROR STARTED\n", 15, 0);
            pthread_mutex_unlock(&games_mutex);
            continue;
        } 
        else { // Trovata la partita
            int idx_new_player = find_free_player_slot(&games[game_idx]);
            
            if (idx_new_player == -1) {
                send(client_socket_fd, "ERROR FULL\n", 15, 0);
                pthread_mutex_unlock(&games_mutex);
                continue;
            }

            Player* new_player = &games[game_idx].players[idx_new_player];
            new_player->socket_fd = client_socket_fd;
            new_player->status = PENDING;
            
            char join_request_msg[128];
            sprintf(join_request_msg, "JOIN_REQUEST %d\n", idx_new_player);
            Player* host = &games[game_idx].players[0];
            send(host->socket_fd, join_request_msg, strlen(join_request_msg), 0);
            
            printf("[MATCHMAKER] Inviata richiesta di join all'host per la partita %s\n", join_code);
            
            // SBLOCCHIAMO IL MUTEX PRIMA DI METTERCI IN ATTESA
            pthread_mutex_unlock(&games_mutex);

            accepted = wait_for_host_decision(new_player);
        }
    } 
}


int find_free_player_slot(Game* game) {
    for (int i = 1; i < MAX_PLAYERS; i++) { // Partiamo da 1 perché 0 è l'host
        if (game->players[i].status == DISCONNECTED) 
            return i;
    }
    return -1;
}

bool wait_for_host_decision(Player* new_player) {
    bool decision_made = false;
    bool is_accepted = false;

    while (!decision_made) {
        usleep(100000); // Dorme per 100ms per non sprecare CPU
        
        pthread_mutex_lock(&games_mutex);
        
        if (new_player->status == CONNECTED) {
            // L'host ha accettato!
            decision_made = true;
            is_accepted = true;
        } 
        else if (new_player->status == DISCONNECTED) {
            // L'host ha rifiutato!
            decision_made = true;
            is_accepted = false;
        }
        
        pthread_mutex_unlock(&games_mutex);
    }
    
    return is_accepted;
}

void print_info_game(int game_idx) {
    Game game = games[game_idx];
    printf("=== INFO PARTITA %s ===\n", game.code);
    printf("Giocatori connessi: %d\n", game.num_players);
    printf("Stato: %s\n", game.started ? "Avviata" : "In attesa");
    
    for (int i = 0; i < MAX_PLAYERS; i++) {
        printf("Giocatore %d: %d\n", i, game.players[i].status);
    }    
    printf("=====================\n");
}