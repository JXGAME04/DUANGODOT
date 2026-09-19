# The 3D world (KWorldView, ADR-008): the map through KScenePlace3D, an orbit camera (KCamera3D), every entity
# of the zone as a KNpc / KObj state node (the 2.0 client's rules, drawn nowhere: no_2d) with a KNpc3DView
# showing it, missiles as KMissle + KMissle3DView, names and life bars on a 2D layer (Label3D blurs), the
# fight sounds through the same KWavSound as the 2D view (a Camera2D over the character keeps its 2.0 pan
# rule).  Scene units of the zone <-> metres: docs/3D-QUY-UOC.md.
extends "res://scenes/KWorldView.gd"

const NpcScript := preload("res://scenes/KNpc.gd")
const ObjScript := preload("res://scenes/KObj.gd")
const MissleScript := preload("res://scenes/KMissle.gd")
const KWavSound := preload("res://scenes/KWavSound.gd")
const KScene3DMath := preload("res://scenes3d/KScene3DMath.gd")
const PlaceScript := preload("res://scenes3d/KScenePlace3D.gd")
const CameraScript := preload("res://scenes3d/KCamera3D.gd")
const NpcViewScript := preload("res://scenes3d/KNpc3DView.gd")
const MissleViewScript := preload("res://scenes3d/KMissle3DView.gd")
const ModelScript := preload("res://scenes3d/Scn3DNpc.gd")
const ENTITY_PLAYER := 1
const NAME_DIST := 60.0        # metres: names beyond this are not drawn [tự chọn]
const ANIM_DIST := 45.0        # metres: models beyond this stop animating (494 npcs at 145 FPS) [tự chọn]
const LIFE_BAR := Vector2(40, 4)   # KNpc::PaintLife [2.0]
const TrailScript := preload("res://scenes3d/Scn3DTrail.gd")
# The weapon in hand: the item worn in the weapon slot (KItemList::GetWeaponType 0x0060D660 [2.0]: detail / particular
# of the worn piece) -> the reference client's weapon list, which is JX1's own in the same order (铁匕首 = Thiết Trủy thủ,
# 钢剑 = Cang Kiếm...; weapons.json ids 1..10 kiếm, 51.. đao, 101.. thương, 151.. côn, 201.. song đao, 251.. song chùy
# [TK]): id = base of the type + (item level - 1).  JX1 particular of a melee weapon (items/base.json): 0 kiếm, 1 đao,
# 2 côn, 3 thương, 4 chùy, 5 song đao [2.0 tables]; ranged (detail 1) has no model yet.
const WEAPON_TYPE_OF_PARTICULAR := {0: 1, 1: 2, 2: 4, 3: 3, 4: 6, 5: 5}
const WEAPON_BASE_ID := {1: 1, 2: 51, 3: 101, 4: 151, 5: 201, 6: 251, 7: 301}
const ITEMPART_WEAPON := 3

var root: Node3D               # World3D: the map, the views, the camera
var place: Node3D              # KScenePlace3D
var cam_rig: Node3D            # KCamera3D
var _states: Node              # the KNpc / KObj / KMissle state nodes (Node2D, invisible)
var _views_root: Node3D
var _views := {}               # state node -> its 3D view
var _sound_root: Node2D
var _sound_cam: Camera2D
var _sounds: Node2D            # KWavSound
var _names: Control            # the 2D layer drawing names and life bars
var _models := {}              # maps/<id>/models.json: {"templates": {tid: cha}, "player": {sex: cha}, "models_dir"}
var _npc_models := {}          # npc/npc_models.json: cha -> {file, scale, name_vi, bar_y, ...}
var _npc_dir := ""
var _own: Node = null
var _lod_timer := 0.0
var right_click_handler: Callable   # UiGame's right mouse skill (a right click that was not a drag)
var _weapons := {}             # weapon/weapons.json: id -> {file, hangs, animgrp, anchors...}
var _weapon_dir := ""
var _own_weapon := ""          # weapons.json id on the character now ("" = bare hands)
var _trail: Node = null        # Scn3DTrail of the character's weapon


func is_3d() -> bool:
	return true


