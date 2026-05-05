extends CanvasLayer

@onready var message_label = $ColorRect/CenterContainer/PanelContainer/MarginContainer/VBoxContainer/MessageLabel
@onready var ok_button = $ColorRect/CenterContainer/PanelContainer/MarginContainer/VBoxContainer/OkButton
@onready var close_timer = $CloseTimer

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
	close_timer.stop()
