#ifndef MATCHMAKER_UTILS_H
#define MATCHMAKER_UTILS_H

#include <pthread.h>

extern pthread_mutex_t games_mutex;


int find_game_by_code(const char *code);
void generate_code(char *code);
void handle_host_loop(int game_idx);

#endif