func _ready() -> void:
	root = Node3D.new()
	root.name = "World3D"
	add_child(root)
	place = Node3D.new()
	place.set_script(PlaceScript)
	place.name = "Place"
	root.add_child(place)
	_views_root = Node3D.new()
	_views_root.name = "Views"
	root.add_child(_views_root)
	cam_rig = Node3D.new()
	cam_rig.set_script(CameraScript)
	cam_rig.name = "CameraRig"
	root.add_child(cam_rig)
	cam_rig.right_click.connect(_on_right_click)
	_states = Node.new()
	_states.name = "States"
	add_child(_states)
	# the sounds: KWavSound's AudioStreamPlayer2Ds pan against a Camera2D that sits where the 2D view's would
	_sound_root = Node2D.new()
	_sound_root.name = "Sounds2D"
	add_child(_sound_root)
	_sound_cam = Camera2D.new()
	_sound_root.add_child(_sound_cam)
	_sounds = KWavSound.new()
	_sounds.name = "Sounds"
	_sounds.stream_provider = Assets.sound
	_sound_root.add_child(_sounds)
	var layer := CanvasLayer.new()
	layer.name = "Names"
	layer.layer = 1
	add_child(layer)
	_names = Control.new()
	_names.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_names.set_anchors_preset(Control.PRESET_FULL_RECT)
	_names.draw.connect(_draw_names)
	layer.add_child(_names)
	Game.items_changed.connect(_refresh_own_weapon)
	Game.item_changed.connect(func(_it: Dictionary): _refresh_own_weapon())
	Game.item_removed.connect(func(_id: int): _refresh_own_weapon())


# ---- the map --------------------------------------------------------------------------------------

func load_map() -> bool:
	var has_map: bool = place.load_map(Game.map_id)
	cam_rig.setup(place.camera_table() if has_map else {})
	_models = {}
	_npc_models = {}
	_npc_dir = ""
	if has_map:
		var base := "%s/maps/%d" % [Assets.assets3d_root(), Game.map_id]
		var mfile := base.path_join(str(place.info.get("models", "models.json")))
		var m = Assets.load_json(mfile) if FileAccess.file_exists(mfile) else null
		if m is Dictionary:
			_models = m
			_npc_dir = base.path_join(str(m.get("models_dir", "../../npc"))).simplify_path()
			var nm = Assets.load_json(_npc_dir.path_join("npc_models.json"))
			if nm is Dictionary:
				_npc_models = nm
	_weapon_dir = "%s/weapon" % Assets.assets3d_root()
	if _weapons.is_empty():
		var w = Assets.load_json(_weapon_dir.path_join("weapons.json")) if FileAccess.file_exists(_weapon_dir.path_join("weapons.json")) else null
		if w is Dictionary:
			_weapons = w
	Log.info("map3d", "world view", {"map": Game.map_id, "bundle": has_map, "models": _npc_models.size(), "templates": _models.get("templates", {}).size(), "weapons": _weapons.size()})
	return has_map


func map_name() -> String:
	return place.map_name()


func sounds():
	return _sounds


# ---- entities --------------------------------------------------------------------------------------

func add_entity(d: Dictionary, own: bool, existing: Node = null) -> Node:
	var node: Node2D = existing
	if node == null:
		node = Node2D.new()
		node.set_script(ObjScript if int(d.get("type", 0)) == ENTITY_DROP else NpcScript)
		node.no_2d = true
		_states.add_child(node)
	node.setup(d, own)
	if own:
		_own = node
	var old = _views.get(node)
	if old != null and is_instance_valid(old):
		old.queue_free()
	var mv := _model_for(node)
	var view := Node3D.new()
	view.set_script(NpcViewScript)
	view.name = "e%d" % int(d.id)
	_views_root.add_child(view)
	view.bind(node, place, mv[0], mv[1])
	_views[node] = view
	if own:
		_own_weapon = ""
		_refresh_own_weapon()
	return node


func remove_entity(node: Node) -> void:
	var view = _views.get(node)
	if view != null and is_instance_valid(view):
		view.queue_free()
	_views.erase(node)
	if node == _own:
		_own = null


# The 3D model of an entity: players by sex, the rest by their npcs.txt template through the map's models.json
# (template -> cha_pic of the reference models [TK]); null = a marker capsule.
func _model_for(node: Node) -> Array:
	var cha := 0
	var t := int(node.get("entity_type"))
	if t == ENTITY_PLAYER:
		cha = int(_models.get("player", {}).get(str(int(node.get("sex"))), 0))
	elif t != ENTITY_DROP:
		cha = int(_models.get("templates", {}).get(str(int(node.get("template_id"))), 0))
	if cha == 0:
		return [null, {}]
	var mi = _npc_models.get(str(cha))
	if not (mi is Dictionary):
		return [null, {}]
	var m := Node3D.new()
	m.set_script(ModelScript)
	var size_y: float = float(mi.get("sizeY", 0.0)) if mi.get("sizeY") != null else 0.0
	if not m.setup(_npc_dir, str(mi.get("file", "")), float(mi.get("scale", 1.0)), str(node.get("display_name")), size_y, mi):
		m.queue_free()
		return [null, {}]
	return [m, mi]


