extends CanvasLayer

@onready var buttons_container = $BottomPanel/MarginContainer/RoomButtonsContainer

func _ready():
	# L'HUD deve essere invisibile di default
	hide()

func show_room_decision_menu(doors: Array):
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
			btn.text = "Direction: %s\nType: %s\nState: %s" % [direction, room_type, state]
			btn.custom_minimum_size = Vector2(100, 200) # Dai una dimensione minima
			
			# Collega il segnale 'pressed' del bottone alla nostra funzione per gestire la scelta
			# Passiamo 'direction' come argomento per sapere quale bottone è stato premuto
			btn.pressed.connect(_on_room_decision_pressed.bind(direction))
			
			buttons_container.add_child(btn)
			
			if first_button == null:
				first_button = btn
	# 4. Assegna il focus al primo bottone (utile per muoversi con le freccette della tastiera)
	if first_button != null:
		first_button.grab_focus()

func _on_room_decision_pressed(direction: String):
	# Nascondi l'HUD quando una scelta è stata effettuata
	hide()
	
	# Formatta il messaggio da inviare
	var message = "SEND_DECISION " + direction
	print("Inviando al server: ", message)
	NetworkManager.send_to_server(message)
	

func show_inventory_decision_menu():
	# 1. Pulisci eventuali bottoni rimasti
	for child in buttons_container.get_children():
		child.queue_free()
		
	# 2. Mostra l'HUD
	show()
	
	# 3. Crea il bottone SI
	var btn_yes = Button.new()
	btn_yes.text = "SI"
	btn_yes.custom_minimum_size = Vector2(100, 200)
	# Passiamo "true" alla funzione di callback
	btn_yes.pressed.connect(_on_inventory_decision_pressed.bind("T"))
	buttons_container.add_child(btn_yes)
	
	# 4. Crea il bottone NO
	var btn_no = Button.new()
	btn_no.text = "NO"
	btn_no.custom_minimum_size = Vector2(100, 200)
	# Passiamo "false" alla funzione di callback
	btn_no.pressed.connect(_on_inventory_decision_pressed.bind("F"))
	buttons_container.add_child(btn_no)
	
	# 5. Assegna il focus al bottone SI di default per la navigazione da tastiera/pad
	btn_yes.grab_focus()

func _on_inventory_decision_pressed(choice: String):
	# Nascondi l'HUD e il Popup quando una scelta è stata effettuata
	hide()
	StaticPopup.hide_popup()
	
	# Formatta il messaggio da inviare ("SEND_DECISION true" oppure "SEND_DECISION false")
	var message = "SEND_DECISION " + choice
	print("Inviando al server: ", message)
	NetworkManager.send_to_server(message)
