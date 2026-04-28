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
        current_room->encounter(players);
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
        broadcast(room_info_msg);
        

        // 3) ricevere la decisione
        
        
        // 4) mandare la stanza da renderizzare
    }
    

}