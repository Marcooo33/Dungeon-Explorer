extends CanvasLayer

# Manteniamo solo il riferimento al nodo che mostra il testo
@onready var message_label = $ColorRect/CenterContainer/PanelContainer/MarginContainer/VBoxContainer/MessageLabel

func _ready():
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

# Ho aggiunto una funzione di utilità nel caso in cui tu debba 
# nascondere il popup via codice (es. quando il server risponde o inizia il gioco)
func hide_popup():
	hide() 
	message_label.text = ""
