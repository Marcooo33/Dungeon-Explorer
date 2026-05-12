#include <unistd.h>
   
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h>
#include <math.h>
#include "game.h"

void broadcast(const char *message) {
    for (int i = 0; i < connected_count; i++) {
        send(players[i].socket_fd, message, strlen(message), 0);
    }
}

void broadcast_player_info(Player *player) {
    char player_info_message[128];
    sprintf(player_info_message, "PLAYER_INFO %d %d %d %d %d %s:%d:%d %s:%d %s\n", 
            player->id, 
            player->hp, 
            player->x, 
            player->y,
            player->gold,
            player->weapon ? player->weapon->name : "Nessuna",
            player->weapon ? player->weapon->damage : 0,
            player->weapon ? player->weapon->range : 0,
            player->armor ? player->armor->name : "Nessuna",
            player->armor ? player->armor->defense : 0,
            player->item ? player->item->name : "Nessuno"
    );
    broadcast(player_info_message);
}

void default_encounter() {
    // Default encounter function, can be overridden by specific room types
    sleep(2); // Simula un incontro che dura 2 secondi
    printf("You have entered in a room\n");
}

// (2) Mock funzione JSON
void random_room_template(Room *room, Direction required_door){
    int room_type = rand() % 3;

    // Tipo stanza
    if(room_type == 0){
        room->type = "battle";
        room->encounter = combat_encounter;
    } else if(room_type == 1) {
        room->type = "treasure";
        room->encounter = treasure_encounters[rand() % 3];
    } else {
        room->type = "trap";
        room->encounter = trap_encounter;
    }

    room->id = room->type;

    // Numero porte (min 1 = required, max 4)
    int max_extra = rand() % 3; // 0–2 porte extra
    room->doors_num = 2 + max_extra;

    // Lista direzioni disponibili
    Direction all_dirs[4] = {NORTH, SOUTH, EAST, WEST};

    // Inserisci porta obbligatoria
    room->doors[0] = required_door;

    int count = 1;

    // Aggiungi porte casuali senza duplicati
    for(int i = 0; i < 4 && count < room->doors_num; i++){
        Direction d = all_dirs[i];

        // salta se è già presente
        bool already_present = false;
        for(int j = 0; j < count; j++){
            if(room->doors[j] == d){
                already_present = true;
                break;
            }
        }

        if(already_present) continue;

        // 50% probabilità di includerla
        if(rand() % 2){
            room->doors[count++] = d;
        }
    }

    // fallback: se non abbiamo abbastanza porte
    for(int i = 0; count < room->doors_num && i < 4; i++){
        Direction d = all_dirs[i];

        bool already_present = false;
        for(int j = 0; j < count; j++){
            if(room->doors[j] == d){
                already_present = true;
                break;
            }
        }

        if(!already_present){
            room->doors[count++] = d;
        }
    }
}


Direction get_opposite_direction(Direction door){
    switch(door){
        case NORTH: return SOUTH;
        case SOUTH: return NORTH;
        case EAST: return WEST;
        case WEST: return EAST;
        default: return -1; // Invalid direction
    }
}

void build_room_message(Room *room, char *buffer, size_t size) {
    char doors_str[8] = "";
    int pos = 0;

    Direction ordered_dirs[4] = {NORTH, SOUTH, EAST, WEST};

    for (int i = 0; i < 4; i++) {
        Direction d = ordered_dirs[i];

        for (int j = 0; j < room->doors_num; j++) {
            if (room->doors[j] == d) {

                switch (d) {
                    case NORTH: doors_str[pos++] = 'n'; break;
                    case SOUTH: doors_str[pos++] = 's'; break;
                    case EAST:  doors_str[pos++] = 'e'; break;
                    case WEST:  doors_str[pos++] = 'w'; break;
                }

                break;
            }
        }
    }

    doors_str[pos] = '\0';

    snprintf(buffer, size, "LOAD_ROOM %s\n", doors_str);
}


int generate_room(Dungeon *dungeon, int *last_idx, int current_room_idx, Direction door_direction){
    int new_idx = ++(*last_idx);
    Room *new_room = &dungeon->rooms[new_idx];

    new_room->idx = new_idx;

    // (2) scelta stanza compatibile, per ora mock, in futuro lettura dal json
    random_room_template(new_room, get_opposite_direction(door_direction));

    // collega stanze
    new_room->connected_rooms[get_opposite_direction(door_direction)] = current_room_idx;

    new_room->completed = false;

    return new_idx;
}

// (3) BFS per trovare il ramo più lungo del dungeon, da chiamare alla fine della enerazione del dungeon per generare la stanza del boss nel ramo più lungo
int find_farthest_room(Dungeon *dungeon){
    int dist[1000] = {0};
    bool visited[1000] = {false};

    int queue[1000];
    int front = 0, back = 0;

    queue[back++] = 0;
    visited[0] = true;

    int farthest = 0;

    while(front < back){
        int curr = queue[front++];
        Room *r = &dungeon->rooms[curr];

        for(int i=0;i<r->doors_num;i++){
            int next = r->connected_rooms[r->doors[i]];
            if(next != -1 && !visited[next]){
                visited[next] = true;
                dist[next] = dist[curr] + 1;
                queue[back++] = next;

                if(dist[next] > dist[farthest])
                    farthest = next;
            }
        }
    }

    return farthest;
}


