extends CanvasLayer

@onready var buttons_container = $BottomPanel/MarginContainer/RoomButtonsContainer

func _ready():
	# L'HUD deve essere invisibile di default
	hide()

# =====================================================================
# --- SCELTA DELLA STANZA ---
# =====================================================================
func show_room_decision_menu(doors: Array):
	for child in buttons_container.get_children():
		child.queue_free()
	show()
	var first_button = null
	for door_info in doors:
		var parts = door_info.split(":")
		if parts.size() == 3:
			var direction = parts[0]
			var room_type = parts[1]
			var state = parts[2]
			
			var btn = Button.new()
			btn.text = "Direction: %s\nType: %s\nState: %s" % [direction, room_type, state]
			btn.custom_minimum_size = Vector2(100, 200)
			btn.pressed.connect(_on_room_decision_pressed.bind(direction))
			buttons_container.add_child(btn)
			if first_button == null:
				first_button = btn
	if first_button != null:
		first_button.grab_focus()

func _on_room_decision_pressed(direction: String):
	hide()
	var message = "SEND_DECISION " + direction
	print("Inviando al server: ", message)
	NetworkManager.send_to_server(message)

# =====================================================================
# --- SCELTA DELL'INVENTARIO ---
# =====================================================================
func show_inventory_decision_menu():
	for child in buttons_container.get_children():
		child.queue_free()
	show()
	
	var btn_yes = Button.new()
	btn_yes.text = "SI"
	btn_yes.custom_minimum_size = Vector2(100, 200)
	btn_yes.pressed.connect(_on_inventory_decision_pressed.bind("T"))
	buttons_container.add_child(btn_yes)
	
	var btn_no = Button.new()
	btn_no.text = "NO"
	btn_no.custom_minimum_size = Vector2(100, 200)
	btn_no.pressed.connect(_on_inventory_decision_pressed.bind("F"))
	buttons_container.add_child(btn_no)
	
	btn_yes.grab_focus()

func _on_inventory_decision_pressed(choice: String):
	hide()
	StaticPopup.hide_popup()
	var message = "SEND_DECISION " + choice
	print("Inviando al server: ", message)
	NetworkManager.send_to_server(message)

# =====================================================================
# --- CONTROLLO INVENTARIO ---
# =====================================================================
func _player_has_item() -> bool:
	if not GameManager.players_nodes.has(GameManager.my_id): return false
	var player = GameManager.players_nodes[GameManager.my_id]
	var item = player.get_meta("item_name", "None")
	var item_str = str(item).strip_edges()
	return item_str != "None" and item_str != ""

# =====================================================================
# --- FUNZIONI DI SUPPORTO PER IL RANGE DELLE ARMI ---
# =====================================================================
func _is_monster_in_range(monster_node: Node2D) -> bool:
	if not GameManager.players_nodes.has(GameManager.my_id): return false
	var player = GameManager.players_nodes[GameManager.my_id]
	
	var w_range_meta = player.get_meta("weapon_range")
	if w_range_meta == null: return false
	
	# 1. FIX STRINGA C: strip_edges() rimuove tutti gli \n, \r e gli spazi vuoti!
	var clean_range_string = str(w_range_meta).strip_edges()
	var weapon_range = clean_range_string.to_float()
	
	# Calcoliamo la distanza reale in pixel
	var dist = player.position.distance_to(monster_node.position)
	
	# Stampiamo nella console di Godot i valori esatti per verificare che tutto funzioni
	print("[DEBUG RANGE] Distanza reale: ", dist, " | Range calcolato: ", weapon_range)
	
	# 2. FIX DECIMALI: Aggiungiamo 2 pixel di tolleranza (es: 50.001 diventerà valido per un range 50)
	return dist <= (weapon_range + 2.0)

func _has_any_monster_in_range() -> bool:
	for m in GameManager.monsters_nodes.values():
		if is_instance_valid(m) and m.get("is_alive") != false and _is_monster_in_range(m):
			return true
	return false
	
# Controlla se la casella di destinazione è occupata da un mostro
func _is_tile_occupied_by_monster(target_pos: Vector2) -> bool:
	for m in GameManager.monsters_nodes.values():
		if is_instance_valid(m) and m.get("is_alive") != false:
			# Usiamo 5.0 pixel di tolleranza nel caso in cui i float non siano perfetti
			if m.position.distance_to(target_pos) < 5.0: 
				return true
	return false

func _focus_first_button():
	for child in buttons_container.get_children():
		if child is Button and not child.disabled:
			child.grab_focus()
			return

