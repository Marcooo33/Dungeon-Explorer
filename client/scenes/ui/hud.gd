extends CanvasLayer

@onready var my_info_label = $Control/MarginContainer/HBoxContainer/MyInfoLabel
@onready var others_container = $Control/MarginContainer/HBoxContainer/OthersContainer

# Aggiorna l'interfaccia basandosi sui dati correnti del GameManager
func update_display(my_id: int, players_nodes: Dictionary):
	# 1. Gestione Info Personali
	if my_id != -1 and players_nodes.has(my_id):
		# Qui assumiamo che il tuo script player.gd abbia una variabile 'hp'
		var my_hp = players_nodes[my_id].get("hp") if "hp" in players_nodes[my_id] else 100
		my_info_label.text = "TU [ID: %d] | HP: %d" % [my_id, my_hp]
	else:
		my_info_label.text = "In attesa di dati..."

	# 2. Gestione Altri Giocatori
	# Puliamo il container della destra per ricrearlo (metodo semplice)
	for child in others_container.get_children():
		child.queue_free()

	for id in players_nodes:
		if id == my_id: continue # Salta te stesso
		
		var p_node = players_nodes[id]
		var p_hp = p_node.get("hp") if "hp" in p_node else 100
		
		var label = Label.new()
		label.text = "Player %d | HP: %d" % [id, p_hp]
		label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		others_container.add_child(label)
