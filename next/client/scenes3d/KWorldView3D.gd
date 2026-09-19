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
const MissleEffectScript := preload("res://scenes/KMissleEffect.gd")
const MirrorScript := preload("res://scenes3d/KSpriteMirror3D.gd")
const ModelScript := preload("res://scenes3d/Scn3DNpc.gd")
const KNpcGold := preload("res://scenes/KNpcGold.gd")
const SkillFxScript := preload("res://scenes3d/KSkillFx3D.gd")
const ACTION_ATTACK := 1
const ENTITY_PLAYER := 1
const NAME_DIST := 60.0        # metres: names beyond this are not drawn [tự chọn]
const ANIM_DIST := 45.0        # metres: models beyond this stop animating (494 npcs at 145 FPS) [tự chọn]
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
var _weapon_of := {}           # state node -> weapons.json id on it now (every player: ours from the bag, others from the 0xad sync)
var _res_inverse := {}         # items/item_res.json inverted: "melee"/"horse" -> res row -> [particular, level] (the first item of the row)
var _res_tables := {}          # items/item_res.json as loaded: "melee"/"horse" -> rows (KItemChangeRes: row = particular * 10 + level + 2)
var _trail: Node = null        # Scn3DTrail of the character's weapon
var _trails := {}              # state node -> Scn3DTrail (every player with a weapon model)
# anim_effect rows 1..6 [TK]: the weapon's quality picks the trail prefab (Daoguang/dg_xw_cmn_*: white, blue, purple, gold,
# wgold, xgold); the 2.0 item colour rule (KItem::GetDesc, KUiItemView.name_tag) gives ours: yellow = gold, violet = purple,
# a magic weapon = blue, else white. Other players' weapons come as res rows only -> white.
const TRAIL_BY_COLOUR := {"white": "Daoguang_dg_xw_cmn_white", "blue": "Daoguang_dg_xw_cmn_blue", "purple": "Daoguang_dg_xw_cmn_purple",
	"gold": "Daoguang_dg_xw_cmn_gold"}
var _last_dir_offset := -1
var _auras := {}               # state node -> {skill_id: fx node} the halos it shows now
var fx = SkillFxScript.new()   # KSkillFx3D: the skill effects of skill_map.json (3D maps)


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
	Game.state_changed.connect(func(_sid: int): _refresh_auras(_own))
	Game.state_icons_changed.connect(func(eid: int): _refresh_auras_of(eid))
	Game.item_changed.connect(func(_it: Dictionary): _refresh_own_weapon())
	Game.item_removed.connect(func(_id: int): _refresh_own_weapon())
	Game.entity_res.connect(func(r: Dictionary): _refresh_weapon_of.call_deferred(int(r.id)))
	_load_res_inverse()


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


func map_info() -> Dictionary:
	return place.map_info()


func region_count() -> int:
	return place.region_count()


func anim_count() -> int:
	return place.anim_count()


func sounds():
	return _sounds


# ---- entities --------------------------------------------------------------------------------------

func add_entity(d: Dictionary, own: bool, existing: Node = null) -> Node:
	var node: Node2D = existing
	var sprites: bool = place.mode == "2.5d"   # the 2D map's world: the entities keep their 2.0 sprites (mirrored boards)
	if node == null:
		node = Node2D.new()
		node.set_script(ObjScript if int(d.get("type", 0)) == ENTITY_DROP else NpcScript)
		node.no_2d = not sprites
		if int(d.get("type", 0)) != ENTITY_DROP:
			node.sounds = _sounds   # the action sounds (KNpcRes::PlaySound)
		_states.add_child(node)
	node.setup(d, own)
	if sprites:
		node.visible = false   # painted by KNpcRes on the invisible canvas, shown through KSpriteMirror3D
		if node.get("view_dir_offset") != null:
			node.view_dir_offset = _view_dir_offset()
	if own:
		_own = node
	var old = _views.get(node)
	if old != null and is_instance_valid(old):
		old.queue_free()
	var mv := _model_for(node)
	var view := Node3D.new()
	view.set_script(NpcViewScript)
	view.name = "e%d" % int(d.id)
	view.horse_loader = _horse_for
	_views_root.add_child(view)
	view.bind(node, place, mv[0], mv[1])
	_views[node] = view
	_weapon_of.erase(node)
	if int(d.get("type", 0)) == ENTITY_PLAYER:
		_refresh_weapon(node)
	_auras.erase(node)
	_refresh_auras(node)
	return node


