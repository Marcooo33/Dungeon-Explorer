extends RefCounted
class_name NetworkSocket

var socket := StreamPeerTCP.new()
var buffer := ""
var ip = OS.get_environment("SERVER_IP") if OS.has_environment("SERVER_IP") else "127.0.0.1"
var port = "8080"

signal raw_packet(cmd, args)
signal connected
signal disconnected

func connect_to_server():
	socket.connect_to_host(ip, port.to_int())

func poll():
	socket.poll()

	match socket.get_status():
		StreamPeerTCP.STATUS_CONNECTED:
			var available = socket.get_available_bytes()
			if available > 0:
				_process(socket.get_utf8_string(available))
			connected.emit()

		StreamPeerTCP.STATUS_ERROR:
			disconnected.emit()

func _process(data: String):
	buffer += data

	while "\n" in buffer:
		var pos = buffer.find("\n")
		var msg = buffer.substr(0, pos).strip_edges()
		buffer = buffer.substr(pos + 1)
		
		print(msg)

		var parts = msg.split(" ")
		if parts.size() == 0:
			continue

		var cmd = parts[0]
		var args = parts.slice(1)

		raw_packet.emit(cmd, args)

func send(msg: String):
	if socket.get_status() == StreamPeerTCP.STATUS_CONNECTED:
		socket.put_data((msg + "\n").to_utf8_buffer())
