#ifndef DUNGEON_GENERATOR_H
#define DUNGEON_GENERATOR_H

#include <stdbool.h>
#define MAX_DOORS 4

typedef enum Direction{
    NORTH,
    SOUTH,
    EAST,
    WEST
} Direction;

typedef struct Room Room; // Forward declaration of Room for use in EncounterFunction
typedef void (*EncounterFunction)(Room*); // Define a function pointer type for room encounters

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

Dungeon generate_dungeon(int rooms_num);
int generate_room(Dungeon *dungeon, int *last_idx, int current_room_idx, Direction door_direction);
Direction get_opposite_direction(Direction door);
void print_room(Room *r);
void print_dungeon(Dungeon dungeon);

#endif