#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#define MAX_PLAYERS 4
#define MAX_DOORS 4

#define SHORT_RANGE 1
#define LONG_RANGE 3

typedef struct Room Room; 
typedef struct Player Player; 
typedef struct Monster Monster; 

typedef void (*EncounterFunction)(Player *players, int num_players); // Define a function pointer type for room encounters
typedef void (*AttackFunction)(void *attacker, void *target);
typedef void (*ItemFunction)(Player *player); // Define a function pointer type for item usage


void treasure_encounter1(Player *players, int num_players);
void treasure_encounter2(Player *players, int num_players);
void treasure_encounter3(Player *players, int num_players);


void trap_encounter(Player *players, int num_players);

void combat_encounter(Player *players, int num_players);
void boss_encounter(Player *players, int num_players);

void melee_attack(void *attacker, void *target);
void ranged_attack(void *attacker, void *target);
void monster_attack(void *attacker, void *target);

void health_potion_function(Player *player);

typedef struct Weapon{
    char *name;
    int damage;
    int range;
    AttackFunction attack; // Function pointer to define the attack behavior of the weapon
} Weapon;

typedef struct Armor{
    char *name;
    int defense;
} Armor;

typedef struct Item{
    char *name;
    ItemFunction use; // Function pointer to define the effect of using the item
} Item; 

typedef struct Player{
    // communication
    int socket_fd;
    int id;

    // game info
    int x,y;
    int hp;
    bool alive;
    int gold;
    Weapon *weapon;
    Armor *armor;
    Item *item; 

} Player;

typedef struct Monster {
    // game info
    char *name;
    int hp;
    int x, y;

    Weapon *weapon;
    Armor *armor;
} Monster;


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