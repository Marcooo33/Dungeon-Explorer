#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include "game.h"
#include "gameUtils.h"
#include "combat.h"
#include "encounter.h"

/* =======================================================
 * ENCOUNTER TESORO
 * ======================================================= */

bool treasure_encounter1(Player *players, int num_players) {
    broadcast("MESSAGE Congratulazioni! La fortuna vi arride, trovate tutti un ricco tesoro!\n");

    char treasure_found_message[128];
    for (int i = 0; i < num_players; i++) {
        int gold_found = rand() % 50 + 10; // tra 10 e 59
        players[i].gold += gold_found;
        sprintf(treasure_found_message, "MESSAGE Oro trovato: %d\n", gold_found);
        send(players[i].socket_fd, treasure_found_message, strlen(treasure_found_message), MSG_NOSIGNAL);
    }

    broadcast_all_players_info();
    return true;
}

bool treasure_encounter2(Player *players, int num_players) {
    (void)players;
    (void)num_players;
    broadcast("MESSAGE Vi si palesa davanti un tesoro! Lo aprite e... capite che la fortuna ha arriso altri esploratori!\n");
    return true;
}

/* Chiede ad ogni giocatore se vuole prendere l'equipaggiamento trovato (MAKE_INVENTORY_DECISION) */
bool treasure_encounter3(Player *players, int num_players) {
    broadcast("MESSAGE Trovate un forziere con dell'equipaggiamento! Forse un regalo, voluto o meno, da parte degli esploratori prima di voi!\n");

    char treasure_found_message[128];
    char decision_buffer[128];

    for (int i = 0; i < num_players; i++) {
        if (!players[i].connected) continue;

        int reward_type = rand() % 3;

        if (reward_type == 0) {
            /* ---- Arma ---- */
            int weapon_idx       = rand() % 2;
            Weapon *found_weapon = &player_weapons[weapon_idx];

            sprintf(treasure_found_message,
                    "MAKE_INVENTORY_DECISION Hai trovato un'arma: %s atk: %d range: %d\n",
                    found_weapon->name, found_weapon->damage, found_weapon->range);
            ssize_t s = send(players[i].socket_fd, treasure_found_message,
                             strlen(treasure_found_message), MSG_NOSIGNAL);
            if (s <= 0) { mark_player_disconnected(&players[i]); continue; }

            struct pollfd pfd = { players[i].socket_fd, POLLIN, 0 };
            int r = poll(&pfd, 1, 10000);
            if (r <= 0 || (pfd.revents & (POLLHUP | POLLERR))) { mark_player_disconnected(&players[i]); continue; }

            memset(decision_buffer, 0, sizeof(decision_buffer));
            int bytes = recv(players[i].socket_fd, decision_buffer, sizeof(decision_buffer) - 1, 0);
            if (bytes <= 0) { mark_player_disconnected(&players[i]); continue; }

            if (strncmp(decision_buffer, "SEND_DECISION T", 15) == 0) {
                printf("[DEBUG] Trovata arma: %s, danno: %d, range: %d\n",
                       found_weapon->name, found_weapon->damage, found_weapon->range);
                players[i].weapon = found_weapon;
            }
            broadcast_player_info(&players[i]);

        } else if (reward_type == 1) {
            /* ---- Armatura ---- */
            int armor_idx      = rand() % 2;
            Armor *found_armor = &armors[armor_idx];

            sprintf(treasure_found_message,
                    "MAKE_INVENTORY_DECISION Hai trovato un'armatura: %s def: %d\n",
                    found_armor->name, found_armor->defense);
            ssize_t s = send(players[i].socket_fd, treasure_found_message,
                             strlen(treasure_found_message), MSG_NOSIGNAL);
            if (s <= 0) { mark_player_disconnected(&players[i]); continue; }

            struct pollfd pfd = { players[i].socket_fd, POLLIN, 0 };
            int r = poll(&pfd, 1, 10000);
            if (r <= 0 || (pfd.revents & (POLLHUP | POLLERR))) { mark_player_disconnected(&players[i]); continue; }

            memset(decision_buffer, 0, sizeof(decision_buffer));
            int bytes = recv(players[i].socket_fd, decision_buffer, sizeof(decision_buffer) - 1, 0);
            if (bytes <= 0) { mark_player_disconnected(&players[i]); continue; }

            if (strncmp(decision_buffer, "SEND_DECISION T", 15) == 0) {
                printf("[DEBUG] Trovata armatura: %s, difesa: %d\n",
                       found_armor->name, found_armor->defense);
                players[i].armor = found_armor;
            }
            broadcast_player_info(&players[i]);

        } else {
            /* ---- Oggetto ---- */
            int item_idx     = rand() % 1;
            Item *found_item = &items[item_idx];

            sprintf(treasure_found_message,
                    "MAKE_INVENTORY_DECISION Hai trovato un oggetto: %s\n",
                    found_item->name);
            ssize_t s = send(players[i].socket_fd, treasure_found_message,
                             strlen(treasure_found_message), MSG_NOSIGNAL);
            if (s <= 0) { mark_player_disconnected(&players[i]); continue; }

            struct pollfd pfd = { players[i].socket_fd, POLLIN, 0 };
            int r = poll(&pfd, 1, 10000);
            if (r <= 0 || (pfd.revents & (POLLHUP | POLLERR))) { mark_player_disconnected(&players[i]); continue; }

            memset(decision_buffer, 0, sizeof(decision_buffer));
            int bytes = recv(players[i].socket_fd, decision_buffer, sizeof(decision_buffer) - 1, 0);
            if (bytes <= 0) { mark_player_disconnected(&players[i]); continue; }

            if (strncmp(decision_buffer, "SEND_DECISION T", 15) == 0) {
                printf("[DEBUG] Trovato oggetto: %s\n", found_item->name);
                players[i].item = found_item;
            }
            broadcast_player_info(&players[i]);
        }
    }
    return true;
}

