#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <stdbool.h>
#include <time.h>
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

Weapon fists = {"Fists", 5, SHORT_RANGE, melee_attack};

void init_players() {
    for (int i = 0; i < connected_count; i++) {
        players[i].hp = 100;
        players[i].alive = true;
        players[i].x = 0; // Posizione iniziale X
        players[i].y = i * 50; // Posizione iniziale Y
        players[i].gold = 0;
        players[i].weapon = &fists; // Fists di default
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

    srand(time(NULL));
    printf("Seed generato\n");

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
            bool end_game_triggered = false; // Flag per capire se siamo arrivati a fine partita

            if (room_cleared) {
                current_room->completed = true;

                // Se la stanza completata era il BOSS, la partita è vinta e si attiva la fine gioco
                if (strcmp(current_room->type, "BOSS") == 0) {
                    printf("[GAME %s] Il Boss è stato sconfitto! Dungeon completato.\n", game_code);
                    broadcast("MESSAGE COMPLIMENTI! Il Boss è caduto e il Dungeon è stato ripulito!\n");
                    end_game_triggered = true;
                }
            }
            else {
                // Se i giocatori perdono, la stanza fallisce e si attiva comunque la fine gioco
                printf("[GAME %s] Tutti i giocatori sono morti nella stanza %d. Game Over.\n", game_code, current_room_idx);
                broadcast("MESSAGE Siete stati sconfitti! Tutti i membri del gruppo sono morti.\n");
                end_game_triggered = true;
            }

            // 🌟 GESTIONE FINE PARTITA: Sceglie SOLO il creatore della stanza (Host)
            // Viene eseguita sia in caso di Vittoria del Boss sia in caso di Sconfitta totale
            if (end_game_triggered) {
                // Identifichiamo l'indice dell'Host (colui che ha id == 0)
                int host_idx = -1;
                for (int i = 0; i < connected_count; i++) {
                    if (players[i].id == 0) {
                        host_idx = i;
                        break;
                    }
                }

                if (host_idx != -1) {
                    // Chiediamo la decisione SOLO all'Host
                    char *decision_msg = "MAKE_END_DECISION Scegli: STAY per tornare in lobby con il gruppo, LEAVE per sciogliere la stanza.\n";
                    send(players[host_idx].socket_fd, decision_msg, strlen(decision_msg), 0);
                    
                    // Notifichiamo gli altri giocatori che siamo in attesa dell'Host (usiamo un messaggio generico)
                    char *wait_msg = "MESSAGE Partita terminata! In attesa della decisione del creatore della stanza...\n";
                    for (int i = 0; i < connected_count; i++) {
                        if (i != host_idx) {
                            send(players[i].socket_fd, wait_msg, strlen(wait_msg), 0);
                        }
                    }

                    // Attendiamo la risposta dell'Host con un timeout di 15 secondi
                    struct pollfd host_fd;
                    host_fd.fd = players[host_idx].socket_fd;
                    host_fd.events = POLLIN;

                    int activity = poll(&host_fd, 1, 15000); 
                    if (activity > 0 && (host_fd.revents & POLLIN)) {
                        char buffer[128] = {0};
                        int bytes = recv(players[host_idx].socket_fd, buffer, sizeof(buffer) - 1, 0);
                        
                        if (bytes > 0) {
                            buffer[strcspn(buffer, "\r\n")] = 0;

                            if (strncmp(buffer, "SEND_DECISION STAY", 18) == 0) {
                                broadcast("MESSAGE Il creatore ha scelto di continuare! Si torna in lobby.\n");
                                broadcast("VICTORY\n"); // Manda il segnale ai client per caricare la scena della lobby
                                sleep(2);
                                return 10; // 👈 Codice speciale: dice al Matchmaker di salvare la lobby ed i player
                            }
                        }
                    }
                }

                // Se l'Host sceglie LEAVE, va in timeout o si disconnette, la stanza si scioglie
                printf("[GAME %s] Il creatore ha abbandonato o sciolto la stanza.\n", game_code);
                broadcast("GAME_OVER\n");
                sleep(2);
                return 1; // Uscita standard (scioglimento della sessione)
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
        sprintf(treasure_found_message, "MESSAGE Oro trovato: %d\n", gold_found);
        send(players[i].socket_fd, treasure_found_message, strlen(treasure_found_message), 0);
    }
    
    printf("sono qui\n");
    for (int i = 0; i < num_players; i++) {
        broadcast_player_info(&players[i]);
    }
    printf("sono dopo il broadcast\n");
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
            int weapon_idx = rand() % 2;
            Weapon *found_weapon = &player_weapons[weapon_idx];

            sprintf(treasure_found_message, "MAKE_INVENTORY_DECISION Hai trovato un'arma: %s atk: %d range: %d\n", found_weapon->name, found_weapon->damage, found_weapon->range);
            send(players[i].socket_fd, treasure_found_message, strlen(treasure_found_message), 0);
            memset(decision_buffer, 0, sizeof(decision_buffer));
            recv(players[i].socket_fd, decision_buffer, sizeof(decision_buffer) - 1, 0); // Aspettiamo la decisione del client

            if(strncmp(decision_buffer, "SEND_DECISION T", 15) == 0) {
                printf("[DEBUG] Trovata arma: %s, danno: %d, range: %d\n", found_weapon->name, found_weapon->damage, found_weapon->range);
                players[i].weapon = found_weapon;
            }

            broadcast_player_info(&players[i]);


        } else if (reward_type == 1) {
            int armor_idx = rand() % 2;
            Armor *found_armor = &armors[armor_idx];
            
            sprintf(treasure_found_message, "MAKE_INVENTORY_DECISION Hai trovato un'armatura: %s def: %d\n", found_armor->name, found_armor->defense);
            send(players[i].socket_fd, treasure_found_message, strlen(treasure_found_message), 0);
            memset(decision_buffer, 0, sizeof(decision_buffer));
            recv(players[i].socket_fd, decision_buffer, sizeof(decision_buffer) - 1, 0); // Aspettiamo la decisione del client

            if (strncmp(decision_buffer, "SEND_DECISION T", 15) == 0)
            {
                printf("[DEBUG] Trovata armatura: %s, difesa: %d\n", found_armor->name, found_armor->defense);
                players[i].armor = found_armor;
            }

            broadcast_player_info(&players[i]);

        } else {
            int item_idx = rand() % 1;
            Item *found_item = &items[item_idx];
            sprintf(treasure_found_message, "MAKE_INVENTORY_DECISION Hai trovato un oggetto: %s\n", found_item->name);
            send(players[i].socket_fd, treasure_found_message, strlen(treasure_found_message), 0);
            memset(decision_buffer, 0, sizeof(decision_buffer));
            recv(players[i].socket_fd, decision_buffer, sizeof(decision_buffer) - 1, 0); // Aspettiamo la decisione del client

            if (strncmp(decision_buffer, "SEND_DECISION T", 15) == 0)
            {
                printf("[DEBUG] Trovato oggetto: %s\n", found_item->name);
                players[i].item = found_item;
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
    return room_cleared;
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
    if (m->hp < 0) m->hp = 0; // Evita HP negativi

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
    if (m->hp < 0) m->hp = 0; // Evita HP negativi

    printf("Il Giocatore %d attacca a distanza %s per %d danni (HP: %d)\n",
           p->id, m->name, damage, m->hp);
}

void move_player(Player *p, int x, int y) {
    p->x = x;
    p->y = y;

    printf("Il Giocatore %d si muove a (%d, %d)\n", p->id, p->x, p->y);
}

void use_item(Player *p) {
    if (!p->item) {
        printf("[DEBUG] Il giocatore %d non ha oggetti da usare!\n", p->id);
        char no_item_msg[64];
        sprintf(no_item_msg, "MESSAGE Non hai oggetti da usare!\n");
        send(p->socket_fd, no_item_msg, strlen(no_item_msg), 0);
        return;
    }

    const char *item_name = p->item->name;
    printf("Il Giocatore %d usa %s\n", p->id, item_name);

    // Usa l'oggetto (modifica HP, ecc.)
    p->item->use(p);

    // Rimuovi l'oggetto dall'inventario (consumabile one-shot)
    p->item = NULL;

    // Notifica il giocatore via messaggio
    char used_msg[128];
    sprintf(used_msg, "MESSAGE Hai usato %s! +15 HP. L'oggetto è stato rimosso dall'inventario.\n", item_name);
    send(p->socket_fd, used_msg, strlen(used_msg), 0);

    // Aggiorna tutti i client con le nuove info del giocatore (HP e inventario)
    broadcast_player_info(p);
}

void health_potion_function(Player *p) {
    int heal = 30;
    p->hp += heal;
    if (p->hp > 100) p->hp = 100;

    printf("Il Giocatore %d recupera %d HP (HP attuali: %d)\n",
           p->id, heal, p->hp);
}

// MAKE_TURN_DECISION

bool is_tile_occupied_by_player(int x, int y, Player *players, int num_players) {
    for (int i = 0; i < num_players; i++) {
        if (players[i].alive && players[i].x == x && players[i].y == y) {
            return true;
        }
    }
    return false;
}

// Funzione per controllare se una coordinata è occupata da un mostro vivo
bool is_tile_occupied_by_monster(int x, int y, Monster *monsters, int num_monsters) {
    for (int i = 0; i < num_monsters; i++) {
        if (monsters[i].alive && monsters[i].x == x && monsters[i].y == y) {
            return true;
        }
    }
    return false;
}


void player_turn(Player *p, Monster *monsters, int num_monsters) {
    send(p->socket_fd, "MAKE_TURN_DECISION\n", strlen("MAKE_TURN_DECISION\n"), 0);
    
    char decision_buffer[128] = {0};
    int bytes = recv(p->socket_fd, decision_buffer, sizeof(decision_buffer) - 1, 0);
    if (bytes <= 0) {
        printf("[DEBUG] Nessuna decisione ricevuta per il giocatore %d\n", p->id);
        return;
    }

    decision_buffer[strcspn(decision_buffer, "\r\n")] = 0; // Rimuove newline finale

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
        return;
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
    if (p->hp < 0) p->hp = 0;

    printf("%s attacca il Giocatore %d per %d danni (HP: %d)\n",
           m->name, p->id, damage, p->hp);
}

void monster_turn(Monster *m, Player *players, int num_players) {
    printf("\nTurno del mostro %s\n", m->name);

    int target = rand() % num_players;
    int dist = distance(m->x, m->y, players[target].x, players[target].y);

    if (dist > m->weapon->range) {
        // Avvicinamento intelligente (non fa movimenti a caso se è già allineato)
        if (dist > m->weapon->range) {
            int next_x = m->x;
            int next_y = m->y;

            // Calcola dove vuole andare il mostro
            if (players[target].x > m->x) next_x += 50;
            else if (players[target].x < m->x) next_x -= 50;

            if (players[target].y > m->y) next_y += 50;
            else if (players[target].y < m->y) next_y -= 50;

            // Applica i limiti mappa
            if (next_x < 0) next_x = 0;
            if (next_x > 450) next_x = 450;
            if (next_y < 0) next_y = 0;
            if (next_y > 200) next_y = 200;

            // Si muove solo se non schiaccia un giocatore!
            if (!is_tile_occupied_by_player(next_x, next_y, players, num_players)) {
                m->x = next_x;
                m->y = next_y;
                printf("%s si avvicina a (%d, %d)\n", m->name, m->x, m->y);
            }
        } 
    } else {
            if (m->weapon && m->weapon->attack)
                m->weapon->attack(m, &players[target]);
        }
}
bool combat_encounter(Player *players, int num_players) {
    broadcast("MESSAGE Appena entrati nella stanza vi trovate davanti dei pericolosi mostri, l'unica alternativa è combatterli!\n");
    
    int num_monsters = 2;
    Monster monsters[2] = {
        {0, "Skeleton", 10, true, 300, 0, &monster_weapons[2], NULL},
        {1, "Orc", 10, true, 300, 50, &monster_weapons[2], NULL}
    };

    for (int i = 0; i < num_monsters; i++) {
        broadcast_monster_info(&monsters[i]);
    }

    int turn = 0;
    char message[256]; 

    while (1) {
        sprintf(message, "MESSAGE === ROUND %d ===\n", turn++);
        broadcast(message);

        // --- FASE 1: TURNI GIOCATORI ---
        for (int i = 0; i < num_players; i++) {
            if (players[i].alive) {
                player_turn(&players[i], monsters, num_monsters);
                broadcast_player_info(&players[i]);

                // RISOLUZIONE IMMEDIATA MORTE MOSTRI
                for (int j = 0; j < num_monsters; j++) {
                    if (monsters[j].alive && monsters[j].hp <= 0) {
                        monsters[j].alive = false;
                        char msg[128];
                        sprintf(msg, "MESSAGE Il mostro %s e' morto!\n", monsters[j].name);
                        broadcast(msg);
                    }
                    // Aggiorniamo subito Godot prima che tocchi al prossimo giocatore
                    broadcast_monster_info(&monsters[j]);
                }

                // Se tutti i mostri sono morti, interrompiamo i turni dei giocatori!
                if (are_all_monsters_dead(monsters, num_monsters)) {
                    break; 
                }
            }
        }

        // --- FASE 2: CONTROLLO VITTORIA ---
        if (are_all_monsters_dead(monsters, num_monsters)) {
            broadcast("MESSAGE L'ultimo mostro finisce a terra e tirate tutti un sospiro di sollievo... senza abbassare la guardia\n");
            
            reset_players_position(players);
            for (int i = 0; i < connected_count; i++) {
                if (players[i].alive) broadcast_player_info(&players[i]);
            }
            return true;
        }

        // --- FASE 3: TURNI MOSTRI ---
        for (int i = 0; i < num_monsters; i++) {
            if (monsters[i].alive) {
                sleep(1);
                monster_turn(&monsters[i], players, num_players);
                broadcast_monster_info(&monsters[i]);

                // RISOLUZIONE IMMEDIATA MORTE GIOCATORI
                for (int j = 0; j < num_players; j++) {
                    if (players[j].alive && players[j].hp <= 0) {
                        players[j].alive = false;
                        char msg[64];
                        sprintf(msg, "MESSAGE Il Giocatore %d e' morto!\n", players[j].id);
                        broadcast(msg);
                    }
                    // Aggiorniamo subito Godot per far sparire il giocatore o calare gli HP
                    broadcast_player_info(&players[j]);
                }

                // Se tutti i giocatori muoiono, inutile far attaccare gli altri mostri
                if (are_all_players_dead(players, num_players)) {
                    break;
                }
            }
        }

        // --- FASE 4: CONTROLLO SCONFITTA ---
        if (are_all_players_dead(players, num_players)) {
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
    broadcast("\nMESSAGE Il boss usa ATTACCO AD AREA!\n");

    for (int i = 0; i < num_players; i++) {
        // Ignoriamo i giocatori già morti
        if (players[i].hp <= 0) continue;

        // 1. Calcoliamo la distanza dal boss al giocatore
        int dist = distance(boss->x, boss->y, players[i].x, players[i].y);
        
        int base_damage = boss->weapon->damage; // Danno base dell'arma del boss
        int calculated_damage = 0;

        // 2. Scaliamo i danni in base alle caselle (1 casella = 50px)
        if (dist <= 50) {
            // Stessa casella o 1 casella di distanza: 100% del danno
            calculated_damage = base_damage;
        } else if (dist <= 100) {
            // 2 caselle di distanza: 60% del danno
            calculated_damage = (int)(base_damage * 0.6);
        } else if (dist <= 150) {
            // 3 caselle di distanza: 30% del danno
            calculated_damage = (int)(base_damage * 0.3);
        } else {
            // Fuori range (> 3 caselle): Nessun danno!
            printf("Il Giocatore %d e' abbastanza lontano da schivare l'onda d'urto!\n", players[i].id);
            continue; // Saltiamo al prossimo giocatore
        }

        // 3. Applichiamo la difesa dell'armatura
        int defense = players[i].armor ? players[i].armor->defense : 0;
        calculated_damage -= defense;
        
        // Evitiamo danni negativi se l'armatura supera il danno
        if (calculated_damage < 0) calculated_damage = 0;

        // 4. Sottraiamo i punti vita
        players[i].hp -= calculated_damage;
        if (players[i].hp < 0) players[i].hp = 0;

        // 5. Stampa a schermo i risultati (incluso il calcolo della distanza)
        printf("Il Giocatore %d (a %dpx di distanza) subisce %d danni (HP: %d)\n",
               players[i].id,
               dist,
               calculated_damage,
               players[i].hp);
    }
}

void boss_turn(Monster *boss, Player *players, int num_players) {
    if (boss->hp <= 0) return;

    printf("\nTurno del BOSS %s\n", boss->name);
    char msg[128];

    // 1. Troviamo il giocatore vivo più vicino
    int closest_player_idx = -1;
    int min_dist = 999999; // Valore iniziale altissimo

    for (int i = 0; i < num_players; i++) {
        if (players[i].hp > 0 && players[i].alive) {
            int d = distance(boss->x, boss->y, players[i].x, players[i].y);
            if (d < min_dist) {
                min_dist = d;
                closest_player_idx = i;
            }
        }
    }

    // Se tutti i giocatori sono già morti (non dovrebbe succedere grazie ai controlli precedenti, ma è una sicurezza)
    if (closest_player_idx == -1) return;

    // 2. Applichiamo la logica intelligente in base alla distanza del giocatore più vicino
    
    // CASO 1: Può attaccare da vicino (<= range della sua arma, presumibilmente 50px)
    if (min_dist <= boss->weapon->range) {
        
        sprintf(msg, "MESSAGE Il Boss sferra un attacco ravvicinato contro il Giocatore %d!\n", players[closest_player_idx].id);
        broadcast(msg);
        
        if (boss->weapon && boss->weapon->attack) {
            boss->weapon->attack(boss, &players[closest_player_idx]);
        }
    } 
    // CASO 2: Non arriva da vicino, ma c'è almeno un giocatore nel raggio dell'attacco ad area (<= 150px)
    else if (min_dist <= 150) {
        
        broadcast("MESSAGE Il Boss sbatte i pugni a terra: ONDA D'URTO AD AREA!\n");
        boss_aoe_attack(boss, players, num_players);
    } 
    // CASO 3: Tutti i giocatori sono lontani, quindi si sposta verso quello più vicino
    else {
        
        // Calcoliamo la direzione in cui muoverci verso il giocatore più vicino
        if (players[closest_player_idx].x > boss->x) boss->x += 50;
        else if (players[closest_player_idx].x < boss->x) boss->x -= 50;

        if (players[closest_player_idx].y > boss->y) boss->y += 50;
        else if (players[closest_player_idx].y < boss->y) boss->y -= 50;

        
        printf("%s si muove verso il giocatore %d\n", boss->name, players[closest_player_idx].id);
    }
}

bool boss_encounter(Player *players, int num_players) {
    broadcast("MESSAGE Entrate in una stanza più grande delle altre, al centro vedete un enorme mostro che vi fissa con occhi pieni di odio... è il boss del dungeon!\n");

    Monster boss = {0, "Knight", 150, true, 200, 50, &monster_weapons[3], NULL };
    broadcast_monster_info(&boss);

    int turn = 0;
    char message[256]; 

    while (1) {
        sprintf(message, "MESSAGE === ROUND %d ===\n", turn++);
        broadcast(message);

        // --- FASE 1: TURNI GIOCATORI ---
        for (int i = 0; i < num_players; i++) {
            if (players[i].alive) {
                player_turn(&players[i], &boss, 1); // Passiamo 1 come numero di mostri (il boss)
                broadcast_player_info(&players[i]);

                // RISOLUZIONE IMMEDIATA MORTE BOSS
                if (boss.alive && boss.hp <= 0) {
                    boss.alive = false;
                    char msg[128];
                    sprintf(msg, "MESSAGE Il boss %s è stato sconfitto!\n", boss.name);
                    broadcast(msg);
                }
                
                // Aggiorniamo sempre Godot con i nuovi HP del boss
                broadcast_monster_info(&boss);

                // Se il boss è morto, interrompiamo i turni dei giocatori successivi!
                if (!boss.alive) {
                    break; 
                }
            }
        }

        // --- FASE 2: CONTROLLO VITTORIA ---
        if (!boss.alive) {
            broadcast("MESSAGE Il boss è stato sconfitto! Avete completato il dungeon!\n");
            
            reset_players_position(players);
            for (int i = 0; i < connected_count; i++) {
                if (players[i].alive) broadcast_player_info(&players[i]);
            }
            return true;
        }

        // --- FASE 3: TURNO BOSS ---
        if (boss.alive) {
            sleep(1); // Piccola pausa per far capire ai giocatori che tocca al boss
            
            boss_turn(&boss, players, num_players); // boss_turn gestisce internamente boss_aoe_attack
            broadcast_monster_info(&boss);

            // RISOLUZIONE IMMEDIATA MORTE GIOCATORI
            for (int j = 0; j < num_players; j++) {
                if (players[j].alive && players[j].hp <= 0) {
                    players[j].alive = false;
                    char msg[64];
                    sprintf(msg, "MESSAGE Il Giocatore %d e' morto!\n", players[j].id);
                    broadcast(msg);
                }
                // Aggiorniamo subito Godot per far sparire il giocatore o calare gli HP a schermo
                broadcast_player_info(&players[j]);
            }
        }

        // --- FASE 4: CONTROLLO SCONFITTA ---
        if (are_all_players_dead(players, num_players)) {
            broadcast("MESSAGE Il boss ha annientato il gruppo! I mostri vincono!\n");
            return false;
        }
    }
    
    // Fallback di sicurezza a fine scontro
    reset_players_position(players);
    for (int i = 0; i < connected_count; i++) {
        broadcast_player_info(&players[i]);
    }
    return true;
}