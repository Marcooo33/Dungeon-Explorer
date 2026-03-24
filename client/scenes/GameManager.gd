extends Node2D

# Percorso della scena del giocatore dal tuo progetto
@onready var player_scene = preload("res://characters/player/player.tscn")

# Dizionario per gestire i nodi dei giocatori: { id_int: NodoPlayer }
var players_nodes = {}

func _ready():
	# Ci connettiamo al segnale del NetworkManager
	NetworkManager.game_data_received.connect(_on_game_data_received)
	print("GameManager: Pronto a ricevere dati di gioco")

func _on_game_data_received(data: String):
	# Il server C potrebbe inviare più comandi in un unico pacchetto
	var lines = data.split("\n")
	for line in lines:
		var trimmed = line.strip_edges()
		if trimmed != "":
			_parse_message(trimmed)

func _parse_message(msg: String):
	var parts = msg.split(" ")
	
	# Logica SPAWN: "SPAWN ID X Y"
	if parts[0] == "SPAWN" and parts.size() >= 4:
		var id = parts[1].to_int()
		var x = parts[2].to_int()
		var y = parts[3].to_int()
		_handle_spawn(id, Vector2(x, y))
	
	# Esempio futuro: Logica MOVE per la sincronizzazione
	elif parts[0] == "MOVE" and parts.size() >= 4:
		var id = parts[1].to_int()
		var x = parts[2].to_int()
		var y = parts[3].to_int()
		_update_player_position(id, Vector2(x, y))

func _handle_spawn(id: int, pos: Vector2):
	if not players_nodes.has(id):
		var new_player = player_scene.instantiate()
		new_player.name = "Player_" + str(id)
		new_player.position = pos
		
		# Distinguiamo i giocatori con i colori
		if id == 0:
			new_player.modulate = Color.BLUE
		elif id == 1:
			new_player.modulate = Color.RED
		elif id == 2:
			new_player.modulate = Color.GREEN
		elif id == 3:
			new_player.modulate = Color.YELLOW
			
		add_child(new_player)
		players_nodes[id] = new_player
		print("Rendering Giocatore ID: ", id, " in posizione: ", pos)

func _update_player_position(id: int, pos: Vector2):
	if players_nodes.has(id):
		players_nodes[id].position = pos
