#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "dungeon_generator.h"

void default_encounter(Room *room) {
    // Default encounter function, can be overridden by specific room types
    printf("You have entered room %s of type %s with %d doors at index %d.\n", room->id, room->type, room->doors_num, room->idx);
}

// (2) Mock funzione JSON
void random_room_template(Room *room, Direction required_door){
    int r = rand() % 3;

    // Tipo stanza
    if(r == 0) room->type = "battle";
    else if(r == 1) room->type = "treasure";
    else room->type = "trap";

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


int generate_room(Dungeon *dungeon, int *last_idx, int current_room_idx, Direction door_direction){
    int new_idx = ++(*last_idx);
    Room *new_room = &dungeon->rooms[new_idx];

    new_room->idx = new_idx;

    // (2) scelta stanza compatibile, per ora mock, in futuro lettura dal json
    random_room_template(new_room, get_opposite_direction(door_direction));

    // collega stanze
    new_room->connected_rooms[get_opposite_direction(door_direction)] = current_room_idx;

    new_room->completed = false;
    new_room->encounter = default_encounter;

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
    dungeon.rooms = calloc(rooms_num*2, sizeof(Room)); // allocate more rooms than rooms_num to account for dead-ends and boss room

    //Inizializzazione delle conessioni delle stanze (non so se necessario)
    for(int i=0;i<rooms_num*2;i++)
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

int main(){
    int seed = time(NULL);
    srand(seed);
    printf("Dungeon Generator Test. Seed: %d\n", seed);

    Dungeon dungeon = generate_dungeon(5);
    print_dungeon(dungeon);

    return 0;
}

