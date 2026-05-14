#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <stdbool.h>
#include "game.h"
#include "gameUtils.h"

Player players[MAX_PLAYERS];
int connected_count = 0;
char *game_code = NULL;
Dungeon dungeon;

Weapon monster_weapons[] = {
    {"Sword", 15, SHORT_RANGE, monster_attack},
    {"Bow", 10, LONG_RANGE, monster_attack},
    {"Claw", 12, SHORT_RANGE, monster_attack},
    {"Boss_Claws", 20, SHORT_RANGE, monster_attack}
};

Weapon player_weapons[] = {
    {"Sword", 15, SHORT_RANGE, melee_attack},
    {"Bow", 10, LONG_RANGE, ranged_attack}
};

Armor armors[] = {
    {"Leather", 5},
    {"Chainmail", 10}
};

Item items[] = {
    {"Health_Potion", health_potion_function}
};


void init_players() {
    for (int i = 0; i < connected_count; i++) {
        players[i].hp = 100;
        players[i].alive = true;
        players[i].x = 0; // Posizione iniziale X
        players[i].y = i * 50; // Posizione iniziale Y
        players[i].gold = 0;
        players[i].weapon = &(Weapon){"Fists", 5, SHORT_RANGE, melee_attack}; // Fists di default
        players[i].armor = NULL;
        players[i].item = NULL;
    }
}

