extends Node2D

@onready var player_scene = preload("res://characters/player/player.tscn")

var players_nodes = {}

func _ready():
	print("GameManager ready")
	NetworkManager.game_data_received.connect(_on_game_data_received)

func _on_game_data_received(cmd: String, args: Array):
	print("Ricevuto: ", cmd, " ", args)
	match cmd:
		"SPAWN":
			if args.size() >= 3:
				_spawn(args)

		"MOVE":
			if args.size() >= 3:
				_move(args)

		"REMOVE":
			if args.size() >= 1:
				_remove(args)

func _spawn(args):
	var id = args[0].to_int()
	var pos = Vector2(args[1].to_float(), args[2].to_float())

	if players_nodes.has(id):
		return

	var p = player_scene.instantiate()
	p.name = "Player_%d" % id
	p.position = pos

	add_child(p)
	players_nodes[id] = p

func _move(args):
	var id = args[0].to_int()
	var pos = Vector2(args[1].to_float(), args[2].to_float())

	if players_nodes.has(id):
		players_nodes[id].global_position = pos

func _remove(args):
	var id = args[0].to_int()

	if players_nodes.has(id):
		players_nodes[id].queue_free()
		players_nodes.erase(id)
