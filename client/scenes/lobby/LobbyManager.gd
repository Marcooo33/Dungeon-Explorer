extends Node2D

@onready var join_dialog = $JoinRequestDialog
@onready var start_button = $StartButton
@onready var room_code_label = $RoomCodeLabel
@onready var player_list_label = $PlayerListContainer/PlayerLabel

var join_queue: Array[int] = []

func _ready():
	# Connessioni ai segnali
	NetworkManager.join_request_received.connect(_on_join_request)
	
	if NetworkManager.has_signal("room_code_received"):
		NetworkManager.room_code_received.connect(_on_room_code_received)
	if NetworkManager.has_signal("player_list_updated"):
		NetworkManager.player_list_updated.connect(_on_player_list_updated)

	join_dialog.confirmed.connect(_accept)
	join_dialog.canceled.connect(_reject)
	start_button.pressed.connect(_start)

	# Sincronizzazione per evitare che l'UI rimanga vuota se i dati sono già arrivati
	if NetworkManager.current_room_code != "":
		_on_room_code_received(NetworkManager.current_room_code)
	if NetworkManager.current_players.size() > 0:
		_on_player_list_updated(NetworkManager.current_players)

# ---------------- AGGIORNAMENTO INTERFACCIA (UI) ----------------

func _on_room_code_received(code: String):
	if room_code_label:
		print("room_code received: %s", room_code_label.name)
		room_code_label.text = "Codice\n" + code

func _on_player_list_updated(players: Array):
	if player_list_label:
		var list_text = "GIOCATORI IN PARTITA:\n\n"
		
		for player_id_str in players:
			var clean_id = player_id_str.strip_edges()
			if clean_id != "":
				var real_id = clean_id.to_int()
				var display_number = real_id + 1 # Mostriamo sempre da 1 in poi
				
				# Formattazione lista semplice con trattino
				list_text += "- Giocatore %d\n" % display_number
					
		player_list_label.text = list_text

# ---------------- GESTIONE RICHIESTE INGRESSO (JOIN) ----------------

func _on_join_request(id: int):
	if not join_queue.has(id):
		join_queue.append(id)

	if not join_dialog.visible:
		_show()

func _show():
	if join_queue.size() > 0:
		var id = join_queue[0]
		# Manteniamo il +1 nel pop-up per logica visiva coerente
		join_dialog.dialog_text = "Il Giocatore %d vuole entrare" % (id + 1)
		join_dialog.popup_centered()

func _accept():
	var id = join_queue.pop_front()
	NetworkManager.send_to_server("ACCEPT %d" % id)
	call_deferred("_show")

func _reject():
	var id = join_queue.pop_front()
	NetworkManager.send_to_server("REJECT %d" % id)
	call_deferred("_show")

func _start():
	NetworkManager.send_to_server("START_GAME")
