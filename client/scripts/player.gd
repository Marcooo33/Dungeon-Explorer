class_name Player extends CharacterBody2D

var cardinal_direction : Vector2 = Vector2.DOWN
var direction : Vector2 = Vector2.ZERO
const SPEED : float = 100.0

var state : String = "idle"

@onready var animation_player : AnimationPlayer = $AnimationPlayer
@onready var sprite_2d : Sprite2D = $Sprite2D

func _ready():
	pass

func _process(_delta: float) -> void:
	direction.x = Input.get_action_strength("move_right") - Input.get_action_strength("move_left")
	direction.y = Input.get_action_strength("move_down") - Input.get_action_strength("move_up")
	
	velocity = direction * SPEED
	
	if SetState() == true || SetDirection() == true:
		UpdateAnimation()
	
	pass

func _physics_process( delta ):
	move_and_slide()
	

func SetDirection() -> bool:
	var new_direction : Vector2 = cardinal_direction
	if direction == Vector2.ZERO:
		return false
	
	if direction.y == 0:
		new_direction = Vector2.LEFT if direction.x < 0 else Vector2.RIGHT
	elif direction.y == 0:
		new_direction = Vector2.UP if direction.y < 0 else Vector2.DOWN
	
	if new_direction == cardinal_direction:
		return false

	cardinal_direction = new_direction
	sprite_2d.scale.x = -1 if cardinal_direction == Vector2.LEFT else 1
	return true


func SetState() -> bool:
	var new_state:String = "idle" if direction == Vector2.ZERO else "walk"
	if new_state == state:
		return false
	return true
	
func UpdateAnimation() -> void:
	animation_player.play( state + "_" + AnimDirection())
	pass

func AnimDirection() -> String:
	if cardinal_direction == Vector2.DOWN:
		return "down"
	elif cardinal_direction == Vector2.UP:
		return "up"
	else:
		return "side"
