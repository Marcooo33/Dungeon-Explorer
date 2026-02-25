extends CharacterBody2D

const SPEED = 130.0

func _physics_process(_delta: float) -> void:
	# 1. Otteniamo l'input per entrambi gli assi (X e Y)
	# get_vector gestisce automaticamente la normalizzazione (evita di andare più veloci in diagonale)
	var direction := Input.get_vector("move_left", "move_right", "move_up", "move_down")
	
	if direction != Vector2.ZERO:
		# Applichiamo la direzione alla velocità
		velocity = direction * SPEED
	else:
		# Decelerazione fluida verso lo zero
		velocity = velocity.move_toward(Vector2.ZERO, SPEED)

	# Muove il personaggio gestendo le collisioni
	move_and_slide()
