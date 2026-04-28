extends CanvasLayer

@onready var buttons_container = $BottomPanel/MarginContainer/RoomButtonsContainer

func _ready():
	# L'HUD deve essere invisibile di default
	hide()

# Funzione da chiamare quando arriva il messaggio MAKE_DECISION
# 'doors' sarà un Array di stringhe come ["NORTH:FIGHT:not_completed", "SOUTH:TREASURE:not_completed"]
func show_decision_menu(doors: Array):
	# 1. Pulisci eventuali bottoni rimasti dai turni precedenti
	for child in buttons_container.get_children():
		child.queue_free()
	
	# 2. Mostra l'HUD
	show()
	
	# 3. Crea dinamicamente i bottoni
	var first_button = null
	for door_info in doors:
		var parts = door_info.split(":")
		
		# Assicuriamoci che il formato sia corretto (3 parti)
		if parts.size() == 3:
			var direction = parts[0]
			var room_type = parts[1]
			var state = parts[2]
			
			var btn = Button.new()
			# Formattiamo il testo del bottone su più righe
			btn.text = "Direction: %s Type: %s State: %s" % [direction, room_type, state]
			btn.custom_minimum_size = Vector2(100, 200) # Dai una dimensione minima
			
			# Collega il segnale 'pressed' del bottone alla nostra funzione per gestire la scelta
			# Passiamo 'direction' come argomento per sapere quale bottone è stato premuto
			btn.pressed.connect(_on_button_pressed.bind(direction))
			
			buttons_container.add_child(btn)
			
			if first_button == null:
				first_button = btn

	# 4. Assegna il focus al primo bottone (utile per muoversi con le freccette della tastiera)
	if first_button != null:
		first_button.grab_focus()

func _on_button_pressed(direction: String):
	# Nascondi l'HUD quando una scelta è stata effettuata
	hide()
	
	# Formatta il messaggio da inviare
	var message = "SEND_DECISION " + direction
	print("Inviando al server: ", message)
	NetworkManager.send_to_server(message)
