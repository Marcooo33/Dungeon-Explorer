extends Control

@onready var panel = $Panel
@onready var code_input = $Panel/VBoxContainer/CodeLineEdit
@onready var waiting_overlay = $WaitingOverlay
@onready var error_dialog = $ErrorDialog

func _ready():
	# Connessione ai segnali globali del NetworkManager
	NetworkManager.join_accepted.connect(_on_join_accepted)
	NetworkManager.join_rejected.connect(_on_join_rejected)
	NetworkManager.error_received.connect(_on_error_received)
	
	waiting_overlay.visible = false

func _on_join_button_pressed():
	var code = code_input.text.strip_edges()
	if code != "":
		# Disabilitiamo l'interfaccia durante l'attesa
		panel.visible = false
		waiting_overlay.visible = true
		
		NetworkManager.send_to_server("JOIN " + code)

func _on_error_received(msg: String):
	print("UI: Ricevuto errore dal server -> ", msg)
	
	# Sblocchiamo l'interfaccia e nascondiamo l'overlay di attesa
	waiting_overlay.visible = false
	panel.visible = true
	
	# Gestione grafica differenziata in base al contenuto del messaggio
	if "NOT_FOUND" in msg:
		error_dialog.dialog_text = "Game not found. Please check the code."
		error_dialog.popup_centered()
	elif "FULL" in msg:
		error_dialog.dialog_text = "Game is full"
		error_dialog.popup_centered()
	elif "STARTED" in msg:
		error_dialog.dialog_text = "Game is started"
		error_dialog.popup_centered()
	else:
		error_dialog.dialog_text = "A connection error occurred"
		error_dialog.popup_centered()
	
	# Puliamo l'input per permettere un nuovo inserimento
	code_input.clear()

func _on_join_accepted():
	# Successo: il cambio scena è già gestito qui
	get_tree().change_scene_to_file("res://scenes/rooms/lobby/Lobby.tscn")

func _on_join_rejected():
	# L'host ha cliccato "Rifiuta"
	waiting_overlay.visible = false
	panel.visible = true
	error_dialog.dialog_text = "The host rejected your join request"
	error_dialog.popup_centered()