# The reference weapon id of the worn weapon, "" for bare hands / no model
func _own_weapon_id() -> String:
	var wid := Game.item_worn(ITEMPART_WEAPON)
	if wid == 0 or not Game.items.has(wid):
		return ""
	var it: Dictionary = Game.items[wid]
	if int(it.get("detail", -1)) != 0:
		return ""
	var t: int = WEAPON_TYPE_OF_PARTICULAR.get(int(it.get("particular", -1)), 0)
	if t == 0:
		return ""
	var id := str(int(WEAPON_BASE_ID[t]) + clampi(int(it.get("level", 1)) - 1, 0, 9))
	return id if _weapons.has(id) else ""


func _refresh_own_weapon() -> void:
	if _own == null or not is_instance_valid(_own):
		return
	var view = _views.get(_own)
	if view == null or view.model == null:
		return
	var id := _own_weapon_id()
	if id == _own_weapon:
		return
	_own_weapon = id
	var m: Node3D = view.model
	if _trail != null:
		_trail.queue_free()
		_trail = null
	if id == "":
		m.clear_weapons()
		m.set_group("1")
		return
	var w: Dictionary = _weapons[id]
	var n: int = m.attach_weapon(_weapon_dir, w)
	var g := str(int(w.get("animgrp", 0)))
	m.set_group(g if g != "0" else "1")
	if n > 0 and not m.weapon_nodes.is_empty():
		_trail = TrailScript.new()
		_trail.name = "Trail"
		root.add_child(_trail)
		_trail.setup(m.weapon_nodes[0], w.get("anchors", {}))
	Log.debug("map3d", "weapon in hand", {"weapon": id, "name": w.get("name", ""), "hangs": n, "group": g})


# ---- missiles --------------------------------------------------------------------------------------

func add_missle(d: Dictionary, row: Dictionary) -> Node:
	var node = MissleScript.new()
	node.no_2d = true
	node.sounds = _sounds
	_states.add_child(node)
	node.setup(d, row)
	var view := Node3D.new()
	view.set_script(MissleViewScript)
	_views_root.add_child(view)
	view.bind(node, place)
	return node


func add_missle_effect(_anim: Dictionary, _dir64: int, scene_pos: Vector2, z: int) -> void:
	var w: Vector3 = place.to_world(scene_pos)
	w.y = place.ground_height(w.x, w.z) + 0.9 + KScene3DMath.px_height_to_m(float(z))
	var burst: CPUParticles3D = MissleViewScript._make_burst(Color(1.0, 0.85, 0.4))
	_views_root.add_child(burst)
	burst.global_position = w
	burst.emitting = true
	get_tree().create_timer(1.2).timeout.connect(burst.queue_free)


# ---- camera and cursor ---------------------------------------------------------------------------

func follow(own: Node, snap: bool) -> void:
	var view = _views.get(own)
	if view == null:
		return
	cam_rig.follow = view
	if snap:
		cam_rig.snap_to(view.global_position + Vector3(0, cam_rig.target_height, 0))
	_sound_cam.position = Vector2(own.scene_pos.x, own.scene_pos.y * 0.5).round()


func center_on(scene: Vector2) -> void:
	cam_rig.follow = null
	var w: Vector3 = place.to_world(scene)
	w.y = place.ground_height(w.x, w.z) + cam_rig.target_height
	cam_rig.snap_to(w)


func _cam() -> Camera3D:
	return cam_rig.cam


func screen_to_scene(screen: Vector2) -> Vector2:
	var cam := _cam()
	var hit: Vector3 = place.ray_ground(cam.project_ray_origin(screen), cam.project_ray_normal(screen))
	return place.to_scene(hit)


func scene_to_screen(scene: Vector2, height_px: float = 0.0) -> Vector2:
	var w: Vector3 = place.to_world(scene)
	w.y = place.ground_height(w.x, w.z) + KScene3DMath.px_height_to_m(height_px)
	return _cam().unproject_position(w)


# The entity under the cursor: the ray against a cylinder around each entity's feet (cp_radius, sys_bar high [TK]);
# the nearest hit wins (the 2.0 client takes the topmost drawn sprite, UiGame._entity_at)
func pick(screen: Vector2, entities: Dictionary) -> Node:
	var cam := _cam()
	var from := cam.project_ray_origin(screen)
	var dirv := cam.project_ray_normal(screen)
	var best: Node = null
	var best_t := INF
	for node in entities.values():
		var view = _views.get(node)
		if view == null or not is_instance_valid(view):
			continue
		var t := _ray_cylinder(from, dirv, view.global_position, float(view.radius), float(view.bar_height))
		if t >= 0.0 and t < best_t:
			best_t = t
			best = node
	return best