# The halos of the states an entity holds: our own from Game.states (skill ids), the others from their state icons
# (StateSpecialId -> skill through skill_map.json); spawned looping at the feet, dropped when the state goes
func _refresh_auras_of(entity_id: int) -> void:
	for node in _views.keys():
		if is_instance_valid(node) and int(node.get("entity_id")) == entity_id:
			_refresh_auras(node)
			return


func _refresh_auras(node: Node) -> void:
	if node == null or not is_instance_valid(node) or place.mode != "3d":
		return
	var view = _views.get(node)
	if view == null or not is_instance_valid(view):
		return
	var want := {}
	if node == _own:
		for sid in Game.states.keys():
			if not fx.entry(int(sid)).get("aura", []).is_empty():
				want[int(sid)] = true
	else:
		var icons = node.get("state_icons")
		if icons is Array:
			for sp in icons:
				var sid := fx.skill_of_special(int(sp))
				if sid > 0 and not fx.entry(sid).get("aura", []).is_empty():
					want[sid] = true
	var have: Dictionary = _auras.get(node, {})
	for sid in have.keys():
		if not want.has(sid):
			if is_instance_valid(have[sid]):
				have[sid].queue_free()
			have.erase(sid)
	for sid in want.keys():
		if not have.has(sid):
			var f := fx.aura(view, sid)
			if f != null:
				have[sid] = f
	_auras[node] = have


func remove_entity(node: Node) -> void:
	_auras.erase(node)
	_weapon_of.erase(node)
	var tr = _trails.get(node)
	if tr != null and is_instance_valid(tr):
		tr.queue_free()
	_trails.erase(node)
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
	if cha == 0 or place.mode != "3d":
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
	m.cha = cha
	return [m, mi]


# items/item_res.json (the *Res.txt of the old server, KItemChangeRes): every res row of the melee / horse table back to the
# first (particular, level) that draws it - the 0xad sync of other players names the row only (KNpc::SetPlayerRes 0x005ED920)
func _load_res_inverse() -> void:
	var p := "%s/items/item_res.json" % Assets.assets_root()
	var j = Assets.load_json(p) if FileAccess.file_exists(p) else null
	if not (j is Dictionary):
		return
	_res_tables = j
	for key in ["melee", "horse"]:
		var inv := {}
		var rows: Array = j.get(key, [])
		for i in rows.size():
			var row := i + 1
			if row < 3 or not (rows[i] is Array) or rows[i].size() < 2:
				continue
			# KItemChangeRes::weapon_res / horse_res: row = particular * 10 + level + 2, res = column 2 - 2
			@warning_ignore("integer_division")
			var particular: int = (row - 3) / 10
			var level: int = (row - 3) % 10 + 1
			var res := int(rows[i][1]) - 2
			if not inv.has(res):
				inv[res] = [particular, level]
		_res_inverse[key] = inv


# KItemChangeRes::Get*Res of the old server: the picture row an item (particular, level) draws, -1 = none
func _item_res_row(key: String, particular: int, level: int) -> int:
	var rows: Array = _res_tables.get(key, [])
	var row := particular * 10 + level + 2
	if row < 1 or row > rows.size() or not (rows[row - 1] is Array) or rows[row - 1].size() < 2:
		return -1
	return int(rows[row - 1][1]) - 2


