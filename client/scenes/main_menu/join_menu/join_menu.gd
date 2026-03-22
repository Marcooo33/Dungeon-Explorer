extends Control


func _on_join_button_pressed() -> void:
	var input_code = $VBoxContainer/CodeLineEdit.text
	if NetworkManager.matchmaker_socket.get_status() == StreamPeerTCP.STATUS_CONNECTED:
		NetworkManager.send_to_matchmaker("JOIN " + input_code)
		# Suggerimento: non cambiare scena subito. 
		# Aspetta che il server ti confermi la creazione!
		# Puoi farlo collegandoti al segnale 'game_started'
	else:
		print("Impossibile creare partita: Matchmaker non connesso.")
	
