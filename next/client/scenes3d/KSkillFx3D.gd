# The 3D effects of the skills (docs/LO-TRINH-3D.md 3.1): client/assets3d/sfx/skill_map.json (tools/scn3d/map_skills.py)
# maps a JX1 skill id to the reference client's effects - the same skills by their Han names (怒雷指 = Nộ Lôi Chỉ) -
# as skill_section / skill_event / skill_childobj / sfx_object describe them [TK]:
#   - cast: sfx_object effects at the caster (at a fraction of the cast, the event's frame / the section's 30 frames);
#   - child: child objects - a flying one (sync 4) is what the zone's missile shows (KMissle3DView), a standing one is put
#     at the target point at its frame;
#   - hit: the child object's hit effect, else the element's generic hit (sfx_object 20..: 土/火/水/木/金系击中).
# Timing follows the zone: the cast lasts `frames` logic frames (18 Hz), fractions are taken of that.  The effects
# themselves are Scn3DSfx (CPUParticles3D / meshes from the exported prefabs).
extends RefCounted

const SfxScript := preload("res://scenes3d/Scn3DSfx.gd")
const KScene3DMath := preload("res://scenes3d/KScene3DMath.gd")
const TICK := 1.0 / 18.0
const HANG_HEIGHT := {"sys_bd": 0.9, "sys_bar": 2.0, "sys_foot": 0.0, "": 0.0}   # metres over the feet [tự chọn: sys_bd = chest]

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


# The flying child object of a skill (what its missile looks like), {} when the skill has none
func flying(skill_id: int) -> Dictionary:
	for c in entry(skill_id).get("child", []):
		if bool(c.get("fly", false)):
			return c
	return {}


# The hit effect of a skill: the flying child's own, else the element's generic one
func hit_res(skill_id: int) -> String:
	var f := flying(skill_id)
	if f.has("hit"):
		return str(f["hit"].get("res", ""))
	for c in entry(skill_id).get("child", []):
		if c.has("hit"):
			return str(c["hit"].get("res", ""))
	var el := element_of(skill_id)
	return str(map.get("element_hit", {}).get(str(el), ""))


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
	for c in e.get("child", []):
		if bool(c.get("fly", false)):
			continue   # the missile of the zone shows it (KMissle3DView)
		any = true
		var pos: Vector3 = at_caster if int(c.get("pos_type", 0)) == 0 else at_aim
		pos.y += float(c.get("height", 0.0))
		if str(c.get("hang", "")) != "":
			pos.y += float(HANG_HEIGHT.get(str(c.get("hang", "")), 0.0))
		_later(parent, float(c.get("at", 0.0)) * duration, str(c["res"]), pos, yaw, float(c.get("life_s", 1.5)))
	if not any:
		# no mapped effect: the element's cast flash at the chest (sfx_object 1..4)
		var el := element_of(skill_id)
		var res := str(map.get("element_cast", {}).get(str(el), ""))
		if res != "":
			_later(parent, 0.0, res, at_caster + Vector3(0, 0.9, 0), yaw, 1.0)


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


func hit(parent: Node, skill_id: int, pos: Vector3) -> void:
	if not load_map():
		return
	var res := hit_res(skill_id)
	if res != "":
		_spawn(parent, res, pos, 0.0, 1.0)
