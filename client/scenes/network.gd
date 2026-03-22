extends Node

var matchmaker_socket = StreamPeerTCP.new()
var game_socket = StreamPeerTCP.new()
var message_sent = false # Flag per evitare di inviare il messaggio a ripetizione

func _ready():
	print("DEBUG: Lo script di rete è partito!")
	var socket_connection_status = matchmaker_socket.connect_to_host("server-dev", 8080)
	if socket_connection_status == OK:
		print("Tentativo di connessione al matchmaker in corso...")
	else:
		print("Errore immediato di connessione al matchmaker: ", socket_connection_status)

func _process(_delta):
	matchmaker_socket.poll() # Necessario per aggiornare lo stato del socket
	var status = matchmaker_socket.get_status()
	
	if status == StreamPeerTCP.STATUS_CONNECTED:
		if not message_sent:
			# Creazione messaggio e invio messaggio
			var msg = "CREATE"
			var bytes=msg.to_utf8_buffer()
			matchmaker_socket.put_data(bytes)
			print("Inviato al matchmaker: ", msg)
			message_sent = true # Segnamo che l'invio è avvenuto
			
		if matchmaker_socket.get_available_bytes() > 0:
			# Prendiamo la risposta del server
			var data = matchmaker_socket.get_utf8_string(matchmaker_socket.get_available_bytes())
			print("Ricevuto dal matchmaker: ", data)
			var game_port = data.get_slice(" ", 2).to_int();
			
			# Creaiamo la socket per la partita
			var socket_connection_status = game_socket.connect_to_host("server-dev", game_port)
			if socket_connection_status == OK:
				print("Tentativo di connessione alla partita in corso...")
			else:
				print("Errore immediato di connessione alla partita: ", socket_connection_status)
			
	elif status == StreamPeerTCP.STATUS_ERROR or status == StreamPeerTCP.STATUS_NONE:
		if message_sent: # Se eravamo connessi e ora non più
			print("Connessione persa o errore col matchmaker.")
			message_sent = false
