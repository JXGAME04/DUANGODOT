# The 3D picture of one entity (KNpcRes's part in 3D): a Node3D that follows the state of a KNpc / KObj node
# (scene_pos, res_dir, doing, frames, life...) the way KNpcRes follows it on the canvas.  The state machine stays
# in KNpc.gd (the 2.0 client's rules, docs/CLIENT-2.0.md); this node only shows it:
#   - position: scene units -> metres through the map (KScenePlace3D.to_world), the feet on the terrain;
#   - facing: yaw of res_dir (KNpc::Paint turns res_dir half way to dir every logic frame [2.0]); the yaw eases
#     between logic frames at the real frame rate (owner rule: movement interpolated to the frame rate);
#   - doing -> a clip of the model (Scn3DNpc, the reference client's anim_group names [TK]) with the length the
#     zone dictates (frames / 18 s), the death clip holds its last frame like KNpc::OnDeath;
#   - a marker capsule when the entity has no 3D model yet, a ring at the feet when it is the target.
extends Node3D

const KScene3DMath := preload("res://scenes3d/KScene3DMath.gd")
const KNpcResNode := preload("res://scenes/KNpcResNode.gd")
const MirrorScript := preload("res://scenes3d/KSpriteMirror3D.gd")
const TICK := 1.0 / 18.0
const TURN_TAU := 0.055        # seconds: the yaw closes 63 % of the gap in one logic frame [tự chọn]
const ENTITY_PLAYER := 1
const ENTITY_MONSTER := 3
const ENTITY_DROP := 4

var npc: Node = null           # the KNpc / KObj state node (a Node2D with no_2d)
var place: Node3D = null       # KScenePlace3D
var model: Node3D = null       # Scn3DNpc, null for the marker
var radius := 0.5              # pick cylinder around the feet, metres (cp_radius [TK]; 0.5 when unknown)
var bar_height := 2.0          # the name over the feet, metres (sys_bar [TK] or GetNpcPate [2.0])
var is_own := false
var _last_scene := Vector2(INF, INF)
var _ground := 0.0
var _yaw := 0.0
var _marker: MeshInstance3D = null
var _ring: MeshInstance3D = null
var _doing := -1
var mirror: Node3D = null      # KSpriteMirror3D in the 2.5D world: the 2.0 sprites of the entity as a board
var horse: Node3D = null       # Scn3DNpc of the mount while riding (anim_group 21 [TK]); the rider sits on its ma_qi1
var horse_loader: Callable     # (npc) -> Scn3DNpc, set by the world view (the horse of the worn item, or the default)
var _rider_group := ""         # the rider's weapon group to go back to when dismounting


# Binds the state node; `model_node` is a set-up Scn3DNpc (or null for a marker), `model_info` its npc_models.json row.
func bind(state: Node, place3d: Node3D, model_node: Node3D, model_info: Dictionary) -> void:
	npc = state
	place = place3d
	is_own = _flag(state, "is_own")
	if model_node != null:
		model = model_node
		model.name = "Model"
		add_child(model)
		bar_height = float(model.get("bar_height")) if model.get("bar_height") != null else 2.0
		var cp = model_info.get("cp_radius", null)
		if cp != null and float(cp) > 0.0:
			radius = float(cp)
	elif not _flag(state, "no_2d") and _flag(state, "has_res"):
		# the 2.5D world: the KNpcRes parts of the invisible canvas node, mirrored onto a board facing the camera
		mirror = Node3D.new()
		mirror.set_script(MirrorScript)
		mirror.name = "Mirror"
		add_child(mirror)
		mirror.bind(state)
		var pate_px: float = float(npc.get("stature")) if npc.get("stature") != null else 0.0
		if int(npc.get("entity_type")) == ENTITY_PLAYER:
			pate_px += 84.0
		bar_height = KScene3DMath.px_height_to_m(pate_px) if pate_px > 0.0 else 1.8
	else:
		if not _flag(state, "no_2d") and int(state.get("entity_type")) == ENTITY_DROP:
			# a thing on the ground in the 2.5D world: its own KObj picture
			mirror = Node3D.new()
			mirror.set_script(MirrorScript)
			mirror.name = "Mirror"
			add_child(mirror)
			mirror.bind(state)
		else:
			_make_marker()
		# GetNpcPate of the 2.0 client in metres: stature (+84 for players) screen px over the feet
		var pate: float = float(npc.get("stature")) if npc.get("stature") != null else 0.0
		if int(npc.get("entity_type")) == ENTITY_PLAYER:
			pate += 84.0
		bar_height = KScene3DMath.px_height_to_m(pate) if pate > 0.0 else 1.0
	if npc.has_signal("doing_changed"):
		npc.doing_changed.connect(_on_doing_changed)
	if npc.has_signal("riding_changed"):
		npc.riding_changed.connect(_on_riding_changed)
	if npc.get("dir64") != null:
		_yaw = KScene3DMath.yaw_of_dir(int(npc.res_dir))
		rotation.y = deg_to_rad(_yaw)
	_place(true)
	if model != null and bool(_flag(npc, "riding")):
		_on_riding_changed(true)
	if npc.get("doing") != null and npc.get("total_frame") != null:
		_on_doing_changed(int(npc.doing), int(npc.total_frame))
		# a late joiner sees a corpse at its last frame (KNpc.setup puts cur_frame at the end)
		if int(npc.doing) == KNpcResNode.Doing.DEATH and int(npc.cur_frame) >= int(npc.total_frame) - 1 and model != null:
			model.seek_end()


