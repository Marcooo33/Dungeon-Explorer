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
#include "combat.h"
#include "encounter.h"

/* =======================================================
 * VARIABILI GLOBALI
 * ======================================================= */
Player players[MAX_PLAYERS];
int    connected_count = 0;
char  *game_code       = NULL;
Dungeon dungeon;

/* =======================================================
 * INIZIALIZZAZIONE GIOCATORI
 * ======================================================= */
void init_players() {
    for (int i = 0; i < connected_count; i++) {
        players[i].hp        = 100;
        players[i].alive     = true;
        players[i].connected = true;
        players[i].x         = 0;
        players[i].y         = i * 50;
        players[i].gold      = 0;
        players[i].weapon    = &fists;
        players[i].armor     = NULL;
        players[i].item      = NULL;
    }
}

/* =======================================================
 * GESTIONE FINE PARTITA
 *
 * Viene chiamata sia in caso di vittoria (boss sconfitto)
 * sia in caso di sconfitta (tutti i giocatori morti).
 *
 * Ritorna:
 *   10 → host ha scelto STAY (torna in lobby, salva la sessione)
 *    1 → host ha scelto LEAVE, timeout, disconnessione o non trovato
 * ======================================================= */
static int handle_end_game() {
    /* --- Cerca l'host (player con id == 0) --- */
    int host_idx = -1;
    for (int i = 0; i < connected_count; i++) {
        if (players[i].id == 0) {
            host_idx = i;
            break;
        }
    }

    /* --- Invia MAKE_END_DECISION all'host (se connesso) --- */
    if (host_idx != -1 && players[host_idx].connected) {
        char *decision_msg = "MAKE_END_DECISION\n";
        ssize_t s = send(players[host_idx].socket_fd, decision_msg, strlen(decision_msg), MSG_NOSIGNAL);
        if (s <= 0) {
            mark_player_disconnected(&players[host_idx]);
            host_idx = -1;
        }
    } else {
        host_idx = -1;
    }

    if (host_idx == -1) {
        /* Host non disponibile: sciogliamo la sessione */
        printf("[GAME %s] Host non disponibile a fine partita.\n", game_code);
        broadcast("GAME_OVER\n");
        sleep(2);
        return 1;
    }

    /* --- Notifica gli altri giocatori che si è in attesa dell'host --- */
    char *wait_msg = "MESSAGE Partita terminata! In attesa della decisione del creatore della stanza...\n";
    for (int i = 0; i < connected_count; i++) {
        if (i != host_idx && players[i].connected)
            send(players[i].socket_fd, wait_msg, strlen(wait_msg), MSG_NOSIGNAL);
    }

    /* --- Attende la risposta dell'host (timeout 15 secondi) --- */
    struct pollfd host_fd = { players[host_idx].socket_fd, POLLIN, 0 };
    int activity = poll(&host_fd, 1, 15000);

    if (activity > 0 && (host_fd.revents & POLLIN)) {
        char buffer[128] = {0};
        int bytes = recv(players[host_idx].socket_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes > 0) {
            buffer[strcspn(buffer, "\r\n")] = 0;

            if (strncmp(buffer, "SEND_DECISION STAY", 18) == 0) {
                broadcast("MESSAGE Il creatore ha scelto di continuare! Si torna in lobby.\n");
                broadcast("VICTORY\n");
                sleep(2);
                return 10; /* Dice al Matchmaker di salvare lobby e giocatori */
            }
        } else {
            /* Host disconnesso mentre attendevamo */
            mark_player_disconnected(&players[host_idx]);
        }
    } else if (activity < 0 || (host_fd.revents & (POLLHUP | POLLERR))) {
        mark_player_disconnected(&players[host_idx]);
    }

    /* Host ha scelto LEAVE, è andato in timeout o si è disconnesso */
    printf("[GAME %s] Il creatore ha abbandonato o sciolto la stanza.\n", game_code);
    broadcast("GAME_OVER\n");
    sleep(2);
    return 1;
}

/* =======================================================
 * ESECUZIONE STANZA
 *
 * Esegue l'encounter della stanza corrente.
 * Se la stanza è già completata non fa nulla.
 * Se l'encounter restituisce "fine partita" (vittoria boss
 * o sconfitta totale), chiama handle_end_game().
 *
 * Ritorna:
 *    0 → partita continua
 *   10 → host ha scelto STAY
 *    1 → partita terminata (game over o leave)
 * ======================================================= */
