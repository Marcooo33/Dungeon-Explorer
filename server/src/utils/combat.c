#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include "game.h"
#include "gameUtils.h"
#include "combat.h"

/* =======================================================
 * ARRAY GLOBALI: armi, armature, oggetti
 * =======================================================
 * monster_weapons contiene SOLO le armi dei mostri (Claw, Boss_Claws).
 * Sword e Bow sono stati rimossi perché non usati dai mostri.
 * ======================================================= */
Weapon monster_weapons[] = {
    {"Claw",       12, SHORT_RANGE, monster_attack},   /* [0] usato da mostri normali */
    {"Boss_Claws", 20, SHORT_RANGE, monster_attack}    /* [1] usato dal boss          */
};

Weapon player_weapons[] = {
    {"Sword", 15, SHORT_RANGE, melee_attack},
    {"Bow",   10, LONG_RANGE,  ranged_attack}
};

Armor armors[] = {
    {"Leather",   5},
    {"Chainmail", 10}
};

Item items[] = {
    {"Health_Potion", health_potion_function}
};

Weapon fists = {"Fists", 5, SHORT_RANGE, melee_attack};

/* =======================================================
 * UTILITÀ GRIGLIA
 * ======================================================= */

bool is_tile_occupied_by_player(int x, int y, Player *players, int num_players) {
    for (int i = 0; i < num_players; i++) {
        if (players[i].alive && players[i].x == x && players[i].y == y)
            return true;
    }
    return false;
}

bool is_tile_occupied_by_monster(int x, int y, Monster *monsters, int num_monsters) {
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i].alive && monsters[i].x == x && monsters[i].y == y)
            return true;
    }
    return false;
}

/* =======================================================
 * FUNZIONI DI ATTACCO
 * ======================================================= */

void melee_attack(void *attacker, void *target) {
    Player *p = (Player *)attacker;
    Monster *m = (Monster *)target;

    if (!p->weapon) return;

    int dist = distance(p->x, p->y, m->x, m->y);
    if (dist > p->weapon->range) {
        printf("Bersaglio fuori range!\n");
        return;
    }

    int defense = m->armor && m->armor->defense ? m->armor->defense : 0;
    int damage  = p->weapon->damage - defense;
    if (damage < 0) damage = 0;

    m->hp -= damage;
    if (m->hp < 0) m->hp = 0;

    printf("Il Giocatore %d colpisce %s per %d danni (HP nemico: %d)\n",
           p->id, m->name, damage, m->hp);
}

void ranged_attack(void *attacker, void *target) {
    Player *p = (Player *)attacker;
    Monster *m = (Monster *)target;

    if (!p->weapon) return;

    int dist = distance(p->x, p->y, m->x, m->y);
    if (dist > p->weapon->range) {
        printf("Troppo lontano per colpire!\n");
        return;
    }

    int damage = p->weapon->damage;
    m->hp -= damage;
    if (m->hp < 0) m->hp = 0;

    printf("Il Giocatore %d attacca a distanza %s per %d danni (HP: %d)\n",
           p->id, m->name, damage, m->hp);
}

void monster_attack(void *attacker, void *target) {
    Monster *m = (Monster *)attacker;
    Player  *p = (Player *)target;

    if (!m->weapon) return;

    int dist = distance(m->x, m->y, p->x, p->y);
    if (dist > m->weapon->range) {
        printf("fuori range!\n");
        return;
    }

    int defense = p->armor ? p->armor->defense : 0;
    int damage  = m->weapon->damage - defense;
    if (damage < 0) damage = 0;

    p->hp -= damage;
    if (p->hp < 0) p->hp = 0;

    printf("%s attacca il Giocatore %d per %d danni (HP: %d)\n",
           m->name, p->id, damage, p->hp);
}

