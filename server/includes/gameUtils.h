#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#include <stdbool.h>
#include "game.h"






Dungeon generate_dungeon(int rooms_num);
int generate_room(Dungeon *dungeon, int *last_idx, int current_room_idx, Direction door_direction);
Direction get_opposite_direction(Direction door);
void print_room(Room *r);
void print_dungeon(Dungeon dungeon);

void broadcast(const char *message);

#endif