/* =======================================================
 * ENCOUNTER TRAPPOLA
 * ======================================================= */

bool trap_encounter(Player *players, int num_players) {
    broadcast("MESSAGE Attenzione! Avete attivato una trappola!\n");

    for (int i = 0; i < num_players; i++) {
        int damage = rand() % 20 + 5; // tra 5 e 24
        players[i].hp -= damage;

        if (players[i].hp <= 0) {
            players[i].alive = false;
            players[i].hp    = 0;
            broadcast("MESSAGE Un giocatore è morto a causa della trappola!\n");
        }
    }

    broadcast_all_players_info();
    return !(are_all_players_dead(players, num_players));
}

/* =======================================================
 * ENCOUNTER COMBATTIMENTO
 * ======================================================= */

bool combat_encounter(Player *players, int num_players) {
    broadcast("MESSAGE Appena entrati nella stanza vi trovate davanti dei pericolosi mostri, l'unica alternativa è combatterli!\n");

    int num_monsters = 2;
    Monster monsters[2] = {
        {0, "Skeleton", 10, true, 300,  0, &monster_weapons[0], NULL},  /* Claw */
        {1, "Orc",      10, true, 300, 50, &monster_weapons[0], NULL}   /* Claw */
    };

    for (int i = 0; i < num_monsters; i++)
        broadcast_monster_info(&monsters[i]);

    int turn = 0;
    char message[256];

    while (1) {
        sprintf(message, "MESSAGE === ROUND %d ===\n", turn++);
        broadcast(message);

        /* --- FASE 1: TURNI GIOCATORI --- */
        for (int i = 0; i < num_players; i++) {
            if (players[i].alive && players[i].connected) {
                player_turn(&players[i], monsters, num_monsters);
                broadcast_player_info(&players[i]);

                /* Risoluzione immediata morte mostri */
                for (int j = 0; j < num_monsters; j++) {
                    if (monsters[j].alive && monsters[j].hp <= 0) {
                        monsters[j].alive = false;
                        char msg[128];
                        sprintf(msg, "MESSAGE Il mostro %s e' morto!\n", monsters[j].name);
                        broadcast(msg);
                    }
                    broadcast_monster_info(&monsters[j]);
                }

                if (are_all_monsters_dead(monsters, num_monsters)) break;
            }
        }

        if (are_all_players_disconnected()) {
            printf("[GAME %s] Tutti i giocatori si sono disconnessi durante il combattimento.\n", game_code);
            return false;
        }

        /* --- FASE 2: CONTROLLO VITTORIA --- */
        if (are_all_monsters_dead(monsters, num_monsters)) {
            broadcast("MESSAGE L'ultimo mostro finisce a terra e tirate tutti un sospiro di sollievo... senza abbassare la guardia\n");
            reset_players_position(players);
            broadcast_all_players_info();
            return true;
        }

        /* --- FASE 3: TURNI MOSTRI --- */
        for (int i = 0; i < num_monsters; i++) {
            if (monsters[i].alive) {
                sleep(1);
                monster_turn(&monsters[i], players, num_players);
                broadcast_monster_info(&monsters[i]);

                /* Risoluzione immediata morte giocatori */
                for (int j = 0; j < num_players; j++) {
                    if (players[j].alive && players[j].hp <= 0) {
                        players[j].alive = false;
                        char msg[64];
                        sprintf(msg, "MESSAGE Il Giocatore %d e' morto!\n", players[j].id);
                        broadcast(msg);
                    }
                    broadcast_player_info(&players[j]);
                }

                if (are_all_players_dead(players, num_players)) break;
            }
        }

        /* --- FASE 4: CONTROLLO SCONFITTA --- */
        if (are_all_players_dead(players, num_players)) {
            broadcast("MESSAGE I mostri vincono!\n");
            return false;
        }
    }

    /* Fallback di sicurezza */
    reset_players_position(players);
    broadcast_all_players_info();
    return true;
}

