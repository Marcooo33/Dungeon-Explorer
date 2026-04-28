#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#define MAX_PLAYERS 4
#define MAX_DOORS 4

typedef struct Room Room; // Forward declaration of Room for use in EncounterFunction
typedef void (*EncounterFunction)(); // Define a function pointer type for room encounters


typedef struct {
    int socket_fd;
    int id;
    int x,y;
    int hp;
    bool alive;
} Player;


typedef enum Direction{
    NORTH,
    SOUTH,
    EAST,
    WEST
} Direction;


typedef struct Room{
    int idx;
    int doors_num; // number of doors in the room (to iterate over the doors array)
    char *id;
    char *type;
    bool completed;
    Direction doors[MAX_DOORS];
    int connected_rooms[MAX_DOORS]; // index of the connected rooms in the dungeon's rooms array 
    EncounterFunction encounter; // pointer to the function that represents the encounter that occurs when the player enters the room
} Room;


typedef struct Dungeon{
    int rooms_num; //initially the max number of rooms, later it becomes the actual number of rooms generated, useful for iterating over the rooms array
    Room *rooms;
} Dungeon;





extern Player players[MAX_PLAYERS];
extern int connected_count;
extern bool game_started;
extern char *game_code;

#endif