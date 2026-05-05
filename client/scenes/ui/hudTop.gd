extends CanvasLayer

@onready var my_info_label = $Control/MarginContainer/HBoxContainer/MyInfoLabel
@onready var others_container = $Control/MarginContainer/HBoxContainer/OthersContainer

func update_display(my_id: int, players_nodes: Dictionary):
	# 1. Gestione Info Personali
	if my_id != -1 and players_nodes.has(my_id):
		var p_node = players_nodes[my_id]
		# Usiamo get_meta, passando il valore di default (100 per hp, 0 per oro)
		var my_hp = p_node.get_meta("hp", 100)
		var my_gold = p_node.get_meta("gold", 0)
		
		my_info_label.text = "TU [ID: %d] | HP: %d | GOLD: %d" % [my_id, my_hp, my_gold]
	else:
		my_info_label.text = "In attesa di dati..."

	# 2. Gestione Altri Giocatori
	for child in others_container.get_children():
		child.queue_free()

	for id in players_nodes:
		if id == my_id: continue 
		
		var p_node = players_nodes[id]
		var p_hp = p_node.get_meta("hp", 100)
		var p_gold = p_node.get_meta("gold", 0)
		
		var label = Label.new()
		label.text = "Player %d | HP: %d | GOLD: %d" % [id, p_hp, p_gold]
		label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
		others_container.add_child(label)