int main(int argc, char *argv[]) {
    if(argc < 3){
        printf("Uso: %s <game_code> <fds>\n", argv[0]);
        return -1;
    }

    game_code = argv[1];
    printf("[GAME %s] Partita creata\n", game_code);

    connected_count = 0;
    
    // Variabili per mantenere lo stato (necessarie per strtok_r)
    char *saveptr_sockets;
    char *saveptr_ids;

    // Estraiamo il primo token da entrambe le stringhe
    char *socket_token = strtok_r(argv[2], " ", &saveptr_sockets);
    char *id_token = strtok_r(argv[3], " ", &saveptr_ids);

    // Cicliamo finché abbiamo elementi in entrambe le stringhe
    while (socket_token != NULL && id_token != NULL) {
        // Popoliamo la struct del giocatore corrente
        players[connected_count].socket_fd = atoi(socket_token);
        players[connected_count].id = atoi(id_token);

        connected_count++;

        // Passiamo al token successivo per entrambe le stringhe
        socket_token = strtok_r(NULL, " ", &saveptr_sockets);
        id_token = strtok_r(NULL, " ", &saveptr_ids);
    }


    printf("[GAME %s] Ricevuti %d socket\n", game_code, connected_count);
    broadcast("START_GAME\n");
    sleep(3);

    // Inviamo a ciascun client il proprio ID
    char specific_client_msg[64];
    for (int i = 0; i < connected_count; i++) {
        sprintf(specific_client_msg, "SET_MY_ID %d\n", players[i].id);
        send(players[i].socket_fd, specific_client_msg, strlen(specific_client_msg), 0);
    }

    init_players();

    char player_info_message[128];
    for (int i = 0; i < connected_count; i++) {
        broadcast_player_info(&players[i]);
    }


    // Debug: stampa info giocatori
    printf("[DEBUG] Connessi %d giocatori\n", connected_count);
    printf("[GAME %s] Player Info:\n", game_code);
    for (int i = 0; i < connected_count; i++) {
        printf("Player %d: Socket FD=%d, HP=%d, Alive=%s, Position=(%d,%d), Gold=%d\n",
        players[i].id, players[i].socket_fd, players[i].hp, players[i].alive ? "Yes" : "No", players[i].x, players[i].y, players[i].gold);
    }


    dungeon = generate_dungeon(5);
    int current_room_idx = 0; // Iniziamo nella prima stanza del dungeon
    int next_room_idx = -1; 
    char room_info_msg[256];
    Room *current_room = NULL;

    // game loop start
    while (true){
        current_room = &dungeon.rooms[current_room_idx];

        // 1) far svolgere encounter stanza
        if (current_room->completed == false){
            bool room_cleared = current_room->encounter(players, connected_count);
            if (room_cleared)
                current_room->completed = true;
            else {
                printf("[GAME %s] Tutti i giocatori sono morti nella stanza %d. Game Over.\n", game_code, current_room_idx);
                broadcast("GAME_OVER\n");
                break;
            }
        }
        
        // 2) mandare una sorta di messaggio make decision
        Direction door_direction;
        Room *adjacent_room;

        sprintf(room_info_msg, "MAKE_ROOM_DECISION ");
        for(int door_number=0;door_number < current_room->doors_num;door_number++){
            door_direction = current_room->doors[door_number];
            adjacent_room = &dungeon.rooms[current_room->connected_rooms[door_direction]];
            sprintf(room_info_msg + strlen(room_info_msg), "%s:%s:%s ", direction_to_string(door_direction), adjacent_room->type, adjacent_room->completed ? "completed" : "not_completed");
        }
        sprintf(room_info_msg + strlen(room_info_msg), "\n"); // Aggiungi newline alla fine del messaggio
        broadcast(room_info_msg);
        

        // 3) ricevere la decisione
        int votes[4] = {0}; // NORTH, SOUTH, EAST, WEST
        int received = 0;
        bool has_voted[MAX_PLAYERS] = {false};

        struct pollfd fds[MAX_PLAYERS];

        // Inizializzazione
        for (int i = 0; i < connected_count; i++) {
            fds[i].fd = players[i].socket_fd;
            fds[i].events = POLLIN;
            fds[i].revents = 0;
        }

        int timeout_ms = 20000; // 20 secondi
        int elapsed = 0;
        int step = 1000; // controlliamo ogni 1 secondo

        while (received < connected_count && elapsed < timeout_ms) {
            int activity = poll(fds, connected_count, step);

            if (activity < 0) {
                perror("poll error");
                break;
            }

            if (activity == 0) {
                // nessun evento → è passato 1 secondo
                elapsed += step;
                continue;
            }

            for (int i = 0; i < connected_count; i++) {
                if ((fds[i].revents & POLLIN) && !has_voted[i]) {
                    char buffer[128] = {0};
                    int bytes = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);

                    if (bytes <= 0) continue;

                    if (strncmp(buffer, "SEND_DECISION ", 14) == 0) {
                        char *dir_str = buffer + 14;
                        dir_str[strcspn(dir_str, "\r\n")] = 0;

                        Direction d = string_to_direction(dir_str);

                        votes[d]++;

                        has_voted[i] = true;
                        received++;

                        printf("[GAME %s] Player %d voted: %s\n",
                            game_code, players[i].id, dir_str);
                    }
                }
            }
        }

        // Debug timeout
        if (received < connected_count) {
            printf("[GAME %s] Timeout votazione (%d/%d ricevuti)\n",
                game_code, received, connected_count);
        }

        int max_votes = 0;

        // trova massimo
        for (int i = 0; i < 4; i++) {
            if (votes[i] > max_votes) {
                max_votes = votes[i];
            }
        }


        // raccogli candidati validi
        Direction candidates[4];
        int candidate_count = 0;

        for (int i = 0; i < 4; i++) {
            if (votes[i] == max_votes && max_votes > 0 &&
                is_valid_direction(i, current_room)) {
                candidates[candidate_count++] = i;
            }
        }

        Direction chosen_direction;

        // scelta finale
        if (candidate_count == 0) {
            printf("[GAME %s] Nessun voto valido, fallback casuale\n", game_code);

            // fallback su porte valide della stanza
            int r = rand() % current_room->doors_num;
            chosen_direction = current_room->doors[r];
        }
        else if (candidate_count == 1) {
            chosen_direction = candidates[0];
        }
        else {
            int r = rand() % candidate_count;
            chosen_direction = candidates[r];

            printf("[GAME %s] Pareggio tra %d direzioni, scelta: %s\n",
                game_code,
                candidate_count,
                direction_to_string(chosen_direction));
        }

        // debug finale
        printf("[GAME %s] Decisione finale: %s\n",
            game_code,
            direction_to_string(chosen_direction));

        // aggiorna stanza
        next_room_idx = current_room->connected_rooms[chosen_direction];
        
        
        // 4) mandare la stanza da renderizzare
        char room_msg[64];
        build_room_message(&dungeon.rooms[next_room_idx], room_msg, sizeof(room_msg));

        // invio a tutti i client
        broadcast(room_msg);

        // debug
        printf("[GAME %s] Sent: %s", game_code, room_msg);
        
        current_room_idx = next_room_idx;
    }
    
}

