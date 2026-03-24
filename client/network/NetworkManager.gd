# NetworkManager.gd (Autoload)
extends Node

var matchmaker = MatchmakerSocket.new()
var game = GameSocket.new()

signal game_found
signal game_data_received(cmd, args)

func _ready():
	game.data_received.connect(_on_game_data_received)
	matchmaker.connect_to_server()
	matchmaker.game_found.connect(_on_game_found)

func _process(_delta):
	# Monitoriamo entrambe le socket
	matchmaker.poll()
	game.poll()

func _on_game_found(port: int, _code: String):
	print("NetworkManager: Match trovato sulla porta %d. Switch al GameClient." % port)
	# Chiudiamo il matchmaker e passiamo al game
	matchmaker.socket.disconnect_from_host()
	game.connect_to_game(port)
	game_found.emit()
	
func _on_game_data_received(cmd: String, args: Array):
	game_data_received.emit(cmd, args)

# Funzioni helper per il resto del gioco
func send_to_matchmaker(msg: String): matchmaker.send_command(msg)
func send_to_game(msg: String): game.send_action(msg)
