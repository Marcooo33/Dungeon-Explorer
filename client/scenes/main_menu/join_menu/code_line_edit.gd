extends LineEdit

func _ready():
	# Richiede il focus appena la scena è pronta
	grab_focus()


func _on_text_changed(new_text: String) -> void:
	# Memorizziamo la posizione del cursore per evitare che salti alla fine
	var caret_pos = get_caret_column()
	# Trasformiamo il testo in maiuscolo
	text = new_text.to_upper()
	# Ripristiniamo la posizione del cursore
	set_caret_column(caret_pos)