bool treasure_encounter1(Player *players, int num_players){
    broadcast("MESSAGE Congratulazioni! La fortuna vi arride, trovate tutti un ricco tesoro!\n");
    char treasure_found_message[128]; 
    for (int i = 0; i < num_players; i++) {
        int gold_found = rand() % 50 + 10; // tra 10 e 59
        players[i].gold += gold_found;
        sprintf(treasure_found_message, "MESSAGE Congratulazioni! La fortuna vi arride. Avete trovato un ricco tesoro! Oro trovato: %d\n", gold_found);
        send(players[i].socket_fd, treasure_found_message, strlen(treasure_found_message), 0);
    }
    
    char player_info_message[128];
    for (int i = 0; i < connected_count; i++) {
        broadcast_player_info(&players[i]);
    }
    return true;
}

bool treasure_encounter2(Player *players, int num_players){
    broadcast("MESSAGE Vi si palesa davanti un tesoro! Lo aprite e... capite che la fortuna ha arriso altri esploratori!\n");
    return true;
}

//MAKE_INVENTORY_DECISION 
bool treasure_encounter3(Player *players, int num_players){
    broadcast("MESSAGE Trovate un forziere con dell'equipaggiamento! Forse un regalo, voluto o meno, da parte degli esploratori prima di voi!\n");
    char treasure_found_message[128];
    char decision_buffer[128];

    for (int i = 0; i < num_players; i++) {
        int reward_type = rand() % 3;
        if (reward_type == 0) {
            Weapon found_weapon = player_weapons[rand() % 2];

            sprintf(treasure_found_message, "MAKE_INVENTORY_DECISION Hai trovato un'arma: %s atk: %d range: %d\n", found_weapon.name, found_weapon.damage, found_weapon.range);
            send(players[i].socket_fd, treasure_found_message, strlen(treasure_found_message), 0);
            recv(players[i].socket_fd, decision_buffer, sizeof(decision_buffer) - 1, 0); // Aspettiamo la decisione del client

            if(strncmp(decision_buffer, "SEND_DECISION T", 15) == 0) {
                printf("[DEBUG] Trovata arma: %s, danno: %d, range: %d\n", found_weapon.name, found_weapon.damage, found_weapon.range);
                players[i].weapon = &found_weapon;
            }

            broadcast_player_info(&players[i]);


        } else if (reward_type == 1) {
            Armor found_armor = armors[rand() % 2];
            
            sprintf(treasure_found_message, "MAKE_INVENTORY_DECISION Hai trovato un'armatura: %s def: %d\n", found_armor.name, found_armor.defense);
            send(players[i].socket_fd, treasure_found_message, strlen(treasure_found_message), 0);
            recv(players[i].socket_fd, decision_buffer, sizeof(decision_buffer) - 1, 0); // Aspettiamo la decisione del client

            if (strncmp(decision_buffer, "SEND_DECISION T", 15) == 0)
            {
                printf("[DEBUG] Trovata armatura: %s, difesa: %d\n", found_armor.name, found_armor.defense);
                players[i].armor = &found_armor;
            }

            broadcast_player_info(&players[i]);

        } else {
            Item found_item = items[rand() % 1];
            sprintf(treasure_found_message, "MAKE_INVENTORY_DECISION Hai trovato un oggetto: %s\n", found_item.name);
            send(players[i].socket_fd, treasure_found_message, strlen(treasure_found_message), 0);
            recv(players[i].socket_fd, decision_buffer, sizeof(decision_buffer) - 1, 0); // Aspettiamo la decisione del client

            if (strncmp(decision_buffer, "SEND_DECISION T", 15) == 0)
            {
                printf("[DEBUG] Trovato oggetto: %s\n", found_item.name);
                players[i].item = &found_item;
            }

            broadcast_player_info(&players[i]);

        }
    }
    return true;
}


