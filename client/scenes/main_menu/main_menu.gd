extends Control

func _ready():
	pass
	
func _on_createGame_button_pressed() -> void:
	NetworkManager.send_to_matchmaker("CREATE")
	get_tree().change_scene_to_file("res://scenes/game.tscn")
	
func _on_joinGame_button_pressed() -> void:
	get_tree().change_scene_to_file("res://scenes/main_menu/join_menu/join_menu.tscn")

func _on_exit_button_pressed() -> void:
	get_tree().quit()