static int run_room(Room *room) {
    if (room->completed) return 0;

    bool room_cleared = room->encounter(players, connected_count);
    room->completed = true;

    bool end_game_triggered = false;

    if (room_cleared) {
        /* Vittoria: controlla se era la stanza del boss */
        if (strcmp(room->type, "boss") == 0) {
            printf("[GAME %s] Il Boss è stato sconfitto! Dungeon completato.\n", game_code);
            broadcast("MESSAGE COMPLIMENTI! Il Boss è caduto e il Dungeon è stato ripulito!\n");
            end_game_triggered = true;
        }
    } else {
        /* Sconfitta: tutti i giocatori sono morti */
        printf("[GAME %s] Tutti i giocatori sono morti. Game Over.\n", game_code);
        broadcast("MESSAGE Siete stati sconfitti! Tutti i membri del gruppo sono morti.\n");
        end_game_triggered = true;
    }

    if (end_game_triggered)
        return handle_end_game();

    return 0;
}

/* =======================================================
 * VOTAZIONE PROSSIMA STANZA
 *
 * Invia MAKE_ROOM_DECISION a tutti i giocatori,
 * raccoglie i voti entro 20 secondi, risolve pareggi
 * e fallback casuale, e restituisce la Direction scelta.
 * ======================================================= */
static Direction vote_next_room(Room *current_room) {
    /* Costruisce e invia il messaggio con le porte disponibili */
    char room_info_msg[256];
    sprintf(room_info_msg, "MAKE_ROOM_DECISION ");

    for (int door_number = 0; door_number < current_room->doors_num; door_number++) {
        Direction door_direction = current_room->doors[door_number];
        Room *adjacent_room     = &dungeon.rooms[current_room->connected_rooms[door_direction]];
        sprintf(room_info_msg + strlen(room_info_msg), "%s:%s:%s ",
                direction_to_string(door_direction),
                adjacent_room->type,
                adjacent_room->completed ? "completed" : "not_completed");
    }
    sprintf(room_info_msg + strlen(room_info_msg), "\n");
    broadcast(room_info_msg);

    /* Inizializza poll e contatori */
    int votes[4]   = {0}; // NORTH, SOUTH, EAST, WEST
    int received   = 0;
    bool has_voted[MAX_PLAYERS] = {false};
    struct pollfd fds[MAX_PLAYERS];

    int expected_votes = 0;
    for (int i = 0; i < connected_count; i++) {
        if (players[i].connected && players[i].alive) {
            fds[i].fd     = players[i].socket_fd;
            fds[i].events = POLLIN;
            expected_votes++;
        } else {
            fds[i].fd    = -1; // poll ignora fd negativi
            has_voted[i] = true;
        }
        fds[i].revents = 0;
    }

    /* Loop di raccolta voti con timeout 20 secondi */
    int timeout_ms = 20000;
    int elapsed    = 0;
    int step       = 1000;

    while (received < expected_votes && elapsed < timeout_ms) {
        int activity = poll(fds, connected_count, step);

        if (activity < 0) {
            perror("poll error");
            break;
        }
        if (activity == 0) {
            elapsed += step;
            continue;
        }

        for (int i = 0; i < connected_count; i++) {
            if (has_voted[i]) continue;

            if (fds[i].revents & (POLLHUP | POLLERR)) {
                printf("[GAME %s] Player %d disconnesso durante il voto\n", game_code, players[i].id);
                mark_player_disconnected(&players[i]);
                fds[i].fd    = -1;
                has_voted[i] = true;
                expected_votes--;
                continue;
            }

            if (fds[i].revents & POLLIN) {
                char buffer[128] = {0};
                int bytes = recv(fds[i].fd, buffer, sizeof(buffer) - 1, 0);

                if (bytes <= 0) {
                    printf("[GAME %s] Player %d disconnesso durante il voto (recv)\n", game_code, players[i].id);
                    mark_player_disconnected(&players[i]);
                    fds[i].fd    = -1;
                    has_voted[i] = true;
                    expected_votes--;
                    continue;
                }

                if (strncmp(buffer, "SEND_DECISION ", 14) == 0) {
                    char *dir_str = buffer + 14;
                    dir_str[strcspn(dir_str, "\r\n")] = 0;

                    Direction d = string_to_direction(dir_str);
                    if (d >= 0 && d < 4)
                        votes[d]++;

                    has_voted[i] = true;
                    received++;
                    printf("[GAME %s] Player %d voted: %s\n", game_code, players[i].id, dir_str);
                }
            }
        }
    }

    if (received < expected_votes)
        printf("[GAME %s] Timeout votazione (%d/%d ricevuti)\n", game_code, received, expected_votes);

    /* Trova il massimo dei voti */
    int max_votes = 0;
    for (int i = 0; i < 4; i++)
        if (votes[i] > max_votes) max_votes = votes[i];

    /* Raccogli candidati validi (voti pari al massimo e porta esistente) */
    Direction candidates[4];
    int candidate_count = 0;
    for (int i = 0; i < 4; i++) {
        if (votes[i] == max_votes && max_votes > 0 && is_valid_direction(i, current_room))
            candidates[candidate_count++] = i;
    }

    /* Scelta finale */
    Direction chosen_direction;
    if (candidate_count == 0) {
        printf("[GAME %s] Nessun voto valido, fallback casuale\n", game_code);
        chosen_direction = current_room->doors[rand() % current_room->doors_num];
    } else if (candidate_count == 1) {
        chosen_direction = candidates[0];
    } else {
        int r = rand() % candidate_count;
        chosen_direction = candidates[r];
        printf("[GAME %s] Pareggio tra %d direzioni, scelta: %s\n",
               game_code, candidate_count, direction_to_string(chosen_direction));
    }

    printf("[GAME %s] Decisione finale: %s\n", game_code, direction_to_string(chosen_direction));
    return chosen_direction;
}

