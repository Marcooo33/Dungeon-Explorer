extends Node2D

@onready var room_container = $RoomContainer

@onready var starting_room = preload("res://scenes/dungeon/rooms/starting_room.tscn")
@onready var room_1 = preload("res://scenes/dungeon/rooms/room_1.tscn")
@onready var room_2 = preload("res://scenes/dungeon/rooms/room_2.tscn")
@onready var room_3 = preload("res://scenes/dungeon/rooms/room_3.tscn")
@onready var room_4 = preload("res://scenes/dungeon/rooms/room_4.tscn")
@onready var player_scene = preload("res://characters/player/player.tscn")

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	var room_instance = starting_room.instantiate()
	room_container.add_child(room_instance)


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(_delta: float) -> void:
	pass
	
