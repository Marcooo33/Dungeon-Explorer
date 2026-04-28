#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#include <stdbool.h>
#include "game.h"


const char* direction_to_string(Direction dir);
Direction string_to_direction(const char *str);
bool is_valid_direction(Direction d, Room *room);
Dungeon generate_dungeon(int rooms_num);
int generate_room(Dungeon *dungeon, int *last_idx, int current_room_idx, Direction door_direction);
Direction get_opposite_direction(Direction door);
void print_room(Room *r);
void print_dungeon(Dungeon dungeon);
const char* direction_to_string(Direction d);


void broadcast(const char *message);

#endif
