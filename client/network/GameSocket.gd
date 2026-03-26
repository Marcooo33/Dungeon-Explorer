# GameClient.gd
extends RefCounted
class_name GameSocket

var socket = StreamPeerTCP.new()
var buffer = ""
var server_ip = OS.get_environment("SERVER_IP") if OS.has_environment("SERVER_IP") else "127.0.0.1"

signal data_received(cmd, args)
signal connection_established
signal connection_lost

func connect_to_game(port: int):
	socket.connect_to_host(server_ip, port)

func poll():
	socket.poll()
	var status = socket.get_status()
	if status == StreamPeerTCP.STATUS_CONNECTED:
		var raw_data = socket.get_available_bytes()
		if raw_data > 0:
			_process_data(socket.get_utf8_string(raw_data))
	elif status == StreamPeerTCP.STATUS_ERROR:
		connection_lost.emit()

func _process_data(data: String):
	buffer += data
	while "\n" in buffer:
		var pos = buffer.find("\n")
		var processed_data = buffer.substr(0, pos).strip_edges()
		buffer = buffer.substr(pos + 1)
		var parts = processed_data.split(" ")
		data_received.emit(parts[0], parts.slice(1))

func send_action(action: String):
	if socket.get_status() == StreamPeerTCP.STATUS_CONNECTED:
		socket.put_data((action + "\n").to_utf8_buffer())
