extends CanvasLayer

@onready var message_label = $ColorRect/CenterContainer/PanelContainer/MarginContainer/VBoxContainer/MessageLabel
@onready var ok_button = $ColorRect/CenterContainer/PanelContainer/MarginContainer/VBoxContainer/OkButton
@onready var close_timer = $CloseTimer
@onready var vbox_container = $ColorRect/CenterContainer/PanelContainer/MarginContainer/VBoxContainer

var btn_play: Button = null
var btn_leave: Button = null

func _ready():
	# Colleghiamo i segnali via codice (puoi farlo anche dall'editor)
	ok_button.pressed.connect(close_popup)
	close_timer.timeout.connect(close_popup)
	
	# Assicuriamoci che all'avvio del gioco sia nascosto
	hide()

func show_message(text: String):
	# Sistema antispam e accodamento messaggi
	if visible:
		if not text in message_label.text:
			message_label.text += "\n" + text
	else:
		message_label.text = text
		show() # Mostra il popup
	
	# Riavvia il timer a ogni nuovo messaggio
	close_timer.start(15.0)
	
	# Ruba il focus al frame successivo
	ok_button.call_deferred("grab_focus")

func close_popup():
	hide() # Nascondiamo invece di distruggere
	message_label.text = ""
	ok_button.show()
	if btn_play != null:
		btn_play.hide()
		btn_leave.hide()
	close_timer.stop()

func show_end_decision_menu(text: String):
	message_label.text = text
	ok_button.hide()
	
	if btn_play == null:
		btn_play = Button.new()
		btn_play.text = "RIGIOCA"
		btn_play.custom_minimum_size = Vector2(200, 60)
		btn_play.pressed.connect(_on_end_decision.bind("STAY"))
		vbox_container.add_child(btn_play)
		
		btn_leave = Button.new()
		btn_leave.text = "ABBANDONA"
		btn_leave.custom_minimum_size = Vector2(200, 60)
		btn_leave.pressed.connect(_on_end_decision.bind("LEAVE"))
		vbox_container.add_child(btn_leave)
	else:
		btn_play.show()
		btn_leave.show()
		
	show()
	close_timer.start(15.0)
	btn_play.call_deferred("grab_focus")

func _on_end_decision(choice: String):
	close_popup()
	var message = "SEND_DECISION " + choice
	print("Inviando al server: ", message)
	NetworkManager.send_to_server(message)
