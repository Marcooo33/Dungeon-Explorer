extends Node2D

@onready var room_container = $RoomContainer

@onready var starting_room = preload("res://scenes/dungeon/rooms/starting_room.tscn")
# 1 door rooms
@onready var room_n = preload("res://scenes/dungeon/rooms/1_door_rooms/room_n.tscn")
@onready var room_s = preload("res://scenes/dungeon/rooms/1_door_rooms/room_s.tscn")
@onready var room_e = preload("res://scenes/dungeon/rooms/1_door_rooms/room_e.tscn")
@onready var room_w = preload("res://scenes/dungeon/rooms/1_door_rooms/room_w.tscn")

# 2 doors rooms
@onready var room_ns = preload("res://scenes/dungeon/rooms/2_doors_rooms/room_ns.tscn")
@onready var room_ne = preload("res://scenes/dungeon/rooms/2_doors_rooms/room_ne.tscn")
@onready var room_nw = preload("res://scenes/dungeon/rooms/2_doors_rooms/room_nw.tscn")
@onready var room_se = preload("res://scenes/dungeon/rooms/2_doors_rooms/room_se.tscn")
@onready var room_sw = preload("res://scenes/dungeon/rooms/2_doors_rooms/room_sw.tscn")
@onready var room_ew = preload("res://scenes/dungeon/rooms/2_doors_rooms/room_ew.tscn")

# 3 doors rooms
@onready var room_nse = preload("res://scenes/dungeon/rooms/3_doors_rooms/room_nse.tscn")
@onready var room_nsw = preload("res://scenes/dungeon/rooms/3_doors_rooms/room_nsw.tscn")
@onready var room_new = preload("res://scenes/dungeon/rooms/3_doors_rooms/room_new.tscn")
@onready var room_sew = preload("res://scenes/dungeon/rooms/3_doors_rooms/room_sew.tscn")

# 4 doors room
@onready var room_nsew = preload("res://scenes/dungeon/rooms/4_doors_room/room_nsew.tscn")

@onready var player_scene = preload("res://characters/player/player.tscn")

# Called when the node enters the scene tree for the first time.a
func _ready() -> void:
	GameManager.load_room.connect(_on_loading_room)
	
	# Inietta i riferimenti della scena dentro il GameManager globale
	GameManager.room_container = $RoomContainer
	GameManager.players_container = $PlayersContainer
	GameManager.monsters_container = $MonstersContainer
	GameManager.hud_top = $HudTop
	GameManager.hud_bottom = $HudBottom
	
	var room_instance = starting_room.instantiate()
	room_container.add_child(room_instance)

func _on_loading_room(directions: String):
	var variable_name = "room_" + directions.to_lower()
	
	# Otteniamo il riferimento alla PackedScene
	var room_scene = get(variable_name)
	
	# Se vuoi rimuovere la stanza precedente:
	for child in room_container.get_children():
		child.queue_free()
			
	if room_container:
		var room_instance = room_scene.instantiate()
		room_container.add_child(room_instance)
	else:
		push_error("Errore: Impossibile trovare la stanza corrispondente a: " + variable_name)
	

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(_delta: float) -> void:
	pass
		