# The horse picture row of a state node: the 0xad rows when the zone sent them (ours too), else our own worn horse item
func _horse_res_of(node: Node) -> int:
	var rows = node.get("equip_rows")
	if rows is Dictionary and rows.has(3) and int(rows[3]) >= 0:
		return int(rows[3])
	if node == _own:
		var hid := Game.item_worn(10)   # itempart_horse
		if hid != 0 and Game.items.has(hid):
			var it: Dictionary = Game.items[hid]
			return _item_res_row("horse", int(it.get("particular", -1)), int(it.get("level", 1)))
	return -1


# (particular, level) of the weapon on a state node: ours from the worn item, others from their weapon res row
func _weapon_particular(node: Node) -> Array:
	if node == _own:
		var wid := Game.item_worn(ITEMPART_WEAPON)
		if wid == 0 or not Game.items.has(wid):
			return []
		var it: Dictionary = Game.items[wid]
		if int(it.get("detail", -1)) != 0:
			return []
		return [int(it.get("particular", -1)), int(it.get("level", 1))]
	var rows = node.get("equip_rows")
	if rows is Dictionary and rows.has(2) and int(rows[2]) >= 0:
		var pl = _res_inverse.get("melee", {}).get(int(rows[2]), null)
		if pl is Array:
			return pl
	return []


# The reference weapon id for a state node, "" for bare hands / no model: the reference list is JX1 weapons in the same order
func _weapon_id_of(node: Node) -> String:
	var pl := _weapon_particular(node)
	if pl.is_empty():
		return ""
	var t: int = WEAPON_TYPE_OF_PARTICULAR.get(int(pl[0]), 0)
	if t == 0:
		return ""
	var id := str(int(WEAPON_BASE_ID[t]) + clampi(int(pl[1]) - 1, 0, 9))
	return id if _weapons.has(id) else ""


func _refresh_own_weapon() -> void:
	if _own != null and is_instance_valid(_own):
		_refresh_weapon(_own)


# The XWeaponTrail parameters for a player's weapon: its 2.0 colour -> anim_effect row -> the Daoguang prefab's "xtrail"
func _trail_params(node: Node) -> Dictionary:
	var colour := "white"
	if node == _own:
		var wid := Game.item_worn(ITEMPART_WEAPON)
		if wid != 0 and Game.items.has(wid):
			var it: Dictionary = Game.items[wid]
			var q := int(it.get("ex_type", 0))
			var magic: Array = it.get("magic", [])
			if q == 1 or q == 4 or q == 5:
				colour = "gold"
			elif q == 2:
				colour = "purple"
			elif magic.size() > 0 and int(magic[0].get("type", 0)) != 0:
				colour = "blue"
	var file := str(TRAIL_BY_COLOUR.get(colour, "Daoguang_dg_xw_cmn_white"))
	var p := "%s/sfx/%s.json" % [Assets.assets3d_root(), file]
	if not FileAccess.file_exists(p):
		return {}
	var d = Assets.load_json(p)
	if not (d is Dictionary):
		return {}
	for jn in d.get("nodes", []):
		if jn.has("xtrail"):
			return jn["xtrail"]
	return {}


# --auto3d proof: what the 0xad rows of our own character would give another client (the "others" path of
# _weapon_particular / _horse_for) next to what the bag gives us - the two must name the same weapon / horse
func debug_equip_rows() -> Dictionary:
	var out := {"weapon_bag": _own_weapon, "weapon_rows": "", "horse_rows": -1, "rows": {}}
	if _own == null or not is_instance_valid(_own):
		return out
	var rows = _own.get("equip_rows")
	if not (rows is Dictionary):
		return out
	out["rows"] = rows
	if rows.has(2) and int(rows[2]) >= 0:
		var pl = _res_inverse.get("melee", {}).get(int(rows[2]), null)
		if pl is Array:
			var t: int = WEAPON_TYPE_OF_PARTICULAR.get(int(pl[0]), 0)
			if t != 0:
				out["weapon_rows"] = str(int(WEAPON_BASE_ID[t]) + clampi(int(pl[1]) - 1, 0, 9))
	if rows.has(3) and int(rows[3]) >= 0:
		var by_res: Dictionary = _models.get("horses", {}).get("by_res", {})
		if by_res.has(str(int(rows[3]))):
			out["horse_rows"] = int(by_res[str(int(rows[3]))])
	var hid := Game.item_worn(10)
	if hid != 0 and Game.items.has(hid):
		var it: Dictionary = Game.items[hid]
		out["horse_item_row"] = _item_res_row("horse", int(it.get("particular", -1)), int(it.get("level", 1)))
	return out


