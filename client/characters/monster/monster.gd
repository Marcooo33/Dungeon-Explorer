extends Node2D

@onready var animated_sprite = $AnimatedSprite2D

# Variabili gestite dal GameManager
var hp: int = 0
var is_alive: bool = true
var armor_prefix: String = "orc_"

func _ready():
	# Colleghiamo il segnale per sapere quando finisce un'animazione (es. morte o attacco)
	if not animated_sprite.animation_finished.is_connected(_on_animation_finished):
		animated_sprite.animation_finished.connect(_on_animation_finished)

# Questa funzione viene chiamata dal GameManager appena il mostro viene creato
func setup(monster_name: String, armor_type: String):
	var name_lower = monster_name.to_lower()
	var is_skeleton = "skeleton" in name_lower or "scheletro" in name_lower
	# NUOVO: Aggiungiamo il controllo per il boss Knight
	var is_knight = "knight" in name_lower or "boss" in name_lower
	
	# Scegliamo il prefisso in base alla razza e all'armatura
	if is_knight:
		# Il boss ha una sua skin fissa, ignoriamo l'armatura visiva
		armor_prefix = "knight_"
		
	elif is_skeleton:
		match armor_type:
			"leather":
				armor_prefix = "skeleton_rogue_"
			"chainmail":
				armor_prefix = "skeleton_warrior_"
			"none", _:
				armor_prefix = "skeleton_"
				
	else: # Default: Orco
		match armor_type:
			"leather":
				armor_prefix = "orc_rogue_" 
			"chainmail":
				armor_prefix = "orc_warrior_"
			"none", _:
				armor_prefix = "orc_"
			
	# Avviamo l'animazione di riposo
	play_idle()

# ==========================================
# GESTIONE ANIMAZIONI
# ==========================================

func play_idle():
	# Se il mostro è morto, non deve respirare!
	if not is_alive:
		return
		
	var anim_name = armor_prefix + "idle"
	
	# Controllo di sicurezza: verifichiamo che l'animazione esista prima di avviarla
	if animated_sprite.sprite_frames.has_animation(anim_name):
		animated_sprite.play(anim_name)
		
		# Desincronizzazione: fa partire l'animazione da un frame casuale
		var total_frames = animated_sprite.sprite_frames.get_frame_count(anim_name)
		if total_frames > 0:
			animated_sprite.frame = randi() % total_frames
		animated_sprite.speed_scale = randf_range(0.9, 1.1)
	else:
		print("ATTENZIONE: Animazione mancante in SpriteFrames -> ", anim_name)

# Usata per animazioni generiche (es. attacco)
func play_action(action: String):
	if not is_alive:
		return
		
	animated_sprite.speed_scale = 1.0 
	var anim_name = armor_prefix + action
	
	if animated_sprite.sprite_frames.has_animation(anim_name):
		animated_sprite.play(anim_name)

# Chiamata dal GameManager quando il server comunica che il mostro ha 0 hp
func die():
	is_alive = false
	animated_sprite.speed_scale = 1.0
	var anim_name = armor_prefix + "death"
	
	if animated_sprite.sprite_frames.has_animation(anim_name):
		animated_sprite.play(anim_name)

# Segnale automatico alla fine di un'animazione
func _on_animation_finished():
	var current_anim = animated_sprite.animation
	
	# Se ha finito l'animazione di morte
	if current_anim.ends_with("death"):
		pass # Lascia il cadavere a terra
		
	# Se ha finito qualsiasi altra animazione (es. attacco) e non è morto, torna in idle
	elif not current_anim.ends_with("idle") and is_alive:
		play_idle()
