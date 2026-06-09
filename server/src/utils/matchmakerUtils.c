#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <spawn.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "matchmaker.h"
#include "matchmakerUtils.h"

// Dichiarazione esterna per environ, necessaria per posix_spawn
extern char **environ;

pthread_mutex_t games_mutex = PTHREAD_MUTEX_INITIALIZER;

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

int find_game_by_pid(pid_t pid) {
    for (int i = 0; i < game_count; i++) {
        if (games[i].game_pid == pid) return i;
    }
    return -1;
}

void *host_loop_thread(void *arg) {
    int game_idx = *(int *)arg;
    free(arg);
    handle_host_loop(game_idx);
    return NULL;
}

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
            break;
        }

        buffer[valread] = '\0';
        printf("[MATCHMAKER DEBUG] Comando ricevuto dall'host: %s\n", buffer);
        
        // GESTIONE DEL TCP FRAMING: Dividiamo il buffer usando '\n' come separatore
        char *command = strtok(buffer, "\n");
        
        while (command != NULL) {
            // Se l'host invia "ACCEPT <id>"
            if (strncmp(command, "ACCEPT", 6) == 0) {
                int player_index;
                if (sscanf(command, "ACCEPT %d", &player_index) == 1 && player_index > 0 && player_index < MAX_PLAYERS) {
                    if (games[game_idx].players[player_index].status == PENDING) {
                        
                        // Sezione critica
                        pthread_mutex_lock(&games_mutex);
                        games[game_idx].players[player_index].id = player_index;
                        games[game_idx].players[player_index].status = CONNECTED;
                        games[game_idx].num_players++;
                        pthread_mutex_unlock(&games_mutex);
                        // Fine sezione critica

                        printf("[MATCHMAKER] Giocatore %d accettato nella partita %s\n", player_index, games[game_idx].code);
                        
                        Player* accepted_player = &games[game_idx].players[player_index];
                        
                        // --- RIPRISTINATO COME PRIMA ---
                        // Inviamo un semplice JOIN_ACCEPTED senza numero
                        send(accepted_player->socket_fd, "JOIN_ACCEPTED\n", 14, MSG_NOSIGNAL);

                        // Inviamo il codice stanza al nuovo giocatore (necessario per non avere UI vuota)
                        char room_code_msg[64];
                        snprintf(room_code_msg, sizeof(room_code_msg), "ROOM_CODE %s\n", games[game_idx].code);
                        send(accepted_player->socket_fd, room_code_msg, strlen(room_code_msg), MSG_NOSIGNAL);

                        // Broadcast della lista a tutti (necessario per aggiornare l'host)
                        broadcast_player_list(&games[game_idx]);
                    }
                }
            } 
            // Se l'host invia "REJECT <id>"
            else if (strncmp(command, "REJECT", 6) == 0) {
                int player_index;
                if (sscanf(command, "REJECT %d", &player_index) == 1 && player_index > 0 && player_index < MAX_PLAYERS) {
                    if (games[game_idx].players[player_index].status == PENDING) {

                        // Sezione critica
                        pthread_mutex_lock(&games_mutex);
                        games[game_idx].players[player_index].status = DISCONNECTED;
                        pthread_mutex_unlock(&games_mutex);
                        // Fine sezione critica

                        printf("[MATCHMAKER] Giocatore %d rifiutato nella partita %s\n", player_index, games[game_idx].code);
                        
                        Player* rejected_player = &games[game_idx].players[player_index];
                        send(rejected_player->socket_fd, "JOIN_REJECTED\n", 14, MSG_NOSIGNAL);
                    }
                }
            }
            // Se l'host invia "START_GAME"
            else if (strncmp(command, "START_GAME", 10) == 0) {
                if (games[game_idx].num_players >= 2) {

                    // Sezione critica
                    pthread_mutex_lock(&games_mutex);
                    games[game_idx].started = true;
                    pthread_mutex_unlock(&games_mutex);
                    // Fine sezione critica

                    printf("[MATCHMAKER] Richiesta di inzio per la partita %s\n", games[game_idx].code);
                    
                    // Avvisiamo tutti i client che il gioco inizia
                    for (int i = 0; i < MAX_PLAYERS; i++) {
                        if (games[game_idx].players[i].status == CONNECTED) {
                            send(games[game_idx].players[i].socket_fd, "START_GAME\n", 11, MSG_NOSIGNAL);
                        }
                    }
                    
                    start_game(game_idx);
                    return;
                    
                } else {
                    send(host->socket_fd, "ERROR MIN_PLAYERS_NOT_MET\n", 27, MSG_NOSIGNAL);
                }
            }

            // Passiamo al comando successivo estratto dal buffer (se esiste)
            command = strtok(NULL, "\n");
        }

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
            send(client_socket_fd, "ERROR INVALID_FORMAT\n", 22, MSG_NOSIGNAL);
            continue; 
        }

        pthread_mutex_lock(&games_mutex);
        int game_idx = find_game_by_code(join_code);

        if (game_idx == -1) {
            printf("[MATCHMAKER] Codice errato: %s. Richiedo nuovo inserimento.\n", join_code);
            send(client_socket_fd, "ERROR NOT_FOUND\n", 17, MSG_NOSIGNAL);
            pthread_mutex_unlock(&games_mutex);
            continue;
        } 
        else if (games[game_idx].started) {
            printf("[MATCHMAKER] Partita %s piena.\n", join_code);
            send(client_socket_fd, "ERROR STARTED\n", 15, MSG_NOSIGNAL);
            pthread_mutex_unlock(&games_mutex);
            continue;
        } 
        else { // Trovata la partita
            int idx_new_player = find_free_player_slot(&games[game_idx]);
            
            if (idx_new_player == -1) {
                send(client_socket_fd, "ERROR FULL\n", 15, MSG_NOSIGNAL);
                pthread_mutex_unlock(&games_mutex);
                continue;
            }

            Player* new_player = &games[game_idx].players[idx_new_player];
            new_player->socket_fd = client_socket_fd;
            new_player->status = PENDING;
            
            char join_request_msg[128];
            sprintf(join_request_msg, "JOIN_REQUEST %d\n", idx_new_player);
            Player* host = &games[game_idx].players[0];
            send(host->socket_fd, join_request_msg, strlen(join_request_msg), MSG_NOSIGNAL);
            
            printf("[MATCHMAKER] Inviata richiesta di join all'host per la partita %s\n", join_code);
            
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
            decision_made = true;
            is_accepted = true;
        } 
        else if (new_player->status == DISCONNECTED) {
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
        printf("Giocatore %d: Socket_FD=%d, Status=%d\n", i, game.players[i].socket_fd, game.players[i].status);
    }    
    printf("=====================\n");
}

