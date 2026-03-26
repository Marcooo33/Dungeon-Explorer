extends Control


func _on_join_button_pressed() -> void:
	var input_code = $Panel/VBoxContainer/CodeLineEdit.text
	NetworkManager.send_to_matchmaker("JOIN " + input_code)
	
func _ready():
	# Opzionale: connettiti al segnale per cambiare scena solo quando la porta è pronta
	NetworkManager.game_found.connect(_on_game_found)
	
func _on_game_found():
	get_tree().change_scene_to_file("res://scenes/game.tscn")
