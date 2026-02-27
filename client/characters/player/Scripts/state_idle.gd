class_name State_Idle extends State

@onready var walk : State = $"../Walk"

# defines what happens when the player enters the state
func Enter() -> void:
	player.UpdateAnimation("idle")
	pass

# defines what happens when the player exits the state
func Exit() -> void:
	pass

# defines what happens during the _process update in this state
func Process( _delta : float ) -> State:
	if player.direction != Vector2.ZERO:
		return walk
	player.velocity = Vector2.ZERO
	return null

# defines what appens during the _physiscs_process update in this state
func Physics( _delta : float ) -> State:
	return null

# defines what happens with input events in this state
func HandleInput( _event : InputEvent ) -> State:
	return null