# the 0xad sync of a player: weapon and horse pictures picked again (KNpc::SetPlayerRes -> the res rows)
func _refresh_weapon_of(entity_id: int) -> void:
	for node in _views.keys():
		if is_instance_valid(node) and int(node.get("entity_id")) == entity_id:
			_refresh_weapon(node)
			var view = _views.get(node)
			if view != null and is_instance_valid(view) and view.has_method("reload_horse"):
				view.reload_horse()
			return


func _refresh_weapon(node: Node) -> void:
	var view = _views.get(node)
	if view == null or not is_instance_valid(view) or view.model == null:
		return
	var id := _weapon_id_of(node)
	if id == str(_weapon_of.get(node, "")):
		return
	_weapon_of[node] = id
	if node == _own:
		_own_weapon = id
	var m: Node3D = view.model
	var old_trail = _trails.get(node)
	if old_trail != null and is_instance_valid(old_trail):
		old_trail.queue_free()
	_trails.erase(node)
	if node == _own:
		_trail = null
	if id == "":
		m.clear_weapons()
		view.set_weapon_group("1")
		return
	var w: Dictionary = _weapons[id]
	var n: int = m.attach_weapon(_weapon_dir, w)
	var g := str(int(w.get("animgrp", 0)))
	view.set_weapon_group(g if g != "0" else "1")
	if n > 0 and not m.weapon_nodes.is_empty():
		var tr = TrailScript.new()
		tr.name = "Trail"
		root.add_child(tr)
		tr.setup(m.weapon_nodes[0], w.get("anchors", {}))
		var xt := _trail_params(node)
		if not xt.is_empty():
			tr.apply_params("%s/sfx" % Assets.assets3d_root(), xt)
		_trails[node] = tr
		if node == _own:
			_trail = tr
	Log.debug("map3d", "weapon in hand", {"entity": node.get("entity_id"), "weapon": id, "name": w.get("name", ""), "hangs": n, "group": g})


# The horse of an entity: the 2.0 client draws the horse of the HorseRes.txt picture row (KItemChangeRes::GetHorseRes; the
# 0xad sync carries that row for everyone) - models.json "horses" maps the row (its Chinese picture name) to a reference
# cha_pic, rows without a model get the default horse
func _horse_for(node: Node) -> Node3D:
	var horses: Dictionary = _models.get("horses", {})
	var cha := int(horses.get("default", 1500))
	var res := _horse_res_of(node)
	var by_res: Dictionary = horses.get("by_res", {})
	if res >= 0 and by_res.has(str(res)):
		cha = int(by_res[str(res)])
	var mi = _npc_models.get(str(cha))
	if not (mi is Dictionary):
		return null
	var h := Node3D.new()
	h.set_script(ModelScript)
	if not h.setup(_npc_dir, str(mi.get("file", "")), float(mi.get("scale", 1.0)), "", 0.0, mi):
		h.queue_free()
		return null
	h.cha = cha
	h.set_group("21")
	return h


# ---- missiles --------------------------------------------------------------------------------------

func add_missle(d: Dictionary, row: Dictionary) -> Node:
	var node = MissleScript.new()
	node.no_2d = place.mode != "2.5d"
	node.sounds = _sounds
	_states.add_child(node)
	node.setup(d, row)
	if place.mode == "2.5d":
		node.visible = false
	var view := Node3D.new()
	view.set_script(MissleViewScript)
	_views_root.add_child(view)
	view.bind(node, place, fx if place.mode == "3d" else null)
	return node


