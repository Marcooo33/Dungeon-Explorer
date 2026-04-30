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
	var room_instance = starting_room.instantiate()
	room_container.add_child(room_instance)


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(_delta: float) -> void:
	pass
	