void boss_aoe_attack(Monster *boss, Player *players, int num_players) {
    broadcast("\nMESSAGE Il boss usa ATTACCO AD AREA!\n");

    for (int i = 0; i < num_players; i++) {
        if (players[i].hp <= 0) continue;

        int dist        = distance(boss->x, boss->y, players[i].x, players[i].y);
        int base_damage = boss->weapon->damage;
        int calculated_damage = 0;

        if (dist <= 50) {
            calculated_damage = base_damage;
        } else if (dist <= 100) {
            calculated_damage = (int)(base_damage * 0.6);
        } else if (dist <= 150) {
            calculated_damage = (int)(base_damage * 0.3);
        } else {
            printf("Il Giocatore %d e' abbastanza lontano da schivare l'onda d'urto!\n", players[i].id);
            continue;
        }

        int defense = players[i].armor ? players[i].armor->defense : 0;
        calculated_damage -= defense;
        if (calculated_damage < 0) calculated_damage = 0;

        players[i].hp -= calculated_damage;
        if (players[i].hp < 0) players[i].hp = 0;

        printf("Il Giocatore %d (a %dpx di distanza) subisce %d danni (HP: %d)\n",
               players[i].id, dist, calculated_damage, players[i].hp);
    }
}

/* =======================================================
 * AZIONI GIOCATORE
 * ======================================================= */

void move_player(Player *p, int x, int y) {
    p->x = x;
    p->y = y;
    printf("Il Giocatore %d si muove a (%d, %d)\n", p->id, p->x, p->y);
}

void health_potion_function(Player *p) {
    int heal = 30;
    p->hp += heal;
    if (p->hp > 100) p->hp = 100;
    printf("Il Giocatore %d recupera %d HP (HP attuali: %d)\n", p->id, heal, p->hp);
}

void use_item(Player *p) {
    if (!p->item) {
        printf("[DEBUG] Il giocatore %d non ha oggetti da usare!\n", p->id);
        char no_item_msg[64];
        sprintf(no_item_msg, "MESSAGE Non hai oggetti da usare!\n");
        send(p->socket_fd, no_item_msg, strlen(no_item_msg), MSG_NOSIGNAL);
        return;
    }

    const char *item_name = p->item->name;
    printf("Il Giocatore %d usa %s\n", p->id, item_name);

    p->item->use(p);
    p->item = NULL;

    char used_msg[128];
    sprintf(used_msg, "MESSAGE Hai usato %s! +15 HP. L'oggetto è stato rimosso dall'inventario.\n", item_name);
    send(p->socket_fd, used_msg, strlen(used_msg), MSG_NOSIGNAL);

    broadcast_player_info(p);
}

/* =======================================================
 * TURNI GIOCATORI
 * ======================================================= */

void player_turn(Player *p, Monster *monsters, int num_monsters) {
    if (!p->connected) return;

    ssize_t s = send(p->socket_fd, "MAKE_TURN_DECISION\n", strlen("MAKE_TURN_DECISION\n"), MSG_NOSIGNAL);
    if (s <= 0) {
        printf("[GAME] Player %d disconnesso all'invio di MAKE_TURN_DECISION\n", p->id);
        mark_player_disconnected(p);
        return;
    }

    struct pollfd pfd = { p->socket_fd, POLLIN, 0 };
    int r = poll(&pfd, 1, 120000); // 120 secondi

    if (r <= 0 || (pfd.revents & (POLLHUP | POLLERR))) {
        printf("[GAME] Player %d disconnesso o timeout durante MAKE_TURN_DECISION\n", p->id);
        mark_player_disconnected(p);
        return;
    }

    char decision_buffer[128] = {0};
    int bytes = recv(p->socket_fd, decision_buffer, sizeof(decision_buffer) - 1, 0);
    if (bytes <= 0) {
        printf("[GAME] Player %d disconnesso (recv fallito nel turno)\n", p->id);
        mark_player_disconnected(p);
        return;
    }

    decision_buffer[strcspn(decision_buffer, "\r\n")] = 0;

    if (strncmp(decision_buffer, "SEND_DECISION ", 14) != 0) {
        printf("[DEBUG] Decisione non valida: %s\n", decision_buffer);
        return;
    }

    char *action_str = decision_buffer + 14;

    if (strncmp(action_str, "MOVE ", 5) == 0) {
        int x, y;
        if (sscanf(action_str + 5, "%d %d", &x, &y) == 2) {
            if (!is_tile_occupied_by_monster(x, y, monsters, num_monsters))
                move_player(p, x, y);
        } else {
            printf("[DEBUG] MOVE non valido: %s\n", action_str);
        }

    } else if (strncmp(action_str, "ATTACK ", 7) == 0) {
        int target_id;
        if (sscanf(action_str + 7, "%d", &target_id) == 1) {
            if (target_id < 0 || target_id >= num_monsters) {
                printf("[DEBUG] ATTACK target_id fuori range: %d\n", target_id);
                return;
            }
            Monster *target = &monsters[target_id];
            if (!target->alive) {
                printf("[DEBUG] Mostro %d già morto\n", target_id);
                return;
            }
            if (p->weapon && p->weapon->attack) {
                p->weapon->attack(p, target);
            } else {
                printf("[DEBUG] Giocatore %d non ha un'arma valida\n", p->id);
            }
        } else {
            printf("[DEBUG] ATTACK non valido: %s\n", action_str);
        }

    } else if (strcmp(action_str, "USE_ITEM") == 0) {
        use_item(p);

    } else {
        printf("[DEBUG] Azione non riconosciuta: %s\n", action_str);
    }
}