# a bool property of the state node, false when it has none (a KObj has no hovered / res_dir)
static func _flag(n: Node, prop: String) -> bool:
	var v = n.get(prop)
	return v != null and bool(v)


func _make_marker() -> void:
	var t := int(npc.get("entity_type"))
	if t == ENTITY_DROP and npc.get("obj") != null and str(npc.obj.get("image", "")) != "":
		# a thing on the ground: its ObjData picture (KObj::Draw [2.0]) as a billboard, 1 px = UNIT
		var atlas = Assets.sprite(str(npc.obj.get("image", "")))
		if atlas != null and atlas.frame_count() > 0:
			var spr := Sprite3D.new()
			spr.texture = atlas.frame_texture(0)
			spr.billboard = BaseMaterial3D.BILLBOARD_ENABLED
			spr.pixel_size = KScene3DMath.UNIT
			spr.centered = true
			spr.position = Vector3(0, float(spr.texture.get_height()) * KScene3DMath.UNIT * 0.5, 0)
			spr.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
			add_child(spr)
			radius = 0.3
			return
	_marker = MeshInstance3D.new()
	if t == ENTITY_DROP:
		var box := BoxMesh.new()
		box.size = Vector3(0.3, 0.3, 0.3)
		_marker.mesh = box
		_marker.position = Vector3(0, 0.15, 0)
	else:
		var cap := CapsuleMesh.new()
		cap.radius = 0.3
		cap.height = 1.7
		_marker.mesh = cap
		_marker.position = Vector3(0, 0.85, 0)
	var m := StandardMaterial3D.new()
	# the colours of KNpc._draw: me blue, other players green, monsters red, npcs orange, things yellow
	var color := Color(0.95, 0.6, 0.2)
	if t == ENTITY_PLAYER:
		color = Color(0.3, 0.9, 0.4)
	if is_own:
		color = Color(0.35, 0.65, 1.0)
	elif t == ENTITY_MONSTER:
		color = Color(0.95, 0.3, 0.3)
	elif t == ENTITY_DROP:
		color = Color(1.0, 0.85, 0.3)
	m.albedo_color = color
	_marker.material_override = m
	add_child(_marker)


func _make_ring() -> void:
	_ring = MeshInstance3D.new()
	var tor := TorusMesh.new()
	tor.inner_radius = maxf(radius, 0.4)
	tor.outer_radius = maxf(radius, 0.4) + 0.06
	tor.rings = 24
	tor.ring_segments = 6
	_ring.mesh = tor
	_ring.position = Vector3(0, 0.03, 0)
	var m := StandardMaterial3D.new()
	m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	m.albedo_color = Color(1.0, 0.9, 0.2, 0.85)
	m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	_ring.material_override = m
	_ring.top_level = true
	_ring.visible = false
	add_child(_ring)


func _place(snap: bool) -> void:
	var sp: Vector2 = npc.scene_pos
	if sp != _last_scene:
		if snap or sp.distance_to(_last_scene) > 6.0:   # a new ground sample every ~12 cm of travel
			var w: Vector3 = place.to_world(sp)
			_ground = place.ground_height(w.x, w.z)
		_last_scene = sp
	var world: Vector3 = place.to_world(sp)
	world.y = _ground
	global_position = world


func _process(delta: float) -> void:
	if npc == null or not is_instance_valid(npc):
		queue_free()
		return
	_place(false)
	if npc.get("res_dir") != null and mirror == null:
		var want := KScene3DMath.yaw_of_dir(int(npc.res_dir))
		var k := 1.0 - exp(-delta / TURN_TAU)
		_yaw = wrapf(_yaw + KScene3DMath.yaw_delta(_yaw, want) * k, -180.0, 180.0)
		rotation.y = deg_to_rad(_yaw)
	var target := _flag(npc, "is_target")
	if target and _ring == null:
		_make_ring()
	if _ring != null:
		_ring.visible = target and not (npc.has_method("is_dead") and bool(npc.call("is_dead")))
		_ring.global_position = global_position + Vector3(0, 0.03, 0)