/* =======================================================
 * MAIN
 * ======================================================= */
int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <game_code> <fds>\n", argv[0]);
        return -1;
    }

    game_code = argv[1];
    printf("[GAME %s] Partita creata\n", game_code);

    connected_count = 0;

    /* Parsing socket FD e player ID dagli argomenti */
    char *saveptr_sockets, *saveptr_ids;
    char *socket_token = strtok_r(argv[2], " ", &saveptr_sockets);
    char *id_token     = strtok_r(argv[3], " ", &saveptr_ids);

    while (socket_token != NULL && id_token != NULL) {
        players[connected_count].socket_fd = atoi(socket_token);
        players[connected_count].id        = atoi(id_token);
        connected_count++;
        socket_token = strtok_r(NULL, " ", &saveptr_sockets);
        id_token     = strtok_r(NULL, " ", &saveptr_ids);
    }

    for (int i = 0; i < connected_count; i++)
        players[i].connected = true;

    printf("[GAME %s] Ricevuti %d socket\n", game_code, connected_count);
    broadcast("START_GAME\n");
    sleep(3);

    srand(time(NULL));

    /* Invia a ciascun client il proprio ID */
    char specific_client_msg[64];
    for (int i = 0; i < connected_count; i++) {
        sprintf(specific_client_msg, "SET_MY_ID %d\n", players[i].id);
        send(players[i].socket_fd, specific_client_msg, strlen(specific_client_msg), MSG_NOSIGNAL);
    }

    init_players();
    broadcast_all_players_info();

    /* Debug: info giocatori */
    printf("[DEBUG] Connessi %d giocatori\n", connected_count);
    for (int i = 0; i < connected_count; i++) {
        printf("Player %d: Socket FD=%d, HP=%d, Alive=%s, Position=(%d,%d), Gold=%d\n",
               players[i].id, players[i].socket_fd, players[i].hp,
               players[i].alive ? "Yes" : "No",
               players[i].x, players[i].y, players[i].gold);
    }

    dungeon = generate_dungeon(5);
    int current_room_idx = 0;
    int next_room_idx    = -1;
    Room *current_room   = NULL;

    /* ─────────────────── GAME LOOP ─────────────────── */
    while (true) {
        if (are_all_players_disconnected()) {
            printf("[GAME %s] Tutti i client si sono disconnessi. Chiudo la sessione.\n", game_code);
            return 1;
        }

        current_room = &dungeon.rooms[current_room_idx];

        /* 1) Esegui encounter della stanza corrente */
        int exit_code = run_room(current_room);
        if (exit_code != 0) return exit_code;

        /* 2) Votazione prossima stanza */
        Direction chosen = vote_next_room(current_room);
        next_room_idx = current_room->connected_rooms[chosen];

        /* 3) Notifica la stanza scelta a tutti i client */
        char room_msg[64];
        build_room_message(&dungeon.rooms[next_room_idx], room_msg, sizeof(room_msg));
        broadcast(room_msg);
        printf("[GAME %s] Sent: %s", game_code, room_msg);

        current_room_idx = next_room_idx;
    }
}