# The ray parameter where it enters a vertical cylinder (axis through `foot`, radius r, from the feet up h), -1 when it misses.
static func _ray_cylinder(o: Vector3, d: Vector3, foot: Vector3, r: float, h: float) -> float:
	var ox := o.x - foot.x
	var oz := o.z - foot.z
	var a := d.x * d.x + d.z * d.z
	var b := 2.0 * (ox * d.x + oz * d.z)
	var c := ox * ox + oz * oz - r * r
	var t := -1.0
	if a < 1e-6:
		t = 0.0 if c <= 0.0 else -1.0
	else:
		var disc := b * b - 4.0 * a * c
		if disc < 0.0:
			return -1.0
		var sq := sqrt(disc)
		var t0 := (-b - sq) / (2.0 * a)
		var t1 := (-b + sq) / (2.0 * a)
		# the side hit, or the cap when the ray starts inside / passes over the top
		for cand in [t0, t1]:
			if cand < 0.0:
				continue
			var y: float = o.y + d.y * cand
			if y >= foot.y - 0.1 and y <= foot.y + h:
				t = cand
				break
		if t < 0.0 and absf(d.y) > 1e-6:
			var tc := (foot.y + h - o.y) / d.y
			if tc >= 0.0 and tc >= t0 and tc <= t1:
				t = tc
	return t


func zoom_step(steps: int) -> void:
	cam_rig.zoom_step(steps)


func _on_right_click(screen: Vector2) -> void:
	if right_click_handler.is_valid():
		right_click_handler.call(screen)


# ---- per frame --------------------------------------------------------------------------------------

func update(delta: float) -> void:
	_names.queue_redraw()
	if _trail != null and _own != null and is_instance_valid(_own):
		var view = _views.get(_own)
		_trail.active = view != null and view.model != null and view.model.busy and str(view.model.current).begins_with("gj")
	_lod_timer -= delta
	if _lod_timer <= 0.0:
		_lod_timer = 0.5
		_update_lod()


func _update_lod() -> void:
	var own_view = _views.get(_own) if _own != null else null
	if own_view == null:
		return
	var p: Vector3 = own_view.global_position
	for view in _views.values():
		if not is_instance_valid(view) or view.model == null or view.model.anim == null:
			continue
		var near: bool = view.global_position.distance_to(p) < ANIM_DIST
		if view.model.anim.active != near:
			view.model.anim.active = near


# Names and life bars (KNpc::Paint / PaintLife rules [2.0]) at the unprojected point over each entity's feet.
func _draw_names() -> void:
	var cam := _cam()
	if cam == null:
		return
	var font := ThemeDB.fallback_font
	var own_view = _views.get(_own) if _own != null else null
	var own_pos: Vector3 = own_view.global_position if own_view != null else cam.global_position
	for node in _views.keys():
		var view = _views[node]
		if not is_instance_valid(view) or not is_instance_valid(node) or node.get("display_name") == null:
			continue
		var top: Vector3 = view.global_position + Vector3(0, float(view.bar_height), 0)
		if top.distance_to(own_pos) > NAME_DIST or cam.is_position_behind(top):
			continue
		var sp := cam.unproject_position(top)
		var text: String = str(node.display_name)
		var size := font.get_string_size(text, HORIZONTAL_ALIGNMENT_CENTER, -1, 15)
		var at := Vector2(sp.x - size.x * 0.5, sp.y - 6.0)
		var color := Color.WHITE
		if int(node.get("entity_type")) == ENTITY_DROP:
			color = Color(1.0, 0.85, 0.3)
		_names.draw_string_outline(font, at, text, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, 4, Color(0, 0, 0, 0.9))
		_names.draw_string(font, at, text, HORIZONTAL_ALIGNMENT_LEFT, -1, 15, color)
		# KNpc::PaintLife: a life bar under the name for wounded or selected characters
		var life := int(node.get("life")) if node.get("life") != null else 0
		var life_max := int(node.get("life_max")) if node.get("life_max") != null else 0
		var dead: bool = bool(node.call("is_dead")) if node.has_method("is_dead") else false
		if life_max > 0 and (life < life_max or bool(node.get("is_target"))) and not dead:
			var bar_at := Vector2(sp.x - LIFE_BAR.x * 0.5, sp.y - 2.0)
			_names.draw_rect(Rect2(bar_at, LIFE_BAR), Color(0, 0, 0, 0.7))
			var w := LIFE_BAR.x * clampf(float(life) / float(life_max), 0.0, 1.0)
			_names.draw_rect(Rect2(bar_at, Vector2(w, LIFE_BAR.y)), Color(0.2, 0.8, 0.3) if bool(node.get("is_own")) else Color(0.85, 0.15, 0.15))


func camera_state() -> String:
	return cam_rig.state_text()
