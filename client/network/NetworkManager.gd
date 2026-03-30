# NetworkManager.gd (Autoload)
extends Node

var matchmaker = MatchmakerSocket.new()
var game = GameSocket.new()

signal game_found
signal game_data_received(cmd, args)
signal join_request_received(player_id) 
signal join_accepted
signal join_rejected
signal error_received(msg: String)


func _ready():
	game.data_received.connect(_on_game_data_received)
	matchmaker.connect_to_server()
	matchmaker.join_request_received.connect(func(id): join_request_received.emit(id))
	matchmaker.join_accepted.connect(func(): join_accepted.emit())
	matchmaker.join_rejected.connect(func(): join_rejected.emit())
	matchmaker.error_received.connect(func(msg): error_received.emit(msg))

func _process(_delta):
	matchmaker.poll()
	game.poll()

func _on_join_game(port: int, _code: String):
	print("NetworkManager: Match trovato sulla porta %d. Switch al GameClient." % port)
	matchmaker.socket.disconnect_from_host()
	game.connect_to_game(port)
	game_found.emit()
	
func _on_game_data_received(cmd: String, args: Array):
	game_data_received.emit(cmd, args)

func send_to_matchmaker(msg: String): matchmaker.send_command(msg)
func send_to_game(msg: String): game.send_action(msg)