void start_game(int game_idx){
    Game *game = &games[game_idx];
    pid_t pid;

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    // 🔹 Assicuriamoci che le socket NON vengano chiuse su exec
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].status == CONNECTED) {
            int fd = game->players[i].socket_fd;

            int flags = fcntl(fd, F_GETFD);
            flags &= ~FD_CLOEXEC;
            fcntl(fd, F_SETFD, flags);
        }
    }

    // 🔹 Costruzione stringhe separate per Sockets e IDs
    char sockets_args[256] = {0};
    char ids_args[256] = {0};
    char temp[32];

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].status == CONNECTED) {
            sprintf(temp, "%d ", game->players[i].socket_fd);
            strcat(sockets_args, temp);

            sprintf(temp, "%d ", game->players[i].id);
            strcat(ids_args, temp);
        }
    }

    char *argv[] = {
        "game_server",
        game->code,     // argv[1]: Codice partita
        sockets_args,   // argv[2]: Stringa dei socket fd
        ids_args,       // argv[3]: Stringa degli ID dei giocatori
        NULL
    };

    int status = posix_spawn(
        &pid,
        "/workspace/server/bin/game",
        &actions,
        NULL,
        argv,
        environ
    );

    posix_spawn_file_actions_destroy(&actions);

    if (status == 0) {
        printf("[MATCHMAKER] Game server avviato (pid=%d) per partita %s\n",
               pid, game->code);
        
        // Salviamo il PID all'interno della struct di gioco per tracciarlo nel monitor
        game->game_pid = pid;

    } else {
        perror("posix_spawn fallita");
    }
}

