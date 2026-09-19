# The 3D effects of the skills (docs/LO-TRINH-3D.md 3.1): client/assets3d/sfx/skill_map.json (tools/scn3d/map_skills.py)
# maps a JX1 skill id to the reference client's effects - the same skills by their Han names (怒雷指 = Nộ Lôi Chỉ) -
# as skill_section / skill_event / skill_childobj / sfx_object describe them [TK]:
#   - cast: sfx_object effects at the caster (at a fraction of the cast, the event's frame / the section's 30 frames);
#   - child: child objects - a flying one (sync 4) is what the zone's missile shows (KMissle3DView), a standing one is put
#     at the target point at its frame; a child's own events nest more children ("children", when = time / arrive /
#     hit / end) - a carrier without a picture ("空子物体") only holds them;
#   - hit: the child object's hit effect, else the element's generic hit (sfx_object 20..: 土/火/水/木/金系击中).
# Timing follows the zone: the cast lasts `frames` logic frames (18 Hz), fractions are taken of that.  The effects
# themselves are Scn3DSfx (CPUParticles3D / meshes from the exported prefabs).
extends RefCounted

const SfxScript := preload("res://scenes3d/Scn3DSfx.gd")
const KScene3DMath := preload("res://scenes3d/KScene3DMath.gd")
const OrbitScript := preload("res://scenes3d/KSkillOrbit3D.gd")
const HangScript := preload("res://scenes3d/KSkillHang3D.gd")
const TICK := 1.0 / 18.0
const HANG_HEIGHT := {"sys_bd": 0.9, "sys_bar": 2.0, "sys_foot": 0.0, "sys_state": 2.2, "sys_state@buf_head": 2.2, "": 0.0}   # metres over the feet [tự chọn: sys_bd = chest, sys_state over the head]

var dir := ""
var map := {}
var loaded := false
var spawned := 0        # the --auto proof


func load_map() -> bool:
	if loaded:
		return not map.is_empty()
	loaded = true
	dir = "%s/sfx" % Assets.assets3d_root()
	var d = Assets.load_json(dir.path_join("skill_map.json")) if FileAccess.file_exists(dir.path_join("skill_map.json")) else null
	if d is Dictionary:
		map = d
	return not map.is_empty()


static func _file(res: String) -> String:
	return res.replace("/", "_")


func entry(skill_id: int) -> Dictionary:
	return map.get("by_jx", {}).get(str(skill_id), {})


func element_of(skill_id: int) -> int:
	return int(map.get("series_of", {}).get(str(skill_id), -1))


# The flying child object of a skill (what its missile looks like), {} when the skill has none - nested ones too
func flying(skill_id: int) -> Dictionary:
	return _find_flying(entry(skill_id).get("child", []))


func _find_flying(list: Array) -> Dictionary:
	for c in list:
		if (bool(c.get("fly", false)) or c.has("orbit")) and str(c.get("res", "")) != "":
			return c
	for c in list:
		var f := _find_flying(c.get("children", []))
		if not f.is_empty():
			return f
	return {}


# The hit effect of a skill: the flying child's own, else any child's, else the element's generic one
func hit_res(skill_id: int) -> String:
	var f := flying(skill_id)
	if f.has("hit"):
		return str(f["hit"].get("res", ""))
	var h := _find_hit(entry(skill_id).get("child", []))
	if h != "":
		return h
	var el := element_of(skill_id)
	return str(map.get("element_hit", {}).get(str(el), ""))


func _find_hit(list: Array) -> String:
	return str(_find_hit_entry(list).get("res", ""))


func _find_hit_entry(list: Array) -> Dictionary:
	for c in list:
		if c.has("hit") and str(c["hit"].get("res", "")) != "":
			return c["hit"]
	for c in list:
		var h := _find_hit_entry(c.get("children", []))
		if not h.is_empty():
			return h
	return {}


