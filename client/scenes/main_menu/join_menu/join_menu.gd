extends Control


func _on_join_button_pressed() -> void:
	var input_code = $VBoxContainer/CodeLineEdit.text
	if NetworkManager.matchmaker_socket.get_status() == StreamPeerTCP.STATUS_CONNECTED:
		NetworkManager.send_to_matchmaker("JOIN " + input_code)
	else:
		print("Impossibile creare partita: Matchmaker non connesso.")
	
func _ready():
	# Opzionale: connettiti al segnale per cambiare scena solo quando la porta è pronta
	NetworkManager.connect("game_started", _on_game_ready)
	
func _on_game_ready(_port):
	get_tree().change_scene_to_file("res://scenes/game.tscn")