// Monitor Thread Globale Semplificato
void *game_monitor_loop(void *arg) {
    while (1) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);

        if (pid > 0) {
            pthread_mutex_lock(&games_mutex);
            int idx = find_game_by_pid(pid);

            if (idx != -1) {
                if (WIFEXITED(status)) {
                    int exit_code = WEXITSTATUS(status);

                    // 🟢 L'HOST HA SCELTO DI CONTINUARE: Portiamo tutti indietro
                    if (exit_code == 10) {
                        printf("[MONITOR] Il creatore della partita %s ha scelto di continuare. Tutti tornano in Lobby.\n", games[idx].code);
                        
                        games[idx].started = false;
                        games[idx].game_pid = 0;

                        // Avvisiamo tutti i client che sono di nuovo in lobby e scartiamo chi è crashato nel frattempo
                        bool list_changed = false;
                        for (int i = 0; i < MAX_PLAYERS; i++) {
                            if (games[idx].players[i].status == CONNECTED) {
                                // Controllo robusto della connessione con poll() prima di inviare
                                struct pollfd pfd;
                                pfd.fd = games[idx].players[i].socket_fd;
                                pfd.events = POLLIN;
                                
                                int activity = poll(&pfd, 1, 0); // timeout 0, ritorna subito
                                bool is_disconnected = false;

                                if (activity > 0) {
                                    if (pfd.revents & (POLLHUP | POLLERR)) {
                                        is_disconnected = true;
                                    } else if (pfd.revents & POLLIN) {
                                        char dump_buf[16];
                                        if (recv(pfd.fd, dump_buf, sizeof(dump_buf), MSG_PEEK | MSG_DONTWAIT) == 0) {
                                            is_disconnected = true; // EOF ricevuto (connessione chiusa)
                                        }
                                    }
                                }

                                if (is_disconnected) {
                                    printf("[MONITOR] Rilevata disconnessione del giocatore %d (crashato durante la partita).\n", i);
                                    games[idx].players[i].status = DISCONNECTED;
                                    close(games[idx].players[i].socket_fd);
                                    games[idx].num_players--;
                                    list_changed = true;
                                } else {
                                    // Inviamo il messaggio solo se la connessione è ancora attiva
                                    send(games[idx].players[i].socket_fd, "MESSAGE Siete tornati in lobby! In attesa che l'host avvii una nuova partita...\n", 79, MSG_NOSIGNAL);
                                }
                            }
                        }

                        if (list_changed) {
                            broadcast_player_list(&games[idx]);
                        }

                        // Facciamo ripartire il thread dell'host che ascolta i comandi
                        int *p_idx = malloc(sizeof(int));
                        if (p_idx != NULL) {
                            *p_idx = idx;
                            pthread_t tid;
                            if (pthread_create(&tid, NULL, host_loop_thread, p_idx) == 0) {
                                pthread_detach(tid);
                            } else {
                                free(p_idx);
                            }
                        }
                    } 
                    // 🔴 GAME OVER O SCIOGLIMENTO STANZA
                    else {
                        printf("[MONITOR] La partita %s è conclusa o sciolta. Svuoto la stanza.\n", games[idx].code);
                        for (int i = 0; i < MAX_PLAYERS; i++) {
                            if (games[idx].players[i].status == CONNECTED || games[idx].players[i].status == PENDING) {
                                close(games[idx].players[i].socket_fd);
                            }
                        }
                        memset(&games[idx], 0, sizeof(Game));
                    }
                } 
                // ⚠️ CRASH DEL PROCESSO FIGLIO
                else {
                    printf("[MONITOR] ATTENZIONE: Il processo %s è crashato. Forzo la chiusura.\n", games[idx].code);
                    for (int i = 0; i < MAX_PLAYERS; i++) {
                        if (games[idx].players[i].status == CONNECTED || games[idx].players[i].status == PENDING) {
                            close(games[idx].players[i].socket_fd);
                        }
                    }
                    memset(&games[idx], 0, sizeof(Game));
                }
            }
            pthread_mutex_unlock(&games_mutex);
        }
        usleep(100000); // 100ms
    }
    return NULL;
}

// Funzione per inviare la lista aggiornata dei giocatori a tutti i client della stanza
void broadcast_player_list(Game* game) {
    char response[1024];
    strcpy(response, "PLAYER_LIST");
    
    // Costruiamo la stringa con gli ID dei giocatori connessi
    char temp[32];
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].status == CONNECTED) {
            sprintf(temp, " %d", game->players[i].id);
            strcat(response, temp);
        }
    }
    strcat(response, "\n");

    // Inviamo la stringa a tutti i giocatori connessi
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].status == CONNECTED) {
            send(game->players[i].socket_fd, response, strlen(response), MSG_NOSIGNAL);
        }
    }
}