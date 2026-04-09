extends Node2D

@onready var join_dialog = $JoinRequestDialog
@onready var start_button = $StartButton

var join_queue: Array[int] = []

func _ready():
	NetworkManager.join_request_received.connect(_on_join_request)

	join_dialog.confirmed.connect(_accept)
	join_dialog.canceled.connect(_reject)
	start_button.pressed.connect(_start)

# ---------------- JOIN ----------------

func _on_join_request(id: int):
	if not join_queue.has(id):
		join_queue.append(id)

	if not join_dialog.visible:
		_show()

func _show():
	if join_queue.size() > 0:
		var id = join_queue[0]
		join_dialog.dialog_text = "Player %d wants to join" % id
		join_dialog.popup_centered()

func _accept():
	var id = join_queue.pop_front()
	NetworkManager.send_to_server("ACCEPT %d" % id)
	call_deferred("_show")

func _reject():
	var id = join_queue.pop_front()
	NetworkManager.send_to_server("REJECT %d" % id)
	call_deferred("_show")

func _start():
	NetworkManager.send_to_server("START_GAME")