bool trap_encounter(Player *players, int num_players){
    broadcast("MESSAGE Attenzione! Avete attivato una trappola!\n");
    char player_info_message[128];

    for (int i = 0; i < num_players; i++) {
        int damage = rand() % 20 + 5; // tra 5 e 24
        players[i].hp -= damage;

        if (players[i].hp <= 0) {
            players[i].alive = false;
            players[i].hp = 0;
            broadcast("MESSAGE Un giocatore è morto a causa della trappola!\n");
        }
    }

    for (int i = 0; i < connected_count; i++) {
        broadcast_player_info(&players[i]);
    }

    bool room_cleared = !(are_all_players_dead(players, num_players));
    if (room_cleared)
        return false;
    else
        return true;
}

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
    int damage = p->weapon->damage - defense;
    if (damage < 0) damage = 0;

    m->hp -= damage;

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
    }else if (dist >= SHORT_RANGE) {
        printf("Troppo vicino per un attacco a distanza!\n");
        return;
    }

    int damage = p->weapon->damage;
    m->hp -= damage;

    printf("Il Giocatore %d attacca a distanza %s per %d danni (HP: %d)\n",
           p->id, m->name, damage, m->hp);
}

void move_player(Player *p) {
    int dx = (rand() % 3) - 1;
    int dy = (rand() % 3) - 1;

    p->x += dx;
    p->y += dy;

    printf("Il Giocatore %d si muove a (%d, %d)\n", p->id, p->x, p->y);
}

void use_item(Player *p) {
    if (!p->item) {
        printf("Non hai oggetti!\n");
        return;
    }

    p->item->use(p);

    printf("Il Giocatore %d usa %s\n",
           p->id, p->item->name);
}

void health_potion_function(Player *p) {
    int heal = 30; // tra 20 e 49
    p->hp += heal;
    if (p->hp > 100) p->hp = 100;

    printf("Il Giocatore %d recupera %d HP (HP attuali: %d)\n",
           p->id, heal, p->hp);
}

// MAKE_TURN_DECISION
void player_turn(Player *p, Monster *monsters, int num_monsters) {
    send(p->socket_fd, "MAKE_TURN_DECISION\n", strlen("MAKE_TURN_DECISION\n"), 0);
    
    char decision_buffer[128] = {0};
    recv(p->socket_fd, decision_buffer, sizeof(decision_buffer) - 1, 0);

    //vediamo prima se il client ha mandato una decisione valida (inizia per SEND_DECISION), poi vediamo la stringa successiva se è MOVE, ATTACK o USE_ITEM
    if (strncmp(decision_buffer, "SEND_DECISION ", 14) == 0) {

        char *action_str = decision_buffer + 14;
        action_str[strcspn(action_str, "\r\n")] = 0; // Rimuove newline finale

        if (strcmp(action_str, "MOVE") == 0) {
            move_player(p);

        } else if (strcmp(action_str, "ATTACK") == 0) {
            //da vedere come passare il target scelto dal client.
        } else if (strcmp(action_str, "USE_ITEM") == 0) {
            use_item(p);
        } else {
            printf("[DEBUG] Azione non riconosciuta: %s\n", action_str);
            return;
        }
    
    }

}

void monster_attack(void *attacker, void *target) {
    Monster *m = (Monster *)attacker;
    Player *p = (Player *)target;

    if (!m->weapon) return;

    int dist = distance(m->x, m->y, p->x, p->y);

    if (dist > m->weapon->range) {
        printf("fuori range!\n");
        return;
    }

    int defense = p->armor ? p->armor->defense : 0;
    int damage = m->weapon->damage - defense;
    if (damage < 0) damage = 0;

    p->hp -= damage;

    printf("%s attacca il Giocatore %d per %d danni (HP: %d)\n",
           m->name, p->id, damage, p->hp);
}

void monster_turn(Monster *m, Player *players, int num_players) {
    printf("\nTurno del mostro %s\n", m->name);

    int target = rand() % num_players;

    int dist = distance(m->x, m->y, players[target].x, players[target].y);

    if (dist > m->weapon->range) {
        m->x += (players[target].x > m->x) ? 1 : -1;
        m->y += (players[target].y > m->y) ? 1 : -1;

        printf("%s si avvicina\n", m->name);
    } else {
        if (m->weapon && m->weapon->attack) {
            m->weapon->attack(m, &players[target]);
        }
    }
}

