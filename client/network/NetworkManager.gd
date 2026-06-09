extends Node

enum State {
	LOBBY,
	GAME
}

var state := State.LOBBY
var client := NetworkSocket.new()

# --- VARIABILI DI STATO ---
var current_room_code: String = ""
var current_players: Array = []

# ---------------- SIGNALS ----------------

signal game_found
signal game_data_received(cmd, args)
signal join_request_received(player_id)
signal join_accepted
signal join_rejected
signal error_received(msg: String)
signal room_code_received(code: String)
signal player_list_updated(players: Array)

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

func reset_connection():
	current_room_code = ""
	current_players = []
	state = State.LOBBY
	client = NetworkSocket.new()
	client.raw_packet.connect(_on_raw_packet)
	client.connect_to_server()

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
			get_tree().change_scene_to_file("res://scenes/game/Game.tscn")
			game_found.emit()
			
		"ROOM_CODE":
			if args.size() >= 1:
				current_room_code = args[0]
				room_code_received.emit(args[0])
		
		"PLAYER_LIST":
			current_players = args
			player_list_updated.emit(args)

# ---------------- GAME ----------------

func _handle_game(cmd: String, args: Array):
	if cmd == "VICTORY":
		state = State.LOBBY
		game_data_received.emit(cmd, args)
		return
	if cmd == "GAME_OVER":
		state = State.LOBBY
		game_data_received.emit(cmd, args)
		return
	print("Sending to GameManager: ", cmd, " ", args)
	game_data_received.emit(cmd, args)
