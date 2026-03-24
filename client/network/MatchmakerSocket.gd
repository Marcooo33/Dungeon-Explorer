extends RefCounted # Classe leggera, non serve che sia un nodo
class_name MatchmakerSocket

var socket = StreamPeerTCP.new()
var buffer = ""
var server_ip = "127.0.0.1"
var server_port = 8080

signal game_found(port, code)
signal error_received(reason)

func connect_to_server():
	socket.connect_to_host(server_ip, server_port)

func poll():
	socket.poll()
	var status = socket.get_status()
	if status == StreamPeerTCP.STATUS_CONNECTED:
		var raw_data = socket.get_available_bytes()
		if raw_data > 0:
			_process_data(socket.get_utf8_string(raw_data))


func _process_data(raw_data: String):
	buffer += raw_data
	while "\n" in buffer:
		var pos = buffer.find("\n")
		var processed_data = buffer.substr(0, pos).strip_edges()
		buffer = buffer.substr(pos + 1)
		_handle_command(processed_data)

func _handle_command(msg: String):
	var parts = msg.split(" ")
	if parts[0] == "START" and parts.size() == 3:
		game_found.emit(parts[1].to_int(), parts[2])
	elif parts[0] == "ERROR":
		error_received.emit(msg)
	else:
		print("Errore generico: ", msg)

func send_command(cmd: String):
	if socket.get_status() == StreamPeerTCP.STATUS_CONNECTED:
		socket.put_data((cmd + "\n").to_utf8_buffer())
	else:
		print("Matchmaker Socket non connessa: ", socket.get_status())