bool combat_encounter(Player *players, int num_players) {
    broadcast("MESSAGE Appena entrati nella stanza vi trovate davanti dei pericolosi mostri, l'unica alternativa è combatterli!\n");
    
    int num_monsters = 2;
    Monster monsters[2] = {
        {"Skeleton", 50, true, 300, 0, &monster_weapons[2], NULL},
        {"Orc", 80, true, 300, 50, &monster_weapons[2], NULL}
    };

    //broadcast_monster_info
    for (int i = 0; i < num_monsters; i++) {
        broadcast_monster_info(&monsters[i]);
    }

    int turn = 0;
    char message[256]; 

    while (1) {
        sprintf(message, "=== ROUND %d ===\n", turn++);
        broadcast(message);

        for (int i = 0; i < num_players; i++)
            if (players[i].alive)
                player_turn(&players[i], monsters, num_monsters);

        bool monsters_defeated = are_all_monsters_dead(monsters, num_monsters);
        if (monsters_defeated) {
            broadcast("MESSAGE L'ultimo mostro finisce a terra e tirate tutti un sospiro di sollievo... senza abbassare la guardia\n");
            return true;
        }

        for (int i = 0; i < num_monsters; i++)
            if (monsters[i].alive)
                monster_turn(&monsters[i], players, num_players);

        bool players_defeated = are_all_players_dead(players, num_players);
        if (players_defeated) {
            broadcast("MESSAGE I mostri vincono!\n");
            return false;
        }
    }

    reset_players_position(players);

    for (int i = 0; i < connected_count; i++) {
        broadcast_player_info(&players[i]);
    }

}

void boss_aoe_attack(Monster *boss, Player *players, int num_players) {
    printf("%s usa ATTACCO AD AREA!\n", boss->name);

    for (int i = 0; i < num_players; i++) {
        if (players[i].hp <= 0) continue;

        int damage = boss->weapon->damage / 2;

        int defense = players[i].armor ? players[i].armor->defense : 0;
        damage -= defense;
        if (damage < 0) damage = 0;

        players[i].hp -= damage;

        printf("Il Giocatore %d subisce %d danni (HP: %d)\n",
               players[i].id,
               damage,
               players[i].hp);
    }
}

void boss_turn(Monster *boss, Player *players, int num_players) {
    if (boss->hp <= 0) return;

    printf("\nTurno del BOSS %s\n", boss->name);

    int action = rand() % 3;

    // 0 = movimento, 1 = attacco singolo, 2 = skill
    if (action == 0) {
        int target = rand() % num_players;

        boss->x += (players[target].x > boss->x) ? 1 : -1;
        boss->y += (players[target].y > boss->y) ? 1 : -1;

        printf("%s si muove\n", boss->name);
    }
    else if (action == 1) {
        int target = rand() % num_players;

        if (boss->weapon && boss->weapon->attack) {
            boss->weapon->attack(boss, &players[target]);
        }
    }
    else {
        boss_aoe_attack(boss, players, num_players);
    }
}

bool boss_encounter(Player *players, int num_players) {
    printf("BOSS ENCOUNTER!\n");

    Monster boss = {
        "Dragon",
        200,   // tanti HP
        true,
        2, 2,
        &monster_weapons[3],
        NULL
    };

    int turn = 0;

    while (1) {
        printf("\n=== TURNO %d ===\n", turn++);

        // TURNI GIOCATORI
        for (int i = 0; i < num_players; i++) {
            player_turn(&players[i], &boss, 1);
        }

        // Controllo vittoria
        if (boss.hp <= 0) {
            printf("Il boss è stato sconfitto!\n");
            return true;
        }

        // TURNO BOSS
        boss_turn(&boss, players, num_players);

        // Controllo sconfitta
        int alive_players = 0;
        for (int i = 0; i < num_players; i++) {
            if (players[i].hp > 0) alive_players++;
        }

        if (alive_players == 0) {
            printf("Il boss ha sconfitto tutti i giocatori!\n");
            return false;
        }
    }
}