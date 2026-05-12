extends CanvasLayer

@onready var my_info_label = $Control/MarginContainer/HBoxContainer/MyInfoLabel
@onready var others_container = $Control/MarginContainer/HBoxContainer/OthersContainer

func update_display(my_id: int, players_nodes: Dictionary):
	# 1. Gestione Info Personali (Il tuo giocatore)
	if my_id != -1 and players_nodes.has(my_id):
		var p_node = players_nodes[my_id]
		
		# Recupero statistiche base
		var my_hp = p_node.get_meta("hp", 100)
		var my_gold = p_node.get_meta("gold", 0)
		
		# Recupero equipaggiamento completo
		var w_name = p_node.get_meta("weapon_name", "Pugni")
		var w_dmg  = p_node.get_meta("weapon_damage", 0)
		var a_name = p_node.get_meta("armor_name", "Nessuna")
		var a_def  = p_node.get_meta("armor_defense", 0)
		var i_name = p_node.get_meta("item_name", "Nessuno")
		
		# Visualizzazione dettagliata per te
		my_info_label.text = "TU [ID: %s] | HP: %s | GOLD: %s\nWEAPON: %s (%s ATK) | ARMOR: %s (%s DEF) | ITEM: %s" % [
			my_id, my_hp, my_gold, w_name, w_dmg, a_name, a_def, i_name
		]
	else:
		my_info_label.text = "In attesa di dati giocatore..."

	# 2. Gestione Altri Giocatori
	# Pulisce i vecchi nodi prima di aggiornare la lista
	for child in others_container.get_children():
		child.queue_free()

	for id in players_nodes:
		if id == my_id: continue 
		
		var p_node = players_nodes[id]
		
		# Recupero tutte le informazioni anche per gli altri giocatori
		var p_hp = p_node.get_meta("hp", 100)
		var p_gold = p_node.get_meta("gold", 0)
		var w_name = p_node.get_meta("weapon_name", "Pugni")
		var w_dmg  = p_node.get_meta("weapon_damage", 0)
		var a_name = p_node.get_meta("armor_name", "Nessuna")
		var a_def  = p_node.get_meta("armor_defense", 0)
		var i_name = p_node.get_meta("item_name", "Nessuno")
		
		var label = Label.new()
		# Visualizzazione dettagliata per gli altri (P1, P2, ecc.)
		# Ho aggiunto un "a capo" (\n) per rendere le info leggibili anche nel container
		label.text = "P%s | HP: %s | GOLD: %s\nWEAPON: %s (%s ATK) | ARMOR: %s (%s DEF) | ITEM: %s" % [
			id, p_hp, p_gold, w_name, w_dmg, a_name, a_def, i_name
		]
		
		# Aggiungiamo un po' di separazione visiva tra i giocatori
		label.add_theme_constant_override("line_spacing", 2)
		others_container.add_child(label)