# A cast (EntityAction ACTION_ATTACK with a skill): the cast effects at the caster and the standing child objects at
# the aim, each at its fraction of the `frames` logic frames.  `at_caster` / `at_aim` are world positions of the feet.
func cast(parent: Node, skill_id: int, frames: int, at_caster: Vector3, yaw: float, at_aim: Vector3, view: Node = null) -> void:
	if not load_map():
		return
	var e := entry(skill_id)
	var duration := float(maxi(frames, 1)) * TICK
	var any := false
	# Ghost (skill_event 111 [TK]): afterimages of the caster every `interval` s at `alpha` until the section ends
	if e.has("ghost") and view != null and view.has_method("start_ghost"):
		var g: Dictionary = e["ghost"]
		view.start_ghost(float(g.get("interval", 0.06)), float(g.get("alpha", 0.38)), duration * (1.0 - float(g.get("at", 0.0))))
		any = true
	for c in e.get("cast", []):
		if bool(c.get("on_hit", false)):
			continue
		any = true
		var hang := str(c.get("hang", ""))
		var pos: Vector3 = hang_pos(view, hang, at_caster)
		# sfx_object col 9 [TK]: 1 / 2 = linked to the caster (it moves along), 0 = left where it was made; 2 on a bone hinge
		# (sys_bd) = a follower at the hinge, position only
		var holder: Node = view if (view is Node3D and int(c.get("sync", 0)) in [1, 2]) else parent
		if view is Node3D and int(c.get("sync", 0)) == 2:
			var hn := hinge_of(view, hang)
			if hn != null:
				holder = follower(view, hn)
		_later(holder, float(c.get("at", 0.0)) * duration, str(c["res"]), pos, yaw, float(c.get("life_s", 1.0)), c)
	if _cast_children(parent, e.get("child", []), 0.0, duration, at_caster, yaw, at_aim, view):
		any = true
	if not any:
		# no mapped effect: the element's cast flash at the chest (sfx_object 1..4)
		var el := element_of(skill_id)
		var res := str(map.get("element_cast", {}).get(str(el), ""))
		if res != "":
			# sfx_object 1..4 [TK]: sys_bd, angle 2 (world zero), sync 2 (linked to the caster, no rotation)
			var hn := hinge_of(view, "sys_bd") if view is Node3D else null
			var holder2: Node = follower(view, hn) if hn != null else (view if view is Node3D else parent)
			_later(holder2, 0.0, res, hang_pos(view, "sys_bd", at_caster), yaw, 1.0, {"angle": 2, "sync": 2})


# The standing child objects of a cast (and the children their own events spawn in time, "when" = time) at their
# fraction of the parent's life; flying ones are the zone's missiles (KMissle3DView + the missile-effect packet).
# Returns true when anything was put down.
func _cast_children(parent: Node, list: Array, start: float, span: float, at_caster: Vector3, yaw: float, at_aim: Vector3, view: Node = null) -> bool:
	var any := false
	for c in list:
		if bool(c.get("fly", false)) or c.has("orbit"):
			# flying and orbiting (moveType 7) children ride the zone's missile (attach_flying)
			continue
		var t0: float = start + float(c.get("at", 0.0)) * span
		var life := float(c.get("life_s", 1.5))
		var pos: Vector3 = at_caster if int(c.get("pos_type", 0)) == 0 else at_aim
		pos.y += float(c.get("height", 0.0))
		if str(c.get("hang", "")) != "":
			# skill_childobj col 14 [TK]: the hang point of the creator (its model's own point when the view is known)
			pos = hang_pos(view if int(c.get("pos_type", 0)) == 0 else null, str(c["hang"]), pos)
		if str(c.get("res", "")) != "":
			any = true
			_later(parent, t0, str(c["res"]), pos, yaw, life, c)
		if c.has("hit") and str(c["hit"].get("res", "")) != "":
			# a standing child object that hits (an area blast, a talisman on the target): its hit picture at the aim when it lands
			any = true
			var hp: Vector3 = at_aim + Vector3(0, float(HANG_HEIGHT.get(str(c["hit"].get("hang", "sys_bd")), 0.9)), 0)
			_later(parent, t0 + minf(life, 0.3), str(c["hit"]["res"]), hp, yaw, float(c["hit"].get("life_s", 1.0)), c["hit"])
		for f in c.get("fx", []):
			if str(f.get("when", "time")) == "time" and str(f.get("res", "")) != "":
				any = true
				_later(parent, t0 + float(f.get("at", 0.0)) * life, str(f["res"]), pos, yaw, float(f.get("life_s", 1.0)), f)
		var kids: Array = c.get("children", [])
		if not kids.is_empty():
			var timed: Array = []
			for k in kids:
				var when := str(k.get("when", "time"))
				if when == "time" or when == "end":
					var k2: Dictionary = k.duplicate()
					if when == "end":
						k2["at"] = 1.0
					timed.append(k2)
			if _cast_children(parent, timed, t0, life, pos, yaw, at_aim, view):
				any = true
	return any


