# One visible entity.  Moves towards its target at `speed` units per second, the same rule the
# zone applies, so what the player sees stays within one tick of the server state.
extends Node2D

const RADIUS := 14.0

var entity_id := 0
var entity_type := 0     # jx.pb.EntityType
var display_name := ""
var target := Vector2.ZERO
var speed := 0.0
var is_own := false
var _label: Label


func setup(d: Dictionary, own: bool) -> void:
	entity_id = int(d.id)
	entity_type = int(d.type)
	display_name = str(d.name)
	is_own = own
	position = Vector2(d.x, d.y)
	target = Vector2(d.tx, d.ty)
	speed = float(d.speed)
	if _label == null:
		_label = Label.new()
		_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		_label.position = Vector2(-60, -RADIUS - 24)
		_label.size = Vector2(120, 20)
		add_child(_label)
	_label.text = display_name
	queue_redraw()


func apply_move(mv: Dictionary) -> void:
	position = Vector2(mv.x, mv.y)
	target = Vector2(mv.tx, mv.ty)
	speed = float(mv.speed)


func _process(delta: float) -> void:
	if position == target:
		return
	var step := speed * delta
	var d := target - position
	if d.length() <= step:
		position = target
	else:
		position += d.normalized() * step


func _draw() -> void:
	var color := Color(0.95, 0.6, 0.2)          # npc
	if entity_type == 1:
		color = Color(0.3, 0.9, 0.4)             # other player
	if is_own:
		color = Color(0.35, 0.65, 1.0)           # me
	elif entity_type == 3:
		color = Color(0.95, 0.3, 0.3)            # monster
	draw_circle(Vector2.ZERO, RADIUS, color)
	draw_arc(Vector2.ZERO, RADIUS, 0, TAU, 24, Color(0, 0, 0, 0.6), 2.0)
