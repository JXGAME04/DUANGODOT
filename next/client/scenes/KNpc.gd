# One visible entity (KNpc of the old client, client side).  Positions are kept in *scene units*
# (the zone's coordinate system) and projected to the screen as (x, y / 2), the same projection
# the map bundle uses.  Movement follows the waypoints the zone sent at the same speed, so the
# view stays within a tick of the server.  Animation runs on the old 18 Hz logic tick: the doing
# (stand / walk / run / attack / hurt / death) picks an action, cur_frame counts to the action's
# frame count (KNpc::WaitForFrame) and KNpcRes maps that progress onto the sprite frames.
extends Node2D

const KNpcResScript := preload("res://scenes/KNpcRes.gd")
const KNpcResNode := preload("res://scenes/KNpcResNode.gd")
const KMath := preload("res://scenes/KMath.gd")

const RADIUS := 14.0
const TICK := 1.0 / 18.0        # old logic frame
const RUN_SPEED := 150.0        # scene units / s: faster plays the run action (zone default is 200)
const ENTITY_PLAYER := 1        # jx.pb.EntityType
const ENTITY_MONSTER := 3
# jx.pb.Action
const ACTION_STAND := 0
const ACTION_ATTACK := 1
const ACTION_HURT := 2
const ACTION_DEATH := 3
const ACTION_REVIVE := 4
const LIFE_BAR := Vector2(40, 4)

var entity_id := 0
var entity_type := 0
var display_name := ""
var template_id := 0
var sex := 0
var scene_pos := Vector2.ZERO      # scene units
var path: Array = []               # remaining waypoints (Vector2, scene units)
var speed := 0.0                   # scene units per second
var is_own := false
var is_target := false             # selected by the local player
var dir64 := 0                     # m_Dir: where the entity faces
var res_dir := 0                   # m_ResDir: the drawn facing, turning toward dir64
var doing := -1                    # KNpcResNode.Doing
var total_frame := 15              # m_Frames.nTotalFrame
var cur_frame := 0                 # m_Frames.nCurrentFrame
var life := 0
var life_max := 0
var has_res := false
var frames := {"stand": 15, "stand1": 15, "walk": 12, "run": 15}
var stature := 0                   # m_nStature (npcs.txt): name height above the feet
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
	life = int(d.get("life", 0))
	life_max = int(d.get("life_max", 0))
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
		stature = int(tpl.get("stature", 0))
	has_res = _res.setup(res_name)
	if not has_res:
		Log.debug("npcres", "no appearance, drawing a marker", {"entity": entity_id, "type": entity_type,
			"template": template_id, "res": res_name})
	# KNpc::GetNpcPate: the name sits m_nStature (+84 for players) above the feet
	_label.position.y = -float(_pate()) - 20.0
	doing = -1
	_set_doing(KNpcResNode.Doing.STAND)
	# a late joiner sees corpses and swings already under way
	var now := int(d.get("doing", ACTION_STAND))
	if now == ACTION_DEATH:
		_set_action(KNpcResNode.Doing.DEATH, int(d.get("doing_frames", 1)))
		cur_frame = total_frame - 1
	elif now == ACTION_ATTACK or now == ACTION_HURT:
		apply_action({"action": now, "frames": d.get("doing_frames", 1), "x": d.x, "y": d.y, "dir": dir64})
	_tick_acc = 0.0
	queue_redraw()


func apply_move(mv: Dictionary) -> void:
	scene_pos = Vector2(mv.x, mv.y)
	speed = float(mv.speed)
	path = _waypoints(mv)
	position = to_screen(scene_pos)
	if doing == KNpcResNode.Doing.ATTACK or doing == KNpcResNode.Doing.ATTACK1:
		_set_doing(KNpcResNode.Doing.STAND)   # KNpc::DoWalk interrupts the swing


