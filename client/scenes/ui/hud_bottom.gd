extends CanvasLayer

# Riferimento al contenitore orizzontale (HBoxContainer)
# Assicurati che il percorso corrisponda alla tua gerarchia nella scena
@onready var rooms_container = $BottomPanel/MarginContainer/RoomsContainer

func _ready():
	# L'HUD deve essere invisibile all'inizio della partita
	visible = false

# Questa funzione viene chiamata dal GameManager.gd
# rooms_array contiene stringhe tipo ["NORTH:FIGHT:not_completed", "SOUTH:BOSS:not_completed"]
func display_decision(rooms_array: Array):
	# 1. Pulisci le opzioni della stanza precedente
	for child in rooms_container.get_children():
		child.queue_free()
	
	# 2. Cicla attraverso le opzioni inviate dal server
	for room_string in rooms_array:
		if room_string == "": continue
		
		# Dividiamo la stringa usando i due punti ":"
		var tokens = room_string.split(":")
		
		# Verifichiamo che ci siano tutti e 3 i parametri (direzione:tipo:stato)
		if tokens.size() == 3:
			var direction = tokens[0].to_upper()
			var type = tokens[1].to_upper()
			var status = tokens[2]
			
			# Creiamo un nuovo Label per questa scelta
			var label = Label.new()
			
			# Formattazione del testo richiesta
			label.text = "%s [%s] %s" % [direction, type, status]
			
			# Stile e Allineamento
			label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			label.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
			label.size_flags_horizontal = Control.SIZE_EXPAND_FILL # Distribuisce equamente nello spazio
			
			# Colori dinamici in base al tipo di stanza
			_apply_room_style(label, type, status)
			
			# Aggiungiamo la scritta al contenitore dell'HUD
			rooms_container.add_child(label)
	
	# 3. Rendiamo visibile l'HUD
	visible = true

# Funzione per nascondere l'HUD (da chiamare quando il giocatore effettua una scelta)
func hide_hud():
	visible = false

# Funzione interna per abbellire le scritte in base al contenuto
func _apply_room_style(label: Label, type: String, status: String):
	if status == "completed":
		label.add_theme_color_override("font_color", Color.GRAY) # Grigio se già fatta
	else:
		match type:
			"FIGHT":
				label.add_theme_color_override("font_color", Color.INDIAN_RED)
			"TREASURE":
				label.add_theme_color_override("font_color", Color.GOLD)
			"BOSS":
				label.add_theme_color_override("font_color", Color.CRIMSON)
			_:
				label.add_theme_color_override("font_color", Color.WHITE)
