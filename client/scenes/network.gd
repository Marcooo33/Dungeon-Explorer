extends Node

var socket = StreamPeerTCP.new()

func _ready():
	print("DEBUG: Lo script di rete è partito!")
	var err = socket.connect_to_host("127.0.0.1", 8080)
	if err == OK:
		print("Tentativo di connessione in corso...")
	else:
		print("Errore immediato di connessione: ", err)

func _process(_delta):
	socket.poll() # Necessario per aggiornare lo stato del socket
	var status = socket.get_status()
	
	if status == StreamPeerTCP.STATUS_CONNECTED:
		if socket.get_available_bytes() > 0:
			var data = socket.get_utf8_string(socket.get_available_bytes())
			print("Ricevuto dal matchmaking: ", data)