func add_missle_effect(anim: Dictionary, dir64: int, scene_pos: Vector2, z: int, skill_id: int = 0) -> void:
	if place.mode == "2.5d":
		# the 2.0 collision movie itself (KMissleEffect on the invisible canvas), mirrored onto a board
		var fx = MissleEffectScript.new()
		_states.add_child(fx)
		fx.setup(anim, dir64, scene_pos, z)
		fx.visible = false
		var v := Node3D.new()
		_views_root.add_child(v)
		var w2: Vector3 = place.to_world(scene_pos)
		w2.y = place.ground_height(w2.x, w2.z) + KScene3DMath.px_height_to_m(float(z))
		v.global_position = w2
		var m := Node3D.new()
		m.set_script(MirrorScript)
		v.add_child(m)
		m.bind(fx)
		fx.tree_exited.connect(v.queue_free)
		return
	var w: Vector3 = place.to_world(scene_pos)
	w.y = place.ground_height(w.x, w.z) + 0.9 + KScene3DMath.px_height_to_m(float(z))
	if skill_id > 0 and fx.load_map() and (fx.hit_res(skill_id) != "" or not fx.flying(skill_id).get("children", []).is_empty()):
		fx.hit(_views_root, skill_id, w)   # the skill's own hit effect + what the flying child leaves [TK]
		return
	var burst: CPUParticles3D = MissleViewScript._make_burst(Color(1.0, 0.85, 0.4))
	_views_root.add_child(burst)
	burst.global_position = w
	burst.emitting = true
	get_tree().create_timer(1.2).timeout.connect(burst.queue_free)


# A cast (ACTION_ATTACK with a skill) on a 3D map: the skill's effects at the caster / the aim, timed on the zone's frames
func action(node: Node, a: Dictionary) -> void:
	if place.mode != "3d" or int(a.get("action", -1)) != ACTION_ATTACK or int(a.get("skill", 0)) <= 0:
		return
	var view = _views.get(node)
	if view == null or not is_instance_valid(view):
		return
	Log.debug("map3d", "cast effect", {"entity": a.get("id"), "skill": a.skill, "frames": a.get("frames", 0), "mapped": fx.load_map() and not fx.entry(int(a.skill)).is_empty(), "element": fx.element_of(int(a.skill))})
	var aim: Vector3 = place.to_world(Vector2(float(a.get("ax", a.get("x", 0))), float(a.get("ay", a.get("y", 0)))))
	aim.y = place.ground_height(aim.x, aim.z)
	fx.cast(_views_root, int(a.skill), int(a.get("frames", 1)), view.global_position, float(view.rotation.y), aim, view)


# ---- camera and cursor ---------------------------------------------------------------------------

func follow(own: Node, snap: bool, _delta: float = 0.0) -> void:
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

# a 2.5D sprite's painted facing is relative to the camera: dir 0 (down the 2.0 screen) faces the camera
func _view_dir_offset() -> int:
	return 32 - KScene3DMath.dir_of_yaw(cam_rig.yaw)


func update(delta: float) -> void:
	_names.queue_redraw()
	if place.mode == "2.5d":
		var focus: Vector2 = _own.scene_pos if _own != null and is_instance_valid(_own) else place.to_scene(cam_rig.global_position)
		place.update(focus, delta)
	elif place.mode == "3d":
		var own_view = _views.get(_own) if _own != null else null
		if own_view != null and is_instance_valid(own_view) and cam_rig.cam != null:
			place.update(Vector2.ZERO, delta, cam_rig.cam.global_position, own_view.global_position + Vector3(0, cam_rig.target_height, 0))
		var off := _view_dir_offset()
		if off != _last_dir_offset:
			_last_dir_offset = off
			for node in _views.keys():
				if is_instance_valid(node) and node.get("view_dir_offset") != null:
					node.view_dir_offset = off
	for tn in _trails.keys():
		var tr = _trails[tn]
		if not is_instance_valid(tr) or not is_instance_valid(tn):
			_trails.erase(tn)
			continue
		var view = _views.get(tn)
		tr.active = view != null and view.model != null and view.model.busy and str(view.model.current).begins_with("gj")
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


