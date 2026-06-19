#ifndef ENCOUNTER_H
#define ENCOUNTER_H

#include "game.h"

/* -------------------------------------------------------
 * Encounter tesoro
 * ------------------------------------------------------- */
bool treasure_encounter1(Player *players, int num_players);
bool treasure_encounter2(Player *players, int num_players);
bool treasure_encounter3(Player *players, int num_players);

/* -------------------------------------------------------
 * Encounter trappola
 * ------------------------------------------------------- */
bool trap_encounter(Player *players, int num_players);

/* -------------------------------------------------------
 * Encounter combattimento
 * ------------------------------------------------------- */
bool combat_encounter(Player *players, int num_players);

/* -------------------------------------------------------
 * Encounter boss
 * ------------------------------------------------------- */
bool boss_encounter(Player *players, int num_players);

#endif
