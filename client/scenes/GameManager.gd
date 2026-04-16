extends Node2D

# Caricamento della scena del giocatore
@onready var player_scene = preload("res://characters/player/player.tscn")

# Riferimento all'HUD e al RoomContainer
# get_parent() serve perché GameManager è fratello di HUD e RoomContainer
@onready var hud = get_parent().get_node("Hud")
@onready var room_container = get_parent().get_node("RoomContainer")

# Dizionario per mappare: ID (int) -> Nodo del Giocatore
var players_nodes = {}

# ID univoco del client corrente (assegnato dal server)
var my_id: int = -1

func _ready():
	print("GameManager pronto e connesso")
	# Connessione al segnale del NetworkManager
	NetworkManager.game_data_received.connect(_on_game_data_received)

func _on_game_data_received(cmd: String, args: Array):
	print("Ricevuto dal GameManager: ", cmd, " ", args)
	match cmd:
		"SET_MY_ID":
			if args.size() >= 1:
				my_id = args[0].to_int()
				if players_nodes.has(my_id):
					_setup_local_player_visuals(players_nodes[my_id])

		"SPAWN":
			if args.size() >= 3:
				_spawn(args)

		"MOVE":
			if args.size() >= 3:
				_move(args)

		"REMOVE":
			if args.size() >= 1:
				_remove(args)

		"PLAYER_INFO":
			if args.size() >= 4:
				_handle_player_info(args)

		"HP_PLAYER":
			if args.size() >= 2:
				_handle_hp_update(args)

	# Aggiorna l'interfaccia ad ogni pacchetto ricevuto
	if hud and hud.has_method("update_display"):
		hud.update_display(my_id, players_nodes)
		

# --- LOGICA DI SPAWN E MOVIMENTO ---

func _spawn(args):
	var id = args[0].to_int()
	var pos = Vector2(args[1].to_float(), args[2].to_float())

	if players_nodes.has(id):
		return

	var p = player_scene.instantiate()
	p.name = "Player_%d" % id
	p.position = pos
	p.set("hp", 100) # Assicurati che player.gd abbia 'var hp'

	# Aggiungiamo il giocatore al RoomContainer invece che al GameManager
	# così si muove correttamente con la mappa
	if room_container:
		room_container.add_child(p)
	else:
		add_child(p)
		
	players_nodes[id] = p
	
	if id == my_id:
		_setup_local_player_visuals(p)

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

func _handle_player_info(args):
	var id = args[0].to_int()
	var hp = args[1].to_int()
	var pos = Vector2(args[2].to_float(), args[3].to_float())

	if players_nodes.has(id):
		players_nodes[id].global_position = pos
		players_nodes[id].set("hp", hp)
	else:
		_spawn([args[0], args[2], args[3]])
		players_nodes[id].set("hp", hp)

func _handle_hp_update(args):
	var id = args[0].to_int()
	var hp = args[1].to_int()
	if players_nodes.has(id):
		players_nodes[id].set("hp", hp)

# --- ESTETICA ---

func _setup_local_player_visuals(player_node):
	# Attiva il triangolo indicatore creato nella scena player.tscn
	var indicator = player_node.get_node_or_null("MyPlayerIndicator")
	if indicator:
		indicator.visible = true
