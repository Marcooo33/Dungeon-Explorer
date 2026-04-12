extends Node

enum State {
	LOBBY,
	GAME
}

var state := State.LOBBY

var client := NetworkSocket.new()

# ---------------- SIGNALS ----------------

signal game_found
signal game_data_received(cmd, args)
signal join_request_received(player_id)
signal join_accepted
signal join_rejected
signal error_received(msg: String)

func _ready():
	connect_to_server()
	client.raw_packet.connect(_on_raw_packet)

func _process(_delta):
	client.poll()

# ---------------- API PUBBLICA ----------------

func connect_to_server():
	client.connect_to_server()

func send_to_server(msg: String):
	client.send(msg)

# ---------------- DISPATCH ----------------

func _on_raw_packet(cmd: String, args: Array):
	match state:
		State.LOBBY:
			_handle_lobby(cmd, args)
		State.GAME:
			_handle_game(cmd, args)

# ---------------- LOBBY ----------------

func _handle_lobby(cmd: String, args: Array):
	match cmd:
		"JOIN_REQUEST":
			if args.size() >= 1:
				join_request_received.emit(args[0].to_int())

		"JOIN_ACCEPTED":
			join_accepted.emit()

		"JOIN_REJECTED":
			join_rejected.emit()

		"ERROR":
			error_received.emit(" ".join(args))

		"START_GAME":
			state = State.GAME
			get_tree().change_scene_to_file("res://scenes/dungeon/rooms/starting_room/starting_room.tscn")
			game_found.emit()

# ---------------- GAME ----------------

func _handle_game(cmd: String, args: Array):
	print("Sending to GameManager: ", cmd, " ", args)
	game_data_received.emit(cmd, args)
