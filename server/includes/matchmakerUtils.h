#ifndef MATCHMAKER_UTILS_H
#define MATCHMAKER_UTILS_H

#include <pthread.h>

extern pthread_mutex_t games_mutex;

void handle_host_loop(int game_idx);
void handle_join(int client_socket_fd);
void generate_code(char *code);
int find_game_by_code(const char *code);
int find_free_player_slot(Game* game);
bool wait_for_host_decision(Player* new_player);
void print_info_game(int game_idx);
void start_game(int game_idx);

#endif