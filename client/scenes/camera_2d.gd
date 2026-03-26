extends Camera2D

# Supponiamo che la tua mappa sia larga 320 pixel nel mondo di gioco
var map_width_pixels = 550.0 

func _ready():
	# Opzionale: connettiti all'evento di ridimensionamento della finestra 
	# per aggiornare lo zoom se il giocatore ridimensiona la finestra
	get_tree().root.size_changed.connect(_on_window_resized)
	_adjust_camera_zoom()

func _on_window_resized():
	_adjust_camera_zoom()

func _adjust_camera_zoom():
	var viewport_width = get_viewport_rect().size.x
	
	# Calcola lo zoom necessario: quanto è grande lo schermo diviso quanto è grande la mappa
	var new_zoom_level = viewport_width / map_width_pixels
	
	# Applica lo stesso zoom sia su X che su Y per non distorcere i pixel
	zoom = Vector2(new_zoom_level, new_zoom_level)
