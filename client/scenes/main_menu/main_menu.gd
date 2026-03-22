extends Control

func _on_createGame_button_pressed() -> void:
	if NetworkManager.matchmaker_socket.get_status() == StreamPeerTCP.STATUS_CONNECTED:
		NetworkManager.send_to_matchmaker("CREATE")
		# Suggerimento: non cambiare scena subito. 
		# Aspetta che il server ti confermi la creazione!
		# Puoi farlo collegandoti al segnale 'game_started'
	else:
		print("Impossibile creare partita: Matchmaker non connesso.")

func _ready():
	# Opzionale: connettiti al segnale per cambiare scena solo quando la porta è pronta
	NetworkManager.connect("game_started", _on_game_ready)

func _on_game_ready(_port):
	get_tree().change_scene_to_file("res://scenes/game.tscn")
	
func _on_joinGame_button_pressed() -> void:
	get_tree().change_scene_to_file("res://scenes/main_menu/join_menu/join_menu.tscn")

func _on_exit_button_pressed() -> void:
	get_tree().quit()