/* =======================================================
 * ENCOUNTER BOSS
 * ======================================================= */

bool boss_encounter(Player *players, int num_players) {
    broadcast("MESSAGE Entrate in una stanza più grande delle altre, al centro vedete un enorme mostro che vi fissa con occhi pieni di odio... è il boss del dungeon!\n");

    Monster boss = {0, "Knight", 10, true, 200, 50, &monster_weapons[1], NULL}; /* Boss_Claws */
    broadcast_monster_info(&boss);

    int turn = 0;
    char message[256];

    while (1) {
        sprintf(message, "MESSAGE === ROUND %d ===\n", turn++);
        broadcast(message);

        /* --- FASE 1: TURNI GIOCATORI --- */
        for (int i = 0; i < num_players; i++) {
            if (players[i].alive && players[i].connected) {
                player_turn(&players[i], &boss, 1);
                broadcast_player_info(&players[i]);

                /* Risoluzione immediata morte boss */
                if (boss.alive && boss.hp <= 0) {
                    boss.alive = false;
                    char msg[128];
                    sprintf(msg, "MESSAGE Il boss %s è stato sconfitto!\n", boss.name);
                    broadcast(msg);
                }
                broadcast_monster_info(&boss);

                if (!boss.alive) break;
            }
        }

        if (are_all_players_disconnected()) {
            printf("[GAME %s] Tutti i giocatori si sono disconnessi durante lo scontro col boss.\n", game_code);
            return false;
        }

        /* --- FASE 2: CONTROLLO VITTORIA --- */
        if (!boss.alive) {
            broadcast("MESSAGE Il boss è stato sconfitto! Avete completato il dungeon!\n");
            reset_players_position(players);
            broadcast_all_players_info();
            return true;
        }

        /* --- FASE 3: TURNO BOSS --- */
        if (boss.alive) {
            sleep(1);
            boss_turn(&boss, players, num_players);
            broadcast_monster_info(&boss);

            /* Risoluzione immediata morte giocatori */
            for (int j = 0; j < num_players; j++) {
                if (players[j].alive && players[j].hp <= 0) {
                    players[j].alive = false;
                    char msg[64];
                    sprintf(msg, "MESSAGE Il Giocatore %d e' morto!\n", players[j].id);
                    broadcast(msg);
                }
                broadcast_player_info(&players[j]);
            }
        }

        /* --- FASE 4: CONTROLLO SCONFITTA --- */
        if (are_all_players_dead(players, num_players)) {
            broadcast("MESSAGE Il boss ha annientato il gruppo! I mostri vincono!\n");
            return false;
        }
    }

    /* Fallback di sicurezza */
    reset_players_position(players);
    broadcast_all_players_info();
    return true;
}
