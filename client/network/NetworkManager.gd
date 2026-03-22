extends Node

var SERVER_IP = OS.get_environment("SERVER_IP") if OS.has_environment("SERVER_IP") else "127.0.0.1"
var SERVER_PORT = OS.get_environment("SERVER_PORT") if OS.has_environment("PORT") else "8080"

var matchmaker_socket = StreamPeerTCP.new()
var game_socket = StreamPeerTCP.new()

# Segnali per avvisare il resto del gioco quando succede qualcosa
signal matchmaker_connected
signal game_started(port)

func _ready():
	# Connessione automatica all'avvio 
	var err = matchmaker_socket.connect_to_host(SERVER_IP, int(SERVER_PORT))
	if err == OK:
		print("NetworkManager: Tentativo di connessione al Matchmaker...")
	else:
		print("NetworkManager: Errore inizializzazione socket: ", err)

func _process(_delta):
	_monitor_socket(matchmaker_socket, "Matchmaker")
	_monitor_socket(game_socket, "GameServer")

func _monitor_socket(socket: StreamPeerTCP, label: String):
	socket.poll()
	var status = socket.get_status()
	
	if status == StreamPeerTCP.STATUS_CONNECTED:
		var available_bytes = socket.get_available_bytes()
		if available_bytes > 0:
			var data = socket.get_utf8_string(available_bytes)
			_handle_data(label, data) # Elaboriamo i dati invece di rispedirli indietro
	elif status == StreamPeerTCP.STATUS_ERROR:
		print("ERRORE: Problema con ", label)

# Gestione dei messaggi in arrivo
func _handle_data(source: String, data: String):
	print("Ricevuto da ", source, ": ", data)
	
	if source == "Matchmaker":
		var game_port = data.get_slice(" ", 2).to_int()
		connect_to_game_server(game_port)

func send_to_matchmaker(msg: String):
	if matchmaker_socket.get_status() == StreamPeerTCP.STATUS_CONNECTED:
		matchmaker_socket.put_data((msg + "\n").to_utf8_buffer()) # Aggiungi \n per il server
		print("Inviato: ", msg)

func connect_to_game_server(port: int):
	print("Connessione al Game Server sulla porta: ", port)
	var err = game_socket.connect_to_host(SERVER_IP, port)
	if err == OK:
		emit_signal("game_started", port)
		
