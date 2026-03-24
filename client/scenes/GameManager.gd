extends Node2D

# Percorso della scena del giocatore dal tuo progetto
@onready var player_scene = preload("res://characters/player/player.tscn")

# Dizionario per gestire i nodi dei giocatori: { id_int: NodoPlayer }
var players_nodes = {}

func _ready():
	# Ci connettiamo al nuovo segnale "pulito" del NetworkManager
	NetworkManager.game_data_received.connect(_on_game_data_received)
	print("GameManager: In ascolto di comandi di gioco pre-elaborati")

# Ora riceve CMD (String) e ARGS (Array) invece di una stringa grezza
func _on_game_data_received(cmd: String, args: Array):
	match cmd:
		"SPAWN":
			if args.size() >= 3:
				var id = args[0].to_int()
				var pos = Vector2(args[1].to_float(), args[2].to_float())
				_handle_spawn(id, pos)
		
		"MOVE":
			if args.size() >= 3:
				var id = args[0].to_int()
				var pos = Vector2(args[1].to_float(), args[2].to_float())
				_update_player_position(id, pos)
		
		"REMOVE": # Utile se un giocatore si disconnette
			if args.size() >= 1:
				_handle_remove(args[0].to_int())


### LOGICA DI RENDERING

func _handle_spawn(id: int, pos: Vector2):
	if not players_nodes.has(id):
		var new_player = player_scene.instantiate()
		new_player.name = "Player_" + str(id)
		new_player.position = pos
		
		# Assegnazione colori in base all'ID 
		match id:
			0: new_player.modulate = Color.BLUE
			1: new_player.modulate = Color.RED
			2: new_player.modulate = Color.GREEN
			3: new_player.modulate = Color.YELLOW
			
		add_child(new_player)
		players_nodes[id] = new_player
		print("Rendering: Creato giocatore %d in %v" % [id, pos])

func _update_player_position(id: int, pos: Vector2):
	if players_nodes.has(id):
		# Puoi usare un tween qui se vuoi un movimento fluido invece di un teleport
		players_nodes[id].position = pos

func _handle_remove(id: int):
	if players_nodes.has(id):
		players_nodes[id].queue_free()
		players_nodes.erase(id)
		print("Rendering: Rimosso giocatore %d" % id)