# The name block and the life bar over each entity, the 2.0 client's rules (KNpcGold.gd, docs/CLIENT-2.0.md §16-17): the block
# (0x005F21B0) is the name - a monster's "%s/Lv:%d" over its "%d/%d" - at size 14 with a black outline for the hovered /
# targeted npc, 12 with the show switch, else hidden; players' names coloured by camp (0x005F2507), npcs by gold kind
# (0x005F23E5); the bar (PaintLife 0x005EACF0) 38 x 3, pct x 38 / 100 coloured by KNpcGold.life_bar_color, the rest grey.
# Drawn at the unprojected point `bar_height` over the feet (the pate) instead of the 2.0 client's screen pixel pate.
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
		var etype := int(node.get("entity_type"))
		var focus: bool = NpcViewScript._flag(node, "hovered") or NpcViewScript._flag(node, "is_target")
		var life := int(node.get("life")) if node.get("life") != null else 0
		var life_max := int(node.get("life_max")) if node.get("life_max") != null else 0
		var dead: bool = bool(node.call("is_dead")) if node.has_method("is_dead") else false
		# the bar sits at the pate, the block above it (KNpc.gd: label at pate + 20, the life line 16 higher)
		if life_max > 0 and not dead and etype != ENTITY_DROP and KNpcGold.life_bar(etype, NpcScript.life_switch, focus):
			var pct := int(round(float(life) * 100.0 / float(life_max)))
			var w := float(pct * 38 / 100)
			var bar_at := Vector2(sp.x - 19.0, sp.y - 2.0)
			_names.draw_rect(Rect2(bar_at, Vector2(w, 3.0)), KNpcGold.life_bar_color(pct))
			_names.draw_rect(Rect2(bar_at + Vector2(w, 0.0), Vector2(38.0 - w, 3.0)), Color(0.5, 0.5, 0.5))
		var block := 14
		var text: String = str(node.display_name)
		var color := Color.WHITE
		if etype == ENTITY_DROP:
			color = Color(1.0, 0.85, 0.3)
		else:
			block = KNpcGold.name_block(etype, NpcScript.name_switch, focus)
			if block == 0:
				continue
			text = KNpcGold.name_text(text, etype, int(node.get("level")) if node.get("level") != null else 0)
			color = node.name_color() if node.has_method("name_color") else Color.WHITE
		var outline := 4 if block == 14 else 2
		var size := font.get_string_size(text, HORIZONTAL_ALIGNMENT_CENTER, -1, block)
		var at := Vector2(sp.x - size.x * 0.5, sp.y - 8.0)
		_names.draw_string_outline(font, at, text, HORIZONTAL_ALIGNMENT_LEFT, -1, block, outline, Color(0, 0, 0, 0.9))
		_names.draw_string(font, at, text, HORIZONTAL_ALIGNMENT_LEFT, -1, block, color)
		if etype == NpcScript.ENTITY_MONSTER:
			var lt := "%d/%d" % [life, life_max]
			var ls := font.get_string_size(lt, HORIZONTAL_ALIGNMENT_CENTER, -1, block)
			var lat := Vector2(sp.x - ls.x * 0.5, at.y - float(block) - 2.0)
			_names.draw_string_outline(font, lat, lt, HORIZONTAL_ALIGNMENT_LEFT, -1, block, outline, Color(0, 0, 0, 0.9))
			_names.draw_string(font, lat, lt, HORIZONTAL_ALIGNMENT_LEFT, -1, block, Color.WHITE)


func camera_state() -> String:
	return cam_rig.state_text()
