extends Node2D

# Caricamento della scena del giocatore
@onready var player_scene = preload("res://characters/player/player.tscn")

# Riferimento all'HUD, al RoomContainer, HudBottom e al nuovo PlayersContainer
# get_parent() serve perché GameManager è fratello di questi nodi
@onready var hud_top = get_parent().get_node_or_null("HudTop")
@onready var room_container = get_parent().get_node_or_null("RoomContainer")
@onready var hud_bottom = get_parent().get_node_or_null("HudBottom")

@onready var players_container = get_parent().get_node_or_null("PlayersContainer")

signal load_room(coordinate)

# Dizionario per mappare: ID (int) -> Nodo del Giocatore
var players_nodes = {}

# ID univoco del client corrente (assegnato dal server)
var my_id: int = -1

var active_popup_canvas: CanvasLayer = null
var active_message_label: Label = null
var dialog_timer: Timer = null

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
			if args.size() == 5:
				_handle_player_info(args)

		"HP_PLAYER":
			if args.size() >= 2:
				_handle_hp_update(args)
				
		"MESSAGE":
			var text = " ".join(args)
			_show_message_popup(text)
				
		"MAKE_DECISION":
			if args.size() > 0:
				_handle_make_decision(args)
				
		"LOAD_ROOM":
			if args.size() == 1:
				var directions: String = args[0]
				_handle_loading_room(directions)
				
	# Aggiorna l'interfaccia ad ogni pacchetto ricevuto
	if hud_top and hud_top.has_method("update_display"):
		hud_top.update_display(my_id, players_nodes)

# --- LOGICA DI SPAWN E MOVIMENTO ---

func _spawn(args):
	var id = args[0].to_int()
	var pos = Vector2(args[1].to_float(), args[2].to_float())

	if players_nodes.has(id) and is_instance_valid(players_nodes[id]):
		return

	var p = player_scene.instantiate()
	p.name = "Player_%d" % id
	p.position = pos
	p.set_meta("hp", 100)
	p.set_meta("gold", 0)

	if players_container:
		players_container.add_child(p)
	else:
		# Fallback se dimentichi di creare il nodo nell'editor
		add_child(p)
		
	players_nodes[id] = p
	
	if id == my_id:
		_setup_local_player_visuals(p)

func _move(args):
	var id = args[0].to_int()
	var pos = Vector2(args[1].to_float(), args[2].to_float())
	
	# Controllo di sicurezza: se il nodo non è valido, rimuovilo dalla memoria
	if players_nodes.has(id) and not is_instance_valid(players_nodes[id]):
		players_nodes.erase(id)
		
	if players_nodes.has(id):
		players_nodes[id].global_position = pos

func _remove(args):
	var id = args[0].to_int()
	if players_nodes.has(id):
		if is_instance_valid(players_nodes[id]):
			players_nodes[id].queue_free()
		players_nodes.erase(id)

func _handle_player_info(args):
	var id = args[0].to_int()
	var hp = args[1].to_int()
	var pos = Vector2(args[2].to_float(), args[3].to_float())
	var gold = args[4].to_int()

	# Controllo di sicurezza
	if players_nodes.has(id) and not is_instance_valid(players_nodes[id]):
		players_nodes.erase(id)

	if players_nodes.has(id):
		players_nodes[id].global_position = pos
	else:
		_spawn([args[0], args[2], args[3]])
		
	players_nodes[id].set_meta("hp", hp)
	players_nodes[id].set_meta("gold", gold)
		

func _handle_hp_update(args):
	var id = args[0].to_int()
	var hp = args[1].to_int()
	
	if players_nodes.has(id) and not is_instance_valid(players_nodes[id]):
		players_nodes.erase(id)
		
	if players_nodes.has(id):
		players_nodes[id].set("hp", hp)

func _show_message_popup(message_text: String):
		MessagePopup.show_message(message_text)

func _handle_make_decision(args: Array):
	if hud_bottom and hud_bottom.has_method("show_decision_menu"):
		hud_bottom.show_decision_menu(args)
	else:
		print("ATTENZIONE: Nodo HudBottom non trovato o metodo display_decision mancante!")

func _handle_loading_room(directions: String ): 
	load_room.emit(directions)

# --- ESTETICA ---

func _setup_local_player_visuals(player_node):
	# Attiva il triangolo indicatore creato nella scena player.tscn
	var indicator = player_node.get_node_or_null("MyPlayerIndicator")
	if indicator:
		indicator.visible = true