# The children a flying child object spawns where it lands / hits ("when" = arrive / hit) and its own arrival sfx
func _arrive_children(parent: Node, c: Dictionary, pos: Vector3, yaw: float) -> void:
	for f in c.get("fx", []):
		var when := str(f.get("when", "time"))
		if (when == "arrive" or when == "hit") and str(f.get("res", "")) != "":
			_spawn(parent, str(f["res"]), pos, yaw, float(f.get("life_s", 1.0)), f)
	var landed: Array = []
	for k in c.get("children", []):
		var when := str(k.get("when", "time"))
		if when == "arrive" or when == "hit" or when == "end":
			var k2: Dictionary = k.duplicate()
			k2["at"] = 0.0
			k2["pos_type"] = 0
			landed.append(k2)
	if not landed.is_empty():
		_cast_children(parent, landed, 0.0, float(c.get("life_s", 1.0)), pos, yaw, pos)


func _later(parent: Node, delay: float, res: String, pos: Vector3, yaw: float, life: float, spec: Dictionary = {}) -> void:
	if delay <= 0.01:
		_spawn(parent, res, pos, yaw, life, spec)
		return
	var timer := parent.get_tree().create_timer(delay)
	timer.timeout.connect(func() -> void:
		if is_instance_valid(parent):
			_spawn(parent, res, pos, yaw, life, spec))


# `yaw` = the caster's facing (Node3D.rotation.y of its view: the model looks down -Z at 0); `spec` = the table row
# (angle / offset / scale) the orientation follows
func _spawn(parent: Node, res: String, pos: Vector3, yaw: float, life: float, spec: Dictionary = {}) -> Node3D:
	var fx: Node3D = SfxScript.spawn(parent, dir, _file(res), pos, 0.0, life, false)
	if fx != null:
		spawned += 1
		orient(fx, spec, yaw + PI)
	return fx


# The world position of a hang point on a view [TK HangItemMgr of the bone prefab, npc_models.json]: a bone hinge (sys_bd =
# Bip001 Spine, sys_head) at its current animated place, sys_foot / sys_bar / sys_state at the model's own heights over the
# feet; the fixed table HANG_HEIGHT only for a view without a model (a marker) [tự chọn]
static func hang_pos(view: Node, hang: String, feet: Vector3) -> Vector3:
	var hn := hinge_of(view, hang)
	if hn != null:
		return hn.global_position
	var m = view.get("model") if view != null else null
	if m is Node3D and m.has_method("hang_height"):
		var h: float = m.hang_height(hang)
		if h >= 0.0:
			return feet + Vector3(0, h, 0)
	return feet + Vector3(0, float(HANG_HEIGHT.get(hang, 0.9)), 0)


# the bone hinge of a hang point on the view's model, null when the model has none of that name
static func hinge_of(view: Node, hang: String) -> Node3D:
	if view == null or hang == "":
		return null
	var m = view.get("model")
	if m is Node3D and m.has_method("hang_node"):
		return m.hang_node(hang)
	return null


# a holder that follows the hinge by position (sync 2), a child of the view so it goes with it
static func follower(view: Node, hinge: Node3D) -> Node3D:
	var f := Node3D.new()
	f.set_script(HangScript)
	f.name = "hang_" + str(hinge.name)
	f.target = hinge
	view.add_child(f)
	f.global_position = hinge.global_position
	return f


