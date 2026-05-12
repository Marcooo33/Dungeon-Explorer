#ifndef GAME_UTILS_H
#define GAME_UTILS_H

#include <stdbool.h>
#include "game.h"

void build_room_message(Room *room, char *buffer, size_t size);
const char* direction_to_string(Direction dir);
Direction string_to_direction(const char *str);
bool is_valid_direction(Direction d, Room *room);
Dungeon generate_dungeon(int rooms_num);
int generate_room(Dungeon *dungeon, int *last_idx, int current_room_idx, Direction door_direction);
Direction get_opposite_direction(Direction door);
void print_room(Room *r);
void print_dungeon(Dungeon dungeon);
const char* direction_to_string(Direction d);
int distance(int x1, int y1, int x2, int y2);


void broadcast(const char *message);
void broadcast_player_info(Player *player);

#endif
