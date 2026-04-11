typedef struct {
    char id[20];
    char type[20];
    char doors[4][2]; // Array of strings for doors (e.g., "N", "S", "E", "W")
    bool completed;
    Room *connected_rooms[4]; // Pointers to connected rooms
} Room;


typedef struct{
    int max_rooms;
    Room rooms[max_rooms];
} Dungeon;

generate_room(int current_room_idx, int current_door){
    Room new_room;

    new_room.connected_rooms[get_opposite_door(current_door)] = current_room_idx;
    // Logic to generate a new room based on the current room index
    // This can include random selection of room type, doors, etc.
    return new_room;
}

void generate_dungeon(){
    Dungeon dungeon;

    dungeon.rooms[0]= {
        .id = "room_0",
        .type = "start",
        .doors = {"E"},
        .completed = false,
        .connected_rooms = {NULL, NULL, NULL, NULL}
    };

    int generated_rooms=0;
    int current_room_idx=0;
    int current_door=0;
    Room *current_room;
    while (generated_rooms < dungeon.max_rooms){
        current_room = &dungeon.rooms[current_room_idx];
        for(current_door=0; current_door < current_room.doors; current_door++){
            current_room.connected_rooms[current_door] = generate_room(current_room_idx, current_door);
        }
        current_room_idx++;
    }
    
}