# The rotation of a child object / sfx object [TK ChildObject.InitRotation 0x4de510]: the base from the table's init
# angle - 0 the creator's Front (its facing; for a sfx_object 0 = local zero under its hang point, 4 = the source's
# angle: the same facing), 1 the hang point's rotation (the facing stands in), 2 world zero, 3 a random turn about Y -
# then Rotate(Rx, 0, Rz) in the object's own space (Unity's Euler: Z first, then X) and Ry about the world Y (the
# Front vector turned by Quaternion.Euler(0, Ry, 0)); the scale of the row after that (InitPosAndAngle 0x4df110).
# Mirrored to Godot (x -> -x): Ry and Rz change sign.  `forward_yaw` = the rotation.y at which an exported prefab
# (its forward +Z, like the Unity prefab after the mirror) points where the creator faces: the caster's yaw + PI
# because the caster's model looks down -Z; 0.0 when the effect is a child of a node already turned that way.
func orient(fx: Node3D, spec: Dictionary, forward_yaw: float) -> void:
	match int(spec.get("angle", 0)):
		2:
			fx.global_rotation = Vector3.ZERO
		3:
			fx.rotation = Vector3(0, randf() * TAU, 0)
		_:
			fx.rotation = Vector3(0, forward_yaw, 0)
	var off: Array = spec.get("offset", [])
	if off.size() == 3:
		fx.rotate_object_local(Vector3.RIGHT, deg_to_rad(float(off[0])))
		fx.rotate_object_local(Vector3.BACK, deg_to_rad(-float(off[2])))
		fx.rotate_y(deg_to_rad(-float(off[1])))
	var sc: Array = spec.get("scale", [])
	if sc.size() == 3 and float(sc[0]) > 0.0 and float(sc[1]) > 0.0 and float(sc[2]) > 0.0:
		fx.scale = Vector3(float(sc[0]), float(sc[1]), float(sc[2]))


# The hit picture on a struck entity [TK SkillHitNode -> sfx_object: sys_bd, sync 2 = linked to the target without its
# rotation, angle 2 = world zero (4 = the striker's facing)]: the flying child's own hit row, else any child's, else the
# element's generic hit (sfx_object 20..70 by the skill's series, the striker's series for a plain swing); as a child of
# the target's view so it follows.  `yaw_from` = the striker's facing for angle 4.
func hit_on(target: Node3D, skill_id: int, series: int, yaw_from: float = 0.0) -> Node3D:
	if not load_map() or target == null:
		return null
	var f := flying(skill_id)
	var spec: Dictionary = f.get("hit", {}) if f.has("hit") else _find_hit_entry(entry(skill_id).get("child", []))
	var res := str(spec.get("res", ""))
	if res == "":
		var el := element_of(skill_id) if skill_id > 0 else -1
		if el < 0:
			el = series
		res = str(map.get("element_hit", {}).get(str(el), ""))
		spec = {"angle": 2, "sync": 2, "hang": "sys_bd", "life_s": 1.0}
	if res == "":
		return null
	var hang := str(spec.get("hang", "sys_bd"))
	var pos: Vector3 = hang_pos(target, hang, target.global_position)
	var holder: Node = target if int(spec.get("sync", 2)) in [1, 2] else target.get_parent()
	if int(spec.get("sync", 2)) == 2:
		var hn := hinge_of(target, hang)
		if hn != null:
			holder = follower(target, hn)
	var fx: Node3D = SfxScript.spawn(holder, dir, _file(res), pos, 0.0, float(spec.get("life_s", 1.0)), false)
	if fx != null:
		spawned += 1
		orient(fx, spec, yaw_from + PI)
	return fx


# The flying effect as a child of a missile view (looping while it flies), null when the skill has none exported
func attach_flying(view: Node3D, skill_id: int) -> Node3D:
	if not load_map():
		return null
	var f := flying(skill_id)
	if f.is_empty():
		return null
	if f.has("orbit"):
		# moveType 7 [TK]: the effect circles the missile (KSkillOrbit3D); the born distance = the JX row's AttackRadius
		# in metres (the reference reads Atb_Skill_Distance of its skill script there)
		var orb := Node3D.new()
		orb.set_script(OrbitScript)
		orb.name = "Orbit"
		view.add_child(orb)
		var radius_m := float(Game.skill_row(skill_id).get("AttackRadius", "0")) * KScene3DMath.UNIT
		orb.call("setup", f["orbit"], radius_m, float(f.get("height", 0.0)) - 0.9)
		var ofx: Node3D = SfxScript.spawn(orb, dir, _file(str(f["res"])), orb.global_position, 0.0, 0.0, true)
		if ofx != null:
			ofx.position = Vector3.ZERO
			var ospec: Dictionary = f.duplicate()
			ospec["angle"] = 0
			orient(ofx, ospec, 0.0)
			spawned += 1
		return orb
	var fx: Node3D = SfxScript.spawn(view, dir, _file(str(f["res"])), view.global_position, 0.0, 0.0, true)
	if fx != null:
		fx.position = Vector3.ZERO
		# the missile view points its +Z along the flight (ChildObject.UpdateMove: set_forward of the move direction, no
		# pitch [TK 0x4e0b00]); the row's angle offsets / scale are the effect's own
		var spec: Dictionary = f.duplicate()
		spec["angle"] = 0
		orient(fx, spec, 0.0)
		spawned += 1
	return fx


