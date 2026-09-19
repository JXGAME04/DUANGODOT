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
		if bool(c.get("fly", false)) and str(c.get("res", "")) != "":
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
	for c in list:
		if c.has("hit") and str(c["hit"].get("res", "")) != "":
			return str(c["hit"]["res"])
	for c in list:
		var h := _find_hit(c.get("children", []))
		if h != "":
			return h
	return ""


# A cast (EntityAction ACTION_ATTACK with a skill): the cast effects at the caster and the standing child objects at
# the aim, each at its fraction of the `frames` logic frames.  `at_caster` / `at_aim` are world positions of the feet.
func cast(parent: Node, skill_id: int, frames: int, at_caster: Vector3, yaw: float, at_aim: Vector3) -> void:
	if not load_map():
		return
	var e := entry(skill_id)
	var duration := float(maxi(frames, 1)) * TICK
	var any := false
	for c in e.get("cast", []):
		if bool(c.get("on_hit", false)):
			continue
		any = true
		var pos: Vector3 = at_caster + Vector3(0, float(HANG_HEIGHT.get(str(c.get("hang", "")), 0.9)), 0)
		_later(parent, float(c.get("at", 0.0)) * duration, str(c["res"]), pos, yaw, float(c.get("life_s", 1.0)))
	if _cast_children(parent, e.get("child", []), 0.0, duration, at_caster, yaw, at_aim):
		any = true
	if not any:
		# no mapped effect: the element's cast flash at the chest (sfx_object 1..4)
		var el := element_of(skill_id)
		var res := str(map.get("element_cast", {}).get(str(el), ""))
		if res != "":
			_later(parent, 0.0, res, at_caster + Vector3(0, 0.9, 0), yaw, 1.0)


# The standing child objects of a cast (and the children their own events spawn in time, "when" = time) at their
# fraction of the parent's life; flying ones are the zone's missiles (KMissle3DView + the missile-effect packet).
# Returns true when anything was put down.
func _cast_children(parent: Node, list: Array, start: float, span: float, at_caster: Vector3, yaw: float, at_aim: Vector3) -> bool:
	var any := false
	for c in list:
		if bool(c.get("fly", false)):
			continue
		var t0: float = start + float(c.get("at", 0.0)) * span
		var life := float(c.get("life_s", 1.5))
		var pos: Vector3 = at_caster if int(c.get("pos_type", 0)) == 0 else at_aim
		pos.y += float(c.get("height", 0.0))
		if str(c.get("hang", "")) != "":
			pos.y += float(HANG_HEIGHT.get(str(c.get("hang", "")), 0.0))
		if str(c.get("res", "")) != "":
			any = true
			_later(parent, t0, str(c["res"]), pos, yaw, life)
		if c.has("hit") and str(c["hit"].get("res", "")) != "":
			# a standing child object that hits (an area blast, a talisman on the target): its hit picture at the aim when it lands
			any = true
			var hp: Vector3 = at_aim + Vector3(0, float(HANG_HEIGHT.get(str(c["hit"].get("hang", "sys_bd")), 0.9)), 0)
			_later(parent, t0 + minf(life, 0.3), str(c["hit"]["res"]), hp, yaw, float(c["hit"].get("life_s", 1.0)))
		for f in c.get("fx", []):
			if str(f.get("when", "time")) == "time" and str(f.get("res", "")) != "":
				any = true
				_later(parent, t0 + float(f.get("at", 0.0)) * life, str(f["res"]), pos, yaw, float(f.get("life_s", 1.0)))
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
			if _cast_children(parent, timed, t0, life, pos, yaw, at_aim):
				any = true
	return any


# The children a flying child object spawns where it lands / hits ("when" = arrive / hit) and its own arrival sfx
func _arrive_children(parent: Node, c: Dictionary, pos: Vector3, yaw: float) -> void:
	for f in c.get("fx", []):
		var when := str(f.get("when", "time"))
		if (when == "arrive" or when == "hit") and str(f.get("res", "")) != "":
			_spawn(parent, str(f["res"]), pos, yaw, float(f.get("life_s", 1.0)))
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


func _later(parent: Node, delay: float, res: String, pos: Vector3, yaw: float, life: float) -> void:
	if delay <= 0.01:
		_spawn(parent, res, pos, yaw, life)
		return
	var timer := parent.get_tree().create_timer(delay)
	timer.timeout.connect(func() -> void:
		if is_instance_valid(parent):
			_spawn(parent, res, pos, yaw, life))


func _spawn(parent: Node, res: String, pos: Vector3, yaw: float, life: float) -> Node3D:
	var fx: Node3D = SfxScript.spawn(parent, dir, _file(res), pos, yaw, life, false)
	if fx != null:
		spawned += 1
	return fx


# The flying effect as a child of a missile view (looping while it flies), null when the skill has none exported
func attach_flying(view: Node3D, skill_id: int) -> Node3D:
	if not load_map():
		return null
	var f := flying(skill_id)
	if f.is_empty():
		return null
	var fx: Node3D = SfxScript.spawn(view, dir, _file(str(f["res"])), view.global_position, 0.0, 0.0, true)
	if fx != null:
		fx.position = Vector3.ZERO
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
		fx.position = Vector3(0, float(HANG_HEIGHT.get(str(c.get("hang", "")), 0.0)) + float(c.get("height", 0.0)), 0)
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
	var res := hit_res(skill_id)
	if res != "":
		_spawn(parent, res, pos, 0.0, 1.0)
	var f := flying(skill_id)
	if not f.is_empty():
		_arrive_children(parent, f, pos, 0.0)
