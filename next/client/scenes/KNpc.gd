# One visible entity.  Positions are kept in *scene units* (the zone's coordinate system) and
# projected to the screen as (x, y / 2), the same projection the map bundle uses.  Movement
# follows the waypoints the zone sent at the same speed, so the view stays within a tick of the
# server.
extends Node2D

const RADIUS := 14.0

var entity_id := 0
var entity_type := 0     # jx.pb.EntityType
var display_name := ""
var scene_pos := Vector2.ZERO      # scene units
var path: Array = []               # remaining waypoints (Vector2, scene units)
var speed := 0.0                   # scene units per second
var facing := 0
var is_own := false
var _label: Label


static func to_screen(p: Vector2) -> Vector2:
	return Vector2(p.x, p.y * 0.5)


static func to_scene(p: Vector2) -> Vector2:
	return Vector2(p.x, p.y * 2.0)


func setup(d: Dictionary, own: bool) -> void:
	entity_id = int(d.id)
	entity_type = int(d.type)
	display_name = str(d.name)
	is_own = own
	scene_pos = Vector2(d.x, d.y)
	speed = float(d.speed)
	facing = int(d.get("dir", 0))
	path = _waypoints(d)
	position = to_screen(scene_pos)
	if _label == null:
		_label = Label.new()
		_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		_label.position = Vector2(-70, -RADIUS - 26)
		_label.size = Vector2(140, 20)
		_label.add_theme_color_override("font_color", Color.WHITE)
		_label.add_theme_color_override("font_shadow_color", Color.BLACK)
		_label.add_theme_constant_override("shadow_offset_x", 1)
		_label.add_theme_constant_override("shadow_offset_y", 1)
		add_child(_label)
	_label.text = display_name
	queue_redraw()


func apply_move(mv: Dictionary) -> void:
	scene_pos = Vector2(mv.x, mv.y)
	speed = float(mv.speed)
	path = _waypoints(mv)
	position = to_screen(scene_pos)


func _waypoints(d: Dictionary) -> Array:
	var out: Array = []
	for p in d.get("path", []):
		out.append(Vector2(p[0], p[1]))
	if out.is_empty() and d.has("tx") and (int(d.tx) != int(d.x) or int(d.ty) != int(d.y)):
		out.append(Vector2(d.tx, d.ty))
	return out


func is_moving() -> bool:
	return not path.is_empty()


func target() -> Vector2:
	return path.back() if not path.is_empty() else scene_pos


func _process(delta: float) -> void:
	var budget := speed * delta
	while budget > 0.0 and not path.is_empty():
		var wp: Vector2 = path[0]
		var d := wp - scene_pos
		var dist := d.length()
		if dist <= budget:
			scene_pos = wp
			budget -= dist
			path.pop_front()
		else:
			scene_pos += d / dist * budget
			budget = 0.0
	position = to_screen(scene_pos)


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