# Mounted / dismounted: the horse model under the rider, the rider on the horse's ma_qi1 hang point with its riding
# clips (anim_group 20 of the reference: xx 85 sits, zp 84 rides, weapon 80/81, magic 82), the horse on group 21
func _on_riding_changed(on: bool) -> void:
	if model == null:
		return
	if on and horse == null and horse_loader.is_valid():
		var h: Node3D = horse_loader.call(npc)
		if h == null:
			return
		horse = h
		horse.name = "Horse"
		add_child(horse)
		var seat: Node = null
		var hang: Dictionary = horse.hangs.get("ma_qi1", {})
		if not hang.is_empty() and horse.model != null:
			seat = horse.model.find_child(str(hang.get("node_godot", "")), true, false)
			if seat == null:
				seat = horse.model.find_child(str(hang.get("node", "")), true, false)
		_rider_group = str(model.group)
		remove_child(model)
		if seat is Node3D:
			(seat as Node3D).add_child(model)
			model.position = Vector3(hang["pos"][0], hang["pos"][1], hang["pos"][2])
			model.quaternion = Quaternion(hang["quat"][0], hang["quat"][1], hang["quat"][2], hang["quat"][3])
			model.scale = Vector3(hang["scale"][0], hang["scale"][1], hang["scale"][2])
			# the rider's Model child was turned 180 for -Z; the seat node is in the horse's own frame: undo that turn
			if model.model != null:
				model.model.rotation.y = 0.0
		else:
			horse.add_child(model)
			model.position = Vector3(0, 1.3, 0)
		model.set_group("20")
		bar_height = float(horse.bar_height) + 0.6
	elif not on and horse != null:
		var parent := model.get_parent()
		if parent != null:
			parent.remove_child(model)
		add_child(model)
		model.transform = Transform3D.IDENTITY
		if model.model != null:
			model.model.rotation.y = PI
		model.set_group(_rider_group if _rider_group != "20" else "1")
		horse.queue_free()
		horse = null
		bar_height = float(model.get("bar_height")) if model.get("bar_height") != null else 2.0
	if npc.get("doing") != null and npc.get("total_frame") != null:
		_on_doing_changed(int(npc.doing), int(npc.total_frame))


# The horse rows of the 0xad sync changed while mounted: dismount and mount again so the world view picks the horse anew
func reload_horse() -> void:
	if horse == null or not (npc.get("riding") is bool and bool(npc.riding)):
		return
	_on_riding_changed(false)
	_on_riding_changed(true)


# KNpc's doing changed (its frame count too): the matching clip of the model, as long as the zone says
func _on_doing_changed(doing: int, total_frame: int) -> void:
	_doing = doing
	if model == null:
		return
	if horse != null:
		# the horse's own clips (group 21: xx stands, zp runs); the rider follows below with group 20
		match doing:
			KNpcResNode.Doing.WALK, KNpcResNode.Doing.FIGHT_WALK, KNpcResNode.Doing.RUN, KNpcResNode.Doing.FIGHT_RUN, KNpcResNode.Doing.JUMP:
				horse.walk()
			_:
				horse.idle()
	var seconds := float(maxi(total_frame, 1)) * TICK
	match doing:
		KNpcResNode.Doing.STAND, KNpcResNode.Doing.FIGHT_STAND, KNpcResNode.Doing.SIT:
			model.idle()
		KNpcResNode.Doing.STAND1:
			# the second idle (one cycle in six, KNpc::OnStand): the model's small motion when it has one
			if model.has_clip("xdz"):
				model.play_once("xdz", seconds)
			else:
				model.idle()
		KNpcResNode.Doing.WALK, KNpcResNode.Doing.FIGHT_WALK:
			model.walk()
		KNpcResNode.Doing.RUN, KNpcResNode.Doing.FIGHT_RUN, KNpcResNode.Doing.JUMP:
			model.run()
		KNpcResNode.Doing.ATTACK, KNpcResNode.Doing.ATTACK1:
			var cname: String = model.attack_clip()
			if cname == "" or model.play_once(cname, seconds) == "":
				model.idle()
		KNpcResNode.Doing.MAGIC:
			if model.play_once(model.clip_for("magic"), seconds) == "":
				model.idle()
		KNpcResNode.Doing.HURT:
			var hurt: String = model.pick_clip("ss")
			if hurt == "" or model.play_once(hurt, seconds) == "":
				model.idle()
		KNpcResNode.Doing.DEATH:
			var dead: String = model.pick_clip("sw")
			if dead != "":
				model.play_once(dead, seconds, true)
		_:
			model.idle()