/* =======================================================
 * TURNI MOSTRI
 * ======================================================= */

void monster_turn(Monster *m, Player *players, int num_players) {
    printf("\nTurno del mostro %s\n", m->name);

    int target = rand() % num_players;
    int dist   = distance(m->x, m->y, players[target].x, players[target].y);

    if (dist > m->weapon->range) {
        // Avvicinamento intelligente verso il target
        int next_x = m->x;
        int next_y = m->y;

        if (players[target].x > m->x)      next_x += 50;
        else if (players[target].x < m->x) next_x -= 50;

        if (players[target].y > m->y)      next_y += 50;
        else if (players[target].y < m->y) next_y -= 50;

        // Applica limiti mappa
        if (next_x < 0)   next_x = 0;
        if (next_x > 450) next_x = 450;
        if (next_y < 0)   next_y = 0;
        if (next_y > 200) next_y = 200;

        // Si muove solo se la casella è libera
        if (!is_tile_occupied_by_player(next_x, next_y, players, num_players)) {
            m->x = next_x;
            m->y = next_y;
            printf("%s si avvicina a (%d, %d)\n", m->name, m->x, m->y);
        }
    } else {
        if (m->weapon && m->weapon->attack)
            m->weapon->attack(m, &players[target]);
    }
}

void boss_turn(Monster *boss, Player *players, int num_players) {
    if (boss->hp <= 0) return;

    printf("\nTurno del BOSS %s\n", boss->name);
    char msg[128];

    // Trova il giocatore vivo più vicino
    int closest_player_idx = -1;
    int min_dist = 999999;

    for (int i = 0; i < num_players; i++) {
        if (players[i].hp > 0 && players[i].alive) {
            int d = distance(boss->x, boss->y, players[i].x, players[i].y);
            if (d < min_dist) {
                min_dist = d;
                closest_player_idx = i;
            }
        }
    }

    if (closest_player_idx == -1) return;

    if (min_dist <= boss->weapon->range) {
        // Attacco ravvicinato
        sprintf(msg, "MESSAGE Il Boss sferra un attacco ravvicinato contro il Giocatore %d!\n",
                players[closest_player_idx].id);
        broadcast(msg);
        if (boss->weapon && boss->weapon->attack)
            boss->weapon->attack(boss, &players[closest_player_idx]);

    } else if (min_dist <= 150) {
        // Attacco ad area
        broadcast("MESSAGE Il Boss sbatte i pugni a terra: ONDA D'URTO AD AREA!\n");
        boss_aoe_attack(boss, players, num_players);

    } else {
        // Avvicinamento verso il giocatore più vicino
        if (players[closest_player_idx].x > boss->x)      boss->x += 50;
        else if (players[closest_player_idx].x < boss->x) boss->x -= 50;

        if (players[closest_player_idx].y > boss->y)      boss->y += 50;
        else if (players[closest_player_idx].y < boss->y) boss->y -= 50;

        printf("%s si muove verso il giocatore %d\n", boss->name, players[closest_player_idx].id);
    }
}
