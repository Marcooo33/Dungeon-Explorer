#ifndef COMBAT_H
#define COMBAT_H

#include "game.h"

/* -------------------------------------------------------
 * Array globali di dati (definiti in combat.c)
 * ------------------------------------------------------- */
extern Weapon monster_weapons[];   // {Claw, Boss_Claws}
extern Weapon player_weapons[];    // {Sword, Bow}
extern Armor  armors[];            // {Leather, Chainmail}
extern Item   items[];             // {Health_Potion}
extern Weapon fists;

/* -------------------------------------------------------
 * Funzioni di attacco
 * ------------------------------------------------------- */
void melee_attack(void *attacker, void *target);
void ranged_attack(void *attacker, void *target);
void monster_attack(void *attacker, void *target);
void boss_aoe_attack(Monster *boss, Player *players, int num_players);

/* -------------------------------------------------------
 * Turni
 * ------------------------------------------------------- */
void player_turn(Player *p, Monster *monsters, int num_monsters);
void monster_turn(Monster *m, Player *players, int num_players);
void boss_turn(Monster *boss, Player *players, int num_players);

/* -------------------------------------------------------
 * Azioni giocatore
 * ------------------------------------------------------- */
void move_player(Player *p, int x, int y);
void use_item(Player *p);
void health_potion_function(Player *player);

/* -------------------------------------------------------
 * Utilità griglia
 * ------------------------------------------------------- */
bool is_tile_occupied_by_player(int x, int y, Player *players, int num_players);
bool is_tile_occupied_by_monster(int x, int y, Monster *monsters, int num_monsters);

#endif