# EntityAction from the zone: KNpc::DoAttack / DoHurt / DoDeath on the client side.
func apply_action(a: Dictionary) -> void:
	scene_pos = Vector2(a.x, a.y)
	path = []
	position = to_screen(scene_pos)
	dir64 = clampi(int(a.get("dir", dir64)), 0, 63)
	var n := maxi(int(a.get("frames", 1)), 1)
	match int(a.action):
		ACTION_ATTACK:
			# one of the two attack animations at random, like KNpc::DoAttack
			_set_action(KNpcResNode.Doing.ATTACK if _rng.randi_range(0, 1) == 1 else KNpcResNode.Doing.ATTACK1, n)
		ACTION_HURT:
			_set_action(KNpcResNode.Doing.HURT, n)
		ACTION_DEATH:
			_set_action(KNpcResNode.Doing.DEATH, n)
			is_target = false
		_:
			_set_doing(KNpcResNode.Doing.STAND)


func set_life(l: Dictionary) -> void:
	life = int(l.get("life", life))
	life_max = int(l.get("life_max", life_max))
	queue_redraw()


func set_target(on: bool) -> void:
	is_target = on
	queue_redraw()


func is_dead() -> bool:
	return doing == KNpcResNode.Doing.DEATH


# What a click can attack: monsters that are still alive (the zone refuses the rest anyway).
func is_attackable() -> bool:
	return entity_type == ENTITY_MONSTER and not is_dead()


# True when the point (local coordinates, screen px) lies on the drawn body.
func hit_test(local: Vector2) -> bool:
	if has_res:
		for p in _res.parts:
			var sp: Sprite2D = p.sprite
			if sp.texture == null:
				continue
			if Rect2(sp.position, sp.texture.get_size()).has_point(local):
				return true
		return false
	return local.length() <= RADIUS


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
	if doing == KNpcResNode.Doing.DEATH:
		# KNpc::OnDeath: the corpse keeps its last frame until the zone removes it
		if cur_frame < total_frame - 1:
			cur_frame += 1
	elif doing == KNpcResNode.Doing.ATTACK or doing == KNpcResNode.Doing.ATTACK1 or doing == KNpcResNode.Doing.HURT:
		cur_frame += 1
		if cur_frame >= total_frame:   # KNpc::OnSpecial1 / OnHurt -> DoStand
			_set_doing(KNpcResNode.Doing.STAND)
	else:
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


# KNpc::GetNpcPate (no jump height, sitting or riding yet).
func _pate() -> int:
	var h := stature
	if entity_type == ENTITY_PLAYER:
		h += 84
	return h


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


# An action whose length the zone dictates (attack / hurt / death frames).
func _set_action(d: int, n: int) -> void:
	doing = d
	cur_frame = 0
	total_frame = maxi(n, 1)
	if has_res:
		_res.set_action(d)
	queue_redraw()


func _draw() -> void:
	if not has_res:
		var color := Color(0.95, 0.6, 0.2)          # npc
		if entity_type == ENTITY_PLAYER:
			color = Color(0.3, 0.9, 0.4)             # other player
		if is_own:
			color = Color(0.35, 0.65, 1.0)           # me
		elif entity_type == ENTITY_MONSTER:
			color = Color(0.95, 0.3, 0.3)            # monster
		draw_circle(Vector2.ZERO, RADIUS, color)
		draw_arc(Vector2.ZERO, RADIUS, 0, TAU, 24, Color(0, 0, 0, 0.6), 2.0)
	# KNpc::PaintLife: a life bar under the name for wounded or selected characters
	if life_max > 0 and (life < life_max or is_target) and not is_dead():
		var top := Vector2(-LIFE_BAR.x * 0.5, -float(_pate()) + 2.0)
		draw_rect(Rect2(top, LIFE_BAR), Color(0, 0, 0, 0.7))
		var w := LIFE_BAR.x * clampf(float(life) / float(life_max), 0.0, 1.0)
		draw_rect(Rect2(top, Vector2(w, LIFE_BAR.y)), Color(0.85, 0.15, 0.15) if not is_own else Color(0.2, 0.8, 0.3))
	if is_target and not is_dead():
		draw_arc(Vector2(0, 0), 18.0, 0, TAU, 24, Color(1.0, 0.9, 0.2, 0.8), 2.0)