# The halo of an aura / state (state_list: child object or sfx_object kept while the state holds [TK]) as a looping
# child of the entity's view at its feet (sys_foot) or chest; null when the skill has none
func aura(view: Node3D, skill_id: int) -> Node3D:
	if not load_map():
		return null
	var list: Array = entry(skill_id).get("aura", [])
	if list.is_empty():
		return null
	var c: Dictionary = list[0]
	var fx: Node3D = SfxScript.spawn(view, dir, _file(str(c["res"])), view.global_position, 0.0, 0.0, true)
	if fx != null:
		fx.position = view.to_local(hang_pos(view, str(c.get("hang", "")), view.global_position)) + Vector3(0, float(c.get("height", 0.0)), 0)
		# under the view (turned by the facing): angle 0 = the facing (local PI, the model looks down -Z), 2 = world zero
		var spec: Dictionary = c.duplicate()
		if int(spec.get("angle", 0)) == 2:
			spec["angle"] = 0
			orient(fx, spec, -float(view.rotation.y))
		else:
			orient(fx, spec, PI)
		spawned += 1
		return fx
	# the faction halos are SFXMixerMesh prefabs (a mesh the reference client builds at run time) the exporter cannot read
	# yet: a plain slowly turning ring in the element's colour stands in [tự chọn]
	var ring := MeshInstance3D.new()
	var tor := TorusMesh.new()
	tor.inner_radius = 1.0
	tor.outer_radius = 1.4
	tor.rings = 32
	tor.ring_segments = 6
	ring.mesh = tor
	var m := StandardMaterial3D.new()
	m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	m.blend_mode = BaseMaterial3D.BLEND_MODE_ADD
	m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	var el := element_of(skill_id)
	var colors := {0: Color(1.0, 0.9, 0.5, 0.7), 1: Color(0.5, 1.0, 0.5, 0.7), 2: Color(0.5, 0.8, 1.0, 0.7), 3: Color(1.0, 0.5, 0.3, 0.7), 4: Color(0.9, 0.8, 0.5, 0.7)}
	m.albedo_color = colors.get(el, Color(0.9, 0.9, 1.0, 0.7))
	ring.material_override = m
	ring.name = "aura_ring"
	view.add_child(ring)
	ring.position = Vector3(0, 0.25, 0)
	var tw := ring.create_tween().set_loops()
	tw.tween_property(ring, "rotation:y", TAU, 6.0).from(0.0)
	spawned += 1
	return ring


# The JX1 skill a StateSpecialId (the 0x7a packet's icons on other entities) belongs to, 0 when none
func skill_of_special(special_id: int) -> int:
	return int(map.get("skill_of_special", {}).get(str(special_id), 0))


# The missile-effect packet of the zone (the 2.0 KMissleEffect: the missile ended at `pos`): the skill's hit picture and
# whatever its flying child object leaves where it lands
func hit(parent: Node, skill_id: int, pos: Vector3) -> void:
	if not load_map():
		return
	var f := flying(skill_id)
	if not f.is_empty():
		_arrive_children(parent, f, pos, 0.0)
	elif hit_res(skill_id) != "":
		# no flier: the hit picture at the spot (the 2.0 collision movie's place; a struck entity gets its own via hit_on)
		var spec: Dictionary = _find_hit_entry(entry(skill_id).get("child", []))
		if spec.is_empty():
			spec = {"angle": 2, "sync": 2}
		_spawn(parent, hit_res(skill_id), pos, 0.0, 1.0, spec)