# =====================================================================
# --- LOGICA DI TURNO E MOVIMENTO DIRETTO (SPOSTATI) ---
# =====================================================================
func show_turn_decision_menu():
	for child in buttons_container.get_children():
		child.queue_free()
		
	show()
	
	# 1. Bottone SPOSTATI 
	var btn_move = Button.new()
	btn_move.text = "SPOSTATI"
	btn_move.custom_minimum_size = Vector2(100, 200)
	btn_move.pressed.connect(_on_spostati_clicked)
	buttons_container.add_child(btn_move)
	
	# 2. Bottone ATTACCA (Con controllo della distanza!)
	var btn_attack = Button.new()
	btn_attack.custom_minimum_size = Vector2(100, 200)
	
	if _has_any_monster_in_range():
		btn_attack.text = "ATTACCA"
		btn_attack.disabled = false
		btn_attack.pressed.connect(_on_attacca_clicked)
	else:
		btn_attack.text = "ATTACCA\n(Fuori range)"
		btn_attack.disabled = true
		
	buttons_container.add_child(btn_attack)
	
	# 3. Bottone USA OGGETTO
	var btn_item = Button.new()
	btn_item.custom_minimum_size = Vector2(100, 200)
	
	if _player_has_item():
		btn_item.text = "USA OGGETTO"
		btn_item.disabled = false
		btn_item.pressed.connect(_on_turn_decision_pressed.bind("USE_ITEM"))
	else:
		btn_item.text = "USA OGGETTO\n(Nessuno)"
		btn_item.disabled = true
	
	buttons_container.add_child(btn_item)
	
	_focus_first_button()

func _on_turn_decision_pressed(choice: String):
	hide()
	var message = "SEND_DECISION " + choice
	print("Inviando al server: ", message)
	NetworkManager.send_to_server(message)

func _on_spostati_clicked():
	for child in buttons_container.get_children():
		child.queue_free()
		
	var current_pos = Vector2.ZERO
	if GameManager.players_nodes.has(GameManager.my_id) and is_instance_valid(GameManager.players_nodes[GameManager.my_id]):
		current_pos = GameManager.players_nodes[GameManager.my_id].position
	else:
		return # Sicurezza: se non trova il player, non mostriamo nulla.

	var step = 50
	var max_x = 450
	var max_y = 200

	# SU
	if current_pos.y - step >= 0:
		var target = current_pos + Vector2(0, -step)
		if not _is_tile_occupied_by_monster(target):
			_create_direction_button("SU", target)
			
	# GIU
	if current_pos.y + step <= max_y:
		var target = current_pos + Vector2(0, step)
		if not _is_tile_occupied_by_monster(target):
			_create_direction_button("GIU", target)
			
	# SINISTRA
	if current_pos.x - step >= 0:
		var target = current_pos + Vector2(-step, 0)
		if not _is_tile_occupied_by_monster(target):
			_create_direction_button("SINISTRA", target)
			
	# DESTRA
	if current_pos.x + step <= max_x:
		var target = current_pos + Vector2(step, 0)
		if not _is_tile_occupied_by_monster(target):
			_create_direction_button("DESTRA", target)

	# Se tutti i bottoni sono stati bloccati dai mostri o dai muri
	if buttons_container.get_child_count() == 0:
		var btn_back = Button.new()
		btn_back.text = "BLOCCATO\n(Indietro)"
		btn_back.custom_minimum_size = Vector2(100, 200)
		btn_back.pressed.connect(show_turn_decision_menu)
		buttons_container.add_child(btn_back)

	_focus_first_button()

func _create_direction_button(dir_name: String, target_pos: Vector2):
	var btn = Button.new()
	btn.text = dir_name
	btn.custom_minimum_size = Vector2(100, 200)
	btn.pressed.connect(_on_direction_selected.bind(target_pos))
	buttons_container.add_child(btn)

func _on_direction_selected(target_pos: Vector2):
	hide() 
	var message = "SEND_DECISION MOVE %d %d" % [round(target_pos.x), round(target_pos.y)]
	print("Inviando al server: ", message)
	NetworkManager.send_to_server(message)

# =====================================================================
# --- LOGICA DI SELEZIONE BERSAGLIO (ATTACCA) ---
# =====================================================================
func _on_attacca_clicked():
	for child in buttons_container.get_children():
		child.queue_free()
		
	var monster_id = 0
	
	if GameManager.monsters_nodes.is_empty():
		var btn_back = Button.new()
		btn_back.text = "NESSUN BERSAGLIO\n(Indietro)"
		btn_back.custom_minimum_size = Vector2(100, 200)
		btn_back.pressed.connect(show_turn_decision_menu)
		buttons_container.add_child(btn_back)
		btn_back.grab_focus()
		return

	for m_name in GameManager.monsters_nodes.keys():
		var m_node = GameManager.monsters_nodes[m_name]
		var btn = Button.new()
		btn.custom_minimum_size = Vector2(100, 200)
		
		# 1. Controlliamo se il mostro è morto
		if m_node.get("is_alive") == false:
			btn.text = "%s\n(Morto)" % str(m_name)
			btn.disabled = true
			
		# 2. Controlliamo se è a tiro
		elif _is_monster_in_range(m_node):
			btn.text = str(m_name)
			btn.disabled = false
			btn.pressed.connect(_on_attack_target_selected.bind(monster_id))
			
		# 3. Altrimenti è troppo lontano
		else:
			btn.text = "%s\n(Troppo lontano)" % str(m_name)
			btn.disabled = true
			
		buttons_container.add_child(btn)
		
		# Adesso l'ID incrementa SEMPRE, mantenendosi sincronizzato col server
		monster_id += 1
		
	_focus_first_button()

func _on_attack_target_selected(target_id: int):
	hide() 
	var message = "SEND_DECISION ATTACK %d" % target_id
	print("Inviando al server: ", message)
	NetworkManager.send_to_server(message)
