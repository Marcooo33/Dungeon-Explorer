extends Node2D

# Caricamento della scena del giocatore
@onready var player_scene = preload("res://characters/player/player.tscn")
@onready var monster_scene = preload("res://characters/monster/Monster.tscn")
@onready var players_container: Node2D
@onready var monsters_container: Node2D

# Riferimento all'HUD, al RoomContainer, HudBottom e al nuovo PlayersContainer
# get_parent() serve perché GameManager è fratello di questi nodi
@onready var hud_top: CanvasLayer 
@onready var room_container: Node2D
@onready var hud_bottom: CanvasLayer


signal load_room(coordinate)

var players_nodes = {}
var monsters_nodes = {}

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
			_handle_player_info(args)
			
		"MONSTER_INFO":
			_handle_monster_info(args)
			
		"HP_PLAYER":
			if args.size() >= 2:
				_handle_hp_update(args)
				
		"MESSAGE":
			var text = " ".join(args)
			_show_message_popup(text)
				
		"MAKE_ROOM_DECISION":
			if args.size() > 0:
				_handle_make_room_decision(args)
				
		"MAKE_INVENTORY_DECISION":
			if args.size() > 0:
				var text = " ".join(args)
				StaticPopup.show_message(text)
				_handle_make_inventory_decision()
			
		"MAKE_TURN_DECISION":
			_handle_make_turn_decision(args)
		
		"LOAD_ROOM":
			if args.size() == 1:
				var directions: String = args[0]
				_handle_loading_room(directions)
				
		"GAME_OVER":
			_show_message_popup("TUTTI I GIOCATORI SONO MORTI!\nGAME OVER")
			if hud_bottom:
				hud_bottom.hide() # Nasconde i bottoni delle azioni
				
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
	if hp <= 0:
		if players_nodes.has(id):
			if is_instance_valid(players_nodes[id]):
				players_nodes[id].queue_free() # Rimuove il nodo dallo schermo
			players_nodes.erase(id)
		return
	
	var pos = Vector2(args[2].to_float(), args[3].to_float())
	var gold = args[4].to_int()
	
	var weapon_data = args[5].split(":")
	print("[DEBUG] ", weapon_data)
	var weapon_name = str(weapon_data[0])
	var weapon_damage = str(weapon_data[1])
	var weapon_range = str(weapon_data[2])
	
	var armor_data = args[6].split(":")
	var armor_name = str(armor_data[0])
	var armor_defense = str(armor_data[1])
	
	var item_name = str(args[7])
	

	# Controllo di sicurezza
	if players_nodes.has(id) and not is_instance_valid(players_nodes[id]):
		players_nodes.erase(id)

	if players_nodes.has(id):
		players_nodes[id].global_position = pos
	else:
		_spawn([args[0], args[2], args[3]])
		
	players_nodes[id].set_meta("hp", hp)
	players_nodes[id].set_meta("gold", gold)
	players_nodes[id].set_meta("weapon_name", weapon_name)
	players_nodes[id].set_meta("weapon_damage", weapon_damage)
	players_nodes[id].set_meta("weapon_range", weapon_range)
	players_nodes[id].set_meta("armor_name", armor_name)
	players_nodes[id].set_meta("armor_defense", armor_defense)
	players_nodes[id].set_meta("item_name", item_name)
	
	
func _handle_monster_info(args: Array):

	var m_id = args[0]
	var m_name = args[1]
	var m_hp = args[2].to_int()
	var m_alive = args[3] == "1"
	var m_pos = Vector2(args[4].to_float(), args[5].to_float()) # Assumendo tile da 32px
	var m_armor_data = args[6] # "armatura:difesa"

	# Se il mostro non esiste ancora, lo creiamo
	if not monsters_nodes.has(m_name):
		var new_monster = monster_scene.instantiate()
		monsters_container.add_child(new_monster)
		monsters_nodes[m_name] = new_monster
		
		# Estraiamo l'armatura e passiamola allo script del mostro
		var armor_parts = m_armor_data.split(":")
		var armor_type = armor_parts[0]
		if new_monster.has_method("setup"):
			# PASSIAMO SIA IL NOME CHE L'ARMATURA
			new_monster.setup(m_name, armor_type)

	var monster_node = monsters_nodes[m_name]
	
	# Aggiorniamo posizione e stato
	monster_node.position = m_pos
	monster_node.is_alive = m_alive
	
	# Se il mostro è morto nel server, chiamiamo la funzione di morte nel client
	if not m_alive:
		if monster_node.has_method("die"):
			monster_node.die()
			monster_node.queue_free() # Rimuove fisicamente il nodo dallo schermo
		monsters_nodes.erase(m_name) # Lo togliamo dal dizionario dopo la morte
	
	
func _handle_hp_update(args):
	var id = args[0].to_int()
	var hp = args[1].to_int()
	
	if players_nodes.has(id) and not is_instance_valid(players_nodes[id]):
		players_nodes.erase(id)
		
	if players_nodes.has(id):
		players_nodes[id].set("hp", hp)

func _show_message_popup(message_text: String):
		TimedPopup.show_message(message_text)

func _handle_make_room_decision(args: Array):
	if hud_bottom and hud_bottom.has_method("show_room_decision_menu"):
		hud_bottom.show_room_decision_menu(args)
	else:
		print("ATTENZIONE: Nodo HudBottom non trovato o metodo display_decision mancante!")

func _handle_make_inventory_decision():
	if hud_bottom and hud_bottom.has_method("show_inventory_decision_menu"):
		hud_bottom.show_inventory_decision_menu()
	else:
		print("ATTENZIONE: Nodo HudBottom non trovato o metodo display_decision mancante!")
		
		
func _handle_make_turn_decision(args: Array):
	if hud_bottom and hud_bottom.has_method("show_turn_decision_menu"):
		hud_bottom.show_turn_decision_menu()
	else:
		print("ATTENZIONE: Nodo HudBottom non trovato o metodo show_turn_decision_menu mancante!")

func _handle_loading_room(directions: String ): 
	load_room.emit(directions)
# --- ESTETICA ---

func _setup_local_player_visuals(player_node):
	# Attiva il triangolo indicatore creato nella scena player.tscn
	var indicator = player_node.get_node_or_null("MyPlayerIndicator")
	if indicator:
		indicator.visible = true
