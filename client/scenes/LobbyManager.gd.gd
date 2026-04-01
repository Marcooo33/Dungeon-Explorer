extends Node2D # o Control, a seconda di cos'è la root della tua scena Game/Lobby

@onready var join_dialog = $JoinRequestDialog 
@onready var start_button = $StartButton

# Questa è la nostra coda: un array che terrà a memoria chi ha bussato
var join_queue : Array[int] = [] 

func _ready():
	# 1. Ci mettiamo in ascolto del NetworkManager per chi bussa
	NetworkManager.join_request_received.connect(_on_join_request_received)
	
	# 2. Colleghiamo i tasti del popup
	join_dialog.confirmed.connect(_on_accept_pressed) # Tasto OK/Accetta
	join_dialog.canceled.connect(_on_reject_pressed)  # Tasto Cancel/Rifiuta/X
	
	# Assicuriamoci che il popup sia invisibile all'avvio
	join_dialog.hide()
	
	# logica per bottone di inizio partita
	start_button.pressed.connect(_on_start_pressed)
	
# Quando il server C ci dice "Il giocatore X vuole entrare"
func _on_join_request_received(player_id: int):
	print("HOST UI: Ricevuta richiesta per il player ", player_id)
	
	# Lo mettiamo in fila nella coda (se non c'è già)
	if not join_queue.has(player_id):
		join_queue.append(player_id)
	
	# Se il popup NON è già aperto a schermo, mostriamo il primo della fila
	if not join_dialog.visible:
		_show_next_request()

# Funzione interna per mostrare il prossimo in fila
func _show_next_request():
	if join_queue.size() > 0:
		var current_player_id = join_queue[0] # Leggiamo chi è il primo in fila
		join_dialog.dialog_text = "Player %d wants to join the game.\nAccept?" % current_player_id
		join_dialog.popup_centered()

# Se l'Host clicca "ACCETTA"
func _on_accept_pressed():
	if join_queue.size() > 0:
		var current_player_id = join_queue.pop_front() # Togliamo il primo dalla fila
		print("HOST UI: Accettato giocatore ", current_player_id)
		NetworkManager.send_to_matchmaker("ACCEPT " + str(current_player_id))
		call_deferred("_show_next_request")

# Se l'Host clicca "RIFIUTA"
func _on_reject_pressed():
	if join_queue.size() > 0:
		var current_player_id = join_queue.pop_front() # Togliamo il primo dalla fila
		print("HOST UI: Rifiutato giocatore ", current_player_id)
		NetworkManager.send_to_matchmaker("REJECT " + str(current_player_id))
		call_deferred("_show_next_request")

func _on_start_pressed():
	print("HOST UI: Avvio partita richiesto")
	NetworkManager.send_to_matchmaker("START_GAME")
