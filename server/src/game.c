#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <stdbool.h>
#include "game.h"
#include "gameUtils.h"

Player players[MAX_PLAYERS];
int connected_count = 0;
char *game_code = NULL;
Dungeon dungeon;

void init_players() {
    for (int i = 0; i < connected_count; i++) {
        players[i].hp = 100;
        players[i].alive = true;
        players[i].x = 0; // Posizione iniziale X
        players[i].y = i * 50; // Posizione iniziale Y
    }
}

int main(int argc, char *argv[]) {
    if(argc < 3){
        printf("Uso: %s <game_code> <fds>\n", argv[0]);
        return -1;
    }

    game_code = argv[1];
    printf("[GAME %s] Partita creata\n", game_code);

    connected_count = 0;
    
    // Variabili per mantenere lo stato (necessarie per strtok_r)
    char *saveptr_sockets;
    char *saveptr_ids;

    // Estraiamo il primo token da entrambe le stringhe
    char *socket_token = strtok_r(argv[2], " ", &saveptr_sockets);
    char *id_token = strtok_r(argv[3], " ", &saveptr_ids);

    // Cicliamo finché abbiamo elementi in entrambe le stringhe
    while (socket_token != NULL && id_token != NULL) {
        // Popoliamo la struct del giocatore corrente
        players[connected_count].socket_fd = atoi(socket_token);
        players[connected_count].id = atoi(id_token);

        connected_count++;

        // Passiamo al token successivo per entrambe le stringhe
        socket_token = strtok_r(NULL, " ", &saveptr_sockets);
        id_token = strtok_r(NULL, " ", &saveptr_ids);


        printf("[GAME %s] Ricevuti %d socket\n", game_code, connected_count);
        broadcast("START_GAME\n");
        sleep(3);

        // Inviamo a ciascun client il proprio ID
        char specific_client_msg[64];
        for (int i = 0; i < connected_count; i++) {
            sprintf(specific_client_msg, "SET_MY_ID %d\n", players[i].id);
            send(players[i].socket_fd, specific_client_msg, strlen(specific_client_msg), 0);
        }

        init_players();

        char player_info_message[128];
        for (int i = 0; i < connected_count; i++) {
            sprintf(player_info_message, "PLAYER_INFO %d %d %d %d\n", 
                    players[i].id, 
                    players[i].hp, 
                    (int)players[i].x, 
                    (int)players[i].y);
                    
            broadcast(player_info_message);
        }


        // Debug: stampa info giocatori
        printf("[DEBUG] Connessi %d giocatori\n", connected_count);
        printf("[GAME %s] Player Info:\n", game_code);
        for (int i = 0; i < connected_count; i++) {
            printf("Player %d: Socket FD=%d, HP=%d, Alive=%s, Position=(%d,%d)\n",
            players[i].id, players[i].socket_fd, players[i].hp, players[i].alive ? "Yes" : "No", players[i].x, players[i].y);
        }
    }

    dungeon = generate_dungeon(5);
    int current_room_idx = 0; // Iniziamo nella prima stanza del dungeon
    int next_room_idx = -1; 
    char room_info_msg[256];
    Room *current_room = NULL;

    // game loop start
    while (true){
        current_room = &dungeon.rooms[current_room_idx];

        // 1) far svolgere encounter stanza
        current_room->encounter(current_room);
        current_room->completed = true;
        
        // 2) mandare una sorta di messaggio make decision
        Direction door_direction;
        Room *adjacent_room;

        sprintf(room_info_msg, "MAKE_DECISION ");
        for(int door_number=0;door_number < current_room->doors_num;door_number++){
            door_direction = current_room->doors[door_number];
            adjacent_room = &dungeon.rooms[current_room->connected_rooms[door_direction]];
            sprintf(room_info_msg + strlen(room_info_msg), "%s:%s:%s ", direction_to_string(door_direction), adjacent_room->type, adjacent_room->completed ? "completed" : "not_completed");
        }
        sprintf(room_info_msg + strlen(room_info_msg), "\n"); // Aggiungi newline alla fine del messaggio
        broadcast(room_info_msg);
        

        // 3) ricevere la decisione
        int votes[4] = {0}; // NORTH, SOUTH, EAST, WEST
        int received = 0;
        bool has_voted[MAX_PLAYERS] = {false};

        struct pollfd fds[MAX_PLAYERS];

        // Inizializzazione
        for (int i = 0; i < connected_count; i++) {
            fds[i].fd = players[i].socket_fd;
            fds[i].events = POLLIN;
            fds[i].revents = 0;
        }

        int timeout_ms = 20000; // 20 secondi
        int elapsed = 0;
        int step = 1000; // controlliamo ogni 1 secondo

        while (received < connected_count && elapsed < timeout_ms) {
            int activity = poll(fds, connected_count, step);

            if (activity < 0) {
                perror("poll error");
                break;
            }

            if (activity == 0) {
                // nessun evento → è passato 1 secondo
                elapsed += step;
                continue;
            }

            for (int i = 0; i < connected_count; i++) {
                if ((fds[i].revents & POLLIN) && !has_voted[i]) {
                    char buffer[128] = {0};
                    int bytes = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);

                    if (bytes <= 0) continue;

                    if (strncmp(buffer, "SEND_DECISION ", 14) == 0) {
                        char *dir_str = buffer + 14;
                        dir_str[strcspn(dir_str, "\r\n")] = 0;

                        Direction d = string_to_direction(dir_str);

                        votes[d]++;

                        has_voted[i] = true;
                        received++;

                        printf("[GAME %s] Player %d voted: %s\n",
                            game_code, players[i].id, dir_str);
                    }
                }
            }
        }

        // Debug timeout
        if (received < connected_count) {
            printf("[GAME %s] Timeout votazione (%d/%d ricevuti)\n",
                game_code, received, connected_count);
        }

        int max_votes = 0;

        // trova massimo
        for (int i = 0; i < 4; i++) {
            if (votes[i] > max_votes) {
                max_votes = votes[i];
            }
        }


        // raccogli candidati validi
        Direction candidates[4];
        int candidate_count = 0;

        for (int i = 0; i < 4; i++) {
            if (votes[i] == max_votes && max_votes > 0 &&
                is_valid_direction(i, current_room)) {
                candidates[candidate_count++] = i;
            }
        }

        Direction chosen_direction;

        // scelta finale
        if (candidate_count == 0) {
            printf("[GAME %s] Nessun voto valido, fallback casuale\n", game_code);

            // fallback su porte valide della stanza
            int r = rand() % current_room->doors_num;
            chosen_direction = current_room->doors[r];
        }
        else if (candidate_count == 1) {
            chosen_direction = candidates[0];
        }
        else {
            int r = rand() % candidate_count;
            chosen_direction = candidates[r];

            printf("[GAME %s] Pareggio tra %d direzioni, scelta: %s\n",
                game_code,
                candidate_count,
                direction_to_string(chosen_direction));
        }

        // debug finale
        printf("[GAME %s] Decisione finale: %s\n",
            game_code,
            direction_to_string(chosen_direction));

        // aggiorna stanza
        next_room_idx = current_room->connected_rooms[chosen_direction];
        
        
        // 4) mandare la stanza da renderizzare
        char room_msg[64];
        build_room_message(&dungeon.rooms[next_room_idx], room_msg, sizeof(room_msg));

        // invio a tutti i client
        broadcast(room_msg);

        // debug
        printf("[GAME %s] Sent: %s", game_code, room_msg);
        
        current_room_idx = next_room_idx;
    }
    

}