//Generates a dungeon with rooms_num *internal* rooms (not counting dead-ends and boss room)
Dungeon generate_dungeon(int rooms_num){
    //Dungeon allocation
    Dungeon dungeon;
    dungeon.rooms_num = rooms_num;
    dungeon.rooms = calloc(rooms_num*3, sizeof(Room)); // allocate more rooms than rooms_num to account for dead-ends and boss room

    //Inizializzazione delle conessioni delle stanze (non so se necessario)
    for(int i=0;i<rooms_num*3;i++)
        for(int j=0;j<MAX_DOORS;j++)
            dungeon.rooms[i].connected_rooms[j] = -1;

    //Starting room generation
    dungeon.rooms[0].idx = 0;
    dungeon.rooms[0].doors_num = 1;
    dungeon.rooms[0].id = "starting_room";
    dungeon.rooms[0].type = "start";
    dungeon.rooms[0].doors[0] = EAST;
    dungeon.rooms[0].completed = false;
    dungeon.rooms[0].encounter = default_encounter;


    //Dungeon generation
    int generated=1;
    int last_idx=0;
    int current_room_idx=0;
    int door_number=0;
    Direction door_direction;
    Room *current_room;
    while (generated < dungeon.rooms_num){
        current_room = &dungeon.rooms[current_room_idx];
        for(door_number=0; door_number < current_room->doors_num; door_number++){
            door_direction = current_room->doors[door_number];
            if (current_room->connected_rooms[door_direction] == -1){
                current_room->connected_rooms[door_direction] = generate_room(&dungeon, &last_idx, current_room_idx, door_direction);
                generated++;
            }
        }
        current_room_idx++;
    }

    dungeon.rooms_num = generated;

    //Dead-ends and boss room generation
    /* 3) Questo ciclo serve a generare le stanze morte e la stanza del boss, la stanza del boss deve essere generata nel ramo più lungo del dungeon, quindi bisogna 
    effettuare anche la ricerca del ramo più lungo oltre a generare le stanza morte per tutte quelle stanze che hanno porte non ancora connesse */
    for(int i=0;i<dungeon.rooms_num;i++){
        Room *r = &dungeon.rooms[i];

        for(int j=0;j<r->doors_num;j++){
            Direction d = r->doors[j];

            if(r->connected_rooms[d] == -1){
                int new_idx = ++last_idx;
                Room *dead = &dungeon.rooms[new_idx];

                dead->idx = new_idx;
                dead->type = "dead_end";
                dead->id = "dead_end";
                dead->doors_num = 1;
                dead->doors[0] = get_opposite_direction(d);
                dead->connected_rooms[get_opposite_direction(d)] = i;
                dead->encounter = default_encounter;

                r->connected_rooms[d] = new_idx;
                generated++;
            }
        }
    }

    dungeon.rooms_num = generated;

    // (3) boss nella stanza più lontana
    int boss_idx = find_farthest_room(&dungeon);
    dungeon.rooms[boss_idx].type = "boss";
    dungeon.rooms[boss_idx].id = "boss_room";
    dungeon.rooms[boss_idx].encounter = boss_encounter;
    return dungeon;
}

const char* direction_to_string(Direction d){
    switch(d){
        case NORTH: return "NORTH";
        case SOUTH: return "SOUTH";
        case EAST:  return "EAST";
        case WEST:  return "WEST";
        default:    return "UNKNOWN";
    }
}

Direction string_to_direction(const char *str) {
    if (strcmp(str, "NORTH") == 0) return NORTH;
    if (strcmp(str, "SOUTH") == 0) return SOUTH;
    if (strcmp(str, "EAST")  == 0) return EAST;
    if (strcmp(str, "WEST")  == 0) return WEST;

    return -1; // direzione non valida
}

// funzione helper per validità porta
bool is_valid_direction(Direction d, Room *room) {
    for (int i = 0; i < room->doors_num; i++) {
        if (room->doors[i] == d)
            return true;
    }
    return false;
}

void print_room(Room *r){
    if(r == NULL){
        printf("Room NULL\n");
        return;
    }

    printf("Room %d\n", r->idx);
    printf("  ID: %s\n", r->id);
    printf("  Type: %s\n", r->type);
    printf("  Completed: %s\n", r->completed ? "true" : "false");

    printf("  Doors (%d):\n", r->doors_num);

    for(int i = 0; i < r->doors_num; i++){
        Direction d = r->doors[i];
        int connected = r->connected_rooms[d];

        printf("    - %s -> ", direction_to_string(d));

        if(connected != -1)
            printf("Room %d\n", connected);
        else
            printf("NONE\n");
    }

    printf("\n");
}

void print_dungeon(Dungeon dungeon){
    printf("=== DUNGEON (%d rooms) ===\n\n", dungeon.rooms_num);

    for(int i = 0; i < dungeon.rooms_num; i++){
        print_room(&dungeon.rooms[i]);
    }
}

int distance(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}