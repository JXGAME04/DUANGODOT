# One visible entity (KNpc of the old client, client side).  Positions are kept in *scene units*
# (the zone's coordinate system) and projected to the screen as (x, y / 2), the same projection
# the map bundle uses.  Movement follows the waypoints the zone sent at the same speed, so the
# view stays within a tick of the server.  Animation runs on the old 18 Hz logic tick: the doing
# (stand / walk / run) picks an action, cur_frame counts to the template's frame count
# (KNpc::WaitForFrame) and KNpcRes maps that progress onto the sprite frames.
extends Node2D

const KNpcResScript := preload("res://scenes/KNpcRes.gd")
const KNpcResNode := preload("res://scenes/KNpcResNode.gd")
const KMath := preload("res://scenes/KMath.gd")

const RADIUS := 14.0
const TICK := 1.0 / 18.0        # old logic frame
const RUN_SPEED := 150.0        # scene units / s: faster plays the run action (zone default is 200)
const ENTITY_PLAYER := 1        # jx.pb.EntityType

var entity_id := 0
var entity_type := 0
var display_name := ""
var template_id := 0
var sex := 0
var scene_pos := Vector2.ZERO      # scene units
var path: Array = []               # remaining waypoints (Vector2, scene units)
var speed := 0.0                   # scene units per second
var is_own := false
var dir64 := 0                     # m_Dir: where the entity faces
var res_dir := 0                   # m_ResDir: the drawn facing, turning toward dir64
var doing := -1                    # KNpcResNode.Doing
var total_frame := 15              # m_Frames.nTotalFrame
var cur_frame := 0                 # m_Frames.nCurrentFrame
var has_res := false
var frames := {"stand": 15, "stand1": 15, "walk": 12, "run": 15}
var _res: Node2D
var _label: Label
var _tick_acc := 0.0
var _rng := RandomNumberGenerator.new()


static func to_screen(p: Vector2) -> Vector2:
	return Vector2(p.x, p.y * 0.5)


static func to_scene(p: Vector2) -> Vector2:
	return Vector2(p.x, p.y * 2.0)


func setup(d: Dictionary, own: bool) -> void:
	entity_id = int(d.id)
	entity_type = int(d.type)
	display_name = str(d.name)
	template_id = int(d.get("template_id", 0))
	sex = int(d.get("sex", 0))
	is_own = own
	scene_pos = Vector2(d.x, d.y)
	speed = float(d.speed)
	dir64 = clampi(int(d.get("dir", 0)), 0, 63)
	res_dir = dir64
	path = _waypoints(d)
	position = to_screen(scene_pos)
	_rng.seed = entity_id
	if _res == null:
		_res = Node2D.new()
		_res.set_script(KNpcResScript)
		_res.name = "Res"
		add_child(_res)
	if _label == null:
		_label = Label.new()
		_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		_label.size = Vector2(140, 20)
		_label.add_theme_color_override("font_color", Color.WHITE)
		_label.add_theme_color_override("font_shadow_color", Color.BLACK)
		_label.add_theme_constant_override("shadow_offset_x", 1)
		_label.add_theme_constant_override("shadow_offset_y", 1)
		add_child(_label)
	_label.text = display_name
	_label.position = Vector2(-70, -RADIUS - 26)
	# appearance: players are the composed main characters, everything else its npcs.txt template
	var res_name := ""
	if entity_type == ENTITY_PLAYER:
		res_name = KNpcResNode.player_res_name(sex)
		var pf := NpcResList.player_frames(sex)
		frames = {"stand": int(pf.get("stand_frame", 15)), "stand1": int(pf.get("stand_frame", 15)),
			"walk": int(pf.get("walk_frame", 12)), "run": int(pf.get("run_frame", 15))}
	else:
		var tpl := NpcResList.template(template_id)
		res_name = str(tpl.get("res", ""))
		frames = {"stand": int(tpl.get("stand_frame", 15)), "stand1": int(tpl.get("stand_frame1", 15)),
			"walk": int(tpl.get("walk_frame", 12)), "run": int(tpl.get("run_frame", 15))}
	has_res = _res.setup(res_name)
	if not has_res:
		Log.debug("npcres", "no appearance, drawing a marker", {"entity": entity_id, "type": entity_type,
			"template": template_id, "res": res_name})
	doing = -1
	_set_doing(KNpcResNode.Doing.STAND)
	_tick_acc = 0.0
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
	if not path.is_empty():
		var wp: Vector2 = path[0]
		var d := KMath.get_dir_index(int(scene_pos.x), int(scene_pos.y), int(wp.x), int(wp.y))
		if d >= 0:
			dir64 = d
	_tick_acc += delta
	while _tick_acc >= TICK:
		_tick_acc -= TICK
		_tick()


# One old logic frame: choose the doing, advance the frame counter, turn, and draw.
@warning_ignore("integer_division")
func _tick() -> void:
	var want: int = KNpcResNode.Doing.STAND
	if is_moving():
		want = KNpcResNode.Doing.RUN if speed >= RUN_SPEED else KNpcResNode.Doing.WALK
	elif doing == KNpcResNode.Doing.STAND1:
		want = KNpcResNode.Doing.STAND1   # the idle variant plays to its end
	if want != doing:
		_set_doing(want)
	else:
		cur_frame += 1
		if cur_frame >= total_frame:   # KNpc::WaitForFrame
			cur_frame = 0
			if not is_moving():        # KNpc::OnStand: one cycle in six is the second idle
				_set_doing(KNpcResNode.Doing.STAND if _rng.randi_range(0, 5) != 1 else KNpcResNode.Doing.STAND1)
	# KNpc::Paint: the drawn facing turns half the way toward the real one every frame
	if res_dir != dir64:
		var off := dir64 - res_dir
		if off > 32:
			off -= 64
		elif off < -32:
			off += 64
		res_dir = posmod(res_dir + (off / 2 if absi(off) > 1 else off), 64)
	if has_res:
		_res.paint(res_dir, total_frame, cur_frame)
		_label.position.y = _res.head_top - 24.0


func _set_doing(d: int) -> void:
	doing = d
	cur_frame = 0
	var n := 15
	match d:
		KNpcResNode.Doing.STAND:
			n = int(frames.stand)
		KNpcResNode.Doing.STAND1:
			n = int(frames.stand1)
		KNpcResNode.Doing.WALK:
			n = int(frames.walk)
		KNpcResNode.Doing.RUN:
			n = int(frames.run)
	total_frame = maxi(n, 1)
	if has_res:
		_res.set_action(d)
		queue_redraw()


func _draw() -> void:
	if has_res:
		return
	var color := Color(0.95, 0.6, 0.2)          # npc
	if entity_type == ENTITY_PLAYER:
		color = Color(0.3, 0.9, 0.4)             # other player
	if is_own:
		color = Color(0.35, 0.65, 1.0)           # me
	elif entity_type == 3:
		color = Color(0.95, 0.3, 0.3)            # monster
	draw_circle(Vector2.ZERO, RADIUS, color)
	draw_arc(Vector2.ZERO, RADIUS, 0, TAU, 24, Color(0, 0, 0, 0.6), 2.0)
