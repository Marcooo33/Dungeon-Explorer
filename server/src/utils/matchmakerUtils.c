#include <stdlib.h>
#include <string.h>

#include "matchmaker.h"
#include "matchmakerUtils.h"

void generate_code(char *code) {
    const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (int i = 0; i < 6; i++) {
        code[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    code[6] = '\0';
}

int find_game_by_code(const char *code) {
    for (int i = 0; i < game_count; i++) {
        if (strcmp(games[i].code, code) == 0) return i;
    }
    return -1;
}