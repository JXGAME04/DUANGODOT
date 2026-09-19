# Scn3DNpc - mot nhan vat/NPC 3D: nap glTF (skin + animation) do tools/scn3d/export_npc.py xuat, phat animation
# theo nhom (anim_group: xx nghi, zp di, gjxx nghi chien dau, attacks danh, magic noi cong, ss bi danh, sw chet),
# nhan ten tren dau (cao do sys_bar), vu khi treo vao diem treo (HangItemMgr -> hangs).
# Model Unity nhin +Z nen con Model quay 180 do de thanh -Z cua Godot; node nay quay theo goc dung
# (rotation.y = 180 + goc trong mark JSON, giong Scn3DPlayer).
extends Node3D

static var _gltf := preload("res://scenes3d/Scn3DGltfCache.gd").new()   # duong dan -> [GLTFDocument, GLTFState]

var model: Node3D
var anim: AnimationPlayer
var label: Label3D   # khong dung nua (nhan 2D o Scn3D)
var bar_height := 2.1
var bar_y_m := 1.95      # sys_bar of the model in metres (the head bar hang point [TK HangItemMgr], root-relative)
var state_height := 3.0  # sys_state: the state icons / buff effects over the head [TK HangItemMgr, 121/124 bone prefabs]
var cha := 0
var display_name := ""
var current := ""
var info := {}          # muc trong npc_models.json
var anims := {}         # key -> ten clip (nhom mac dinh cua cha_pic)
var groups := {}        # id nhom -> {xx, zp, gjxx, gjpb, xdz, ss, sw, attacks, magic}
var group := ""         # nhom dang dung ("" = anims mac dinh)
var hangs := {}         # ten diem treo -> {node_godot, pos, quat, scale}
var weapon_nodes: Array = []
var busy := false       # dang phat animation mot lan (danh, bi danh, chet)
var battle_until := 0.0 # sau khi danh: dung tu the chien dau toi luc nay (msec)
var hold_last := false  # animation mot lan dang phat giu khung cuoi khi het (chet)
var held := false       # dang giu khung cuoi (xac): khong tu ve idle
var costume_file := ""  # bo da ao dang mac (costumes.json file; "" = da mac dinh cua model)
var _costume_nodes: Array = []

signal action_finished(clip: String)


func setup(dir: String, file: String, scale: float, name_text: String, size_y: float, model_info: Dictionary = {}) -> bool:
	var full := dir + "/" + file
	# one parse per file, a scene with the file's own node / bone names for every instance (Scn3DGltfCache)
	model = _gltf.instantiate(full)
	if model == null:
		return false
	model.name = "Model"
	model.rotation.y = PI
	model.scale = Vector3.ONE * scale
	add_child(model)
	anim = _find_anim(model)
	if anim:
		anim.animation_finished.connect(_on_anim_finished)
	info = model_info
	anims = info.get("anims", {})
	groups = info.get("groups", {})
	hangs = info.get("hangs", {})
	display_name = name_text
	var bar: float = float(info.get("bar_y", 0.0)) if info.has("bar_y") else 0.0
	if bar <= 0.0:
		bar = size_y if size_y > 0.0 else 2.1
	bar_height = bar * scale + 0.15   # nhan ten 2D do Scn3D ve (Label3D mo khi xa)
	bar_y_m = bar * scale
	var st: float = float(info.get("state_y", 0.0)) if info.has("state_y") else 0.0
	state_height = st * scale if st > 0.0 else bar_y_m + 1.0   # [TK]: sys_state = sys_bar + 0.8..1.0 m on every bone prefab
	play("xx")
	return true


func _find_anim(n: Node) -> AnimationPlayer:
	if n is AnimationPlayer:
		return n
	for c in n.get_children():
		var r := _find_anim(c)
		if r:
			return r
	return null


# ---------- animation ----------
func clip_for(key: String) -> String:
	# key: xx, zp, gjxx, gjpb, magic, hoac ten clip truc tiep
	if group != "" and groups.has(group):
		var g: Dictionary = groups[group]
		if g.has(key) and g[key] is String and g[key] != "":
			return g[key]
	if anims.has(key):
		return str(anims[key])
	return key


func _pick(list) -> String:
	# [[ten, xac suat]...] -> ten theo xac suat
	if list == null or not (list is Array) or list.is_empty():
		return ""
	var total := 0.0
	for e in list:
		total += float(e[1])
	var r := randf() * total
	for e in list:
		r -= float(e[1])
		if r <= 0.0:
			return str(e[0])
	return str(list[0][0])


func set_group(gid: String) -> void:
	group = gid if groups.has(gid) else ""
	busy = false
	current = ""
	play("xx")


func has_clip(key: String) -> bool:
	return anim != null and anim.has_animation(clip_for(key))


# key hoac ten clip; loop = lap; force = cat animation mot lan dang phat
func play(key: String, loop := true, force := false) -> bool:
	if anim == null:
		return false
	if busy and not force:
		return false
	if held and not force:
		return false
	held = false
	hold_last = false
	anim.speed_scale = 1.0
	var cname := clip_for(key)
	if cname == current and loop:
		return true
	if not anim.has_animation(cname):
		return false
	var a := anim.get_animation(cname)
	a.loop_mode = Animation.LOOP_LINEAR if loop else Animation.LOOP_NONE
	anim.play(cname, 0.12)
	current = cname
	busy = not loop
	return true


# tu the dung: chien dau (gjxx) neu vua danh, khong thi nghi (xx)
func idle() -> void:
	if busy:
		return
	if Time.get_ticks_msec() < battle_until and has_clip("gjxx"):
		play("gjxx")
	else:
		play("xx")


func walk() -> void:
	if busy:
		return
	if Time.get_ticks_msec() < battle_until and has_clip("gjpb"):
		play("gjpb")
	else:
		play("zp")


# chay: clip chay chien dau (gjpb) neu co, khong thi clip di phat nhanh hon
func run() -> void:
	if busy:
		return
	if has_clip("gjpb"):
		play("gjpb")
		anim.speed_scale = 1.0
	else:
		play("zp")
		anim.speed_scale = 1.35


# Mot clip (key hoac ten) phat dung `seconds` giay (speed_scale = do dai clip / seconds) - so khung cua zone quyet dinh
# don danh/bi danh/chet dai bao nhieu (npcs.txt, 18 Hz); hold = giu khung cuoi (xac). Tra ve ten clip hoac "".
func play_once(key: String, seconds: float, hold := false) -> String:
	if anim == null:
		return ""
	var cname := clip_for(key)
	if not anim.has_animation(cname):
		return ""
	var a := anim.get_animation(cname)
	a.loop_mode = Animation.LOOP_NONE
	anim.speed_scale = (a.length / seconds) if seconds > 0.0 else 1.0
	anim.play(cname, 0.08)
	current = cname
	busy = true
	held = false
	hold_last = hold
	return cname


# giu khung cuoi ngay (xac cua nguoi vao sau khi no da chet)
func seek_end() -> void:
	if anim == null or current == "":
		return
	anim.seek(anim.get_animation(current).length, true)
	anim.pause()
	busy = false
	held = true


# don danh thuong theo nhom vu khi (chon ngau nhien theo xac suat), tra ve ten clip
func attack() -> String:
	var cname := ""
	if group != "" and groups.has(group):
		cname = _pick(groups[group].get("attacks", []))
	if cname == "":
		cname = _pick(info.get("groups", {}).get("1", {}).get("attacks", []))
	if cname == "" or anim == null or not anim.has_animation(cname):
		return ""
	battle_until = Time.get_ticks_msec() + 6000
	play(cname, false, true)
	return cname


# ten clip don danh thuong theo nhom vu khi (ngau nhien theo xac suat), "" khi khong co - khong phat
func attack_clip() -> String:
	var cname := ""
	if group != "" and groups.has(group):
		cname = _pick(groups[group].get("attacks", []))
	if cname == "":
		cname = _pick(info.get("groups", {}).get("1", {}).get("attacks", []))
	if cname == "" or anim == null or not anim.has_animation(cname):
		return ""
	battle_until = Time.get_ticks_msec() + 6000
	return cname


# ten clip cua mot loai (ss bi danh, sw chet, xdz dong tac nho): nhom dang dung, roi bang mac dinh; "" khi khong co
func pick_clip(kind: String) -> String:
	var cname := ""
	if group != "" and groups.has(group):
		var g: Dictionary = groups[group]
		cname = str(g.get("magic", "")) if kind == "magic" else _pick(g.get(kind, []))
	if cname == "" and anims.has(kind):
		cname = str(anims[kind])
	if cname == "" or anim == null or not anim.has_animation(cname):
		return ""
	return cname


# noi cong / ky nang (sf*), bi danh (ss), chet (sw), dong tac nho (xdz)
func act(kind: String) -> String:
	var cname := ""
	if group != "" and groups.has(group):
		var g: Dictionary = groups[group]
		if kind == "magic":
			cname = str(g.get("magic", ""))
		else:
			cname = _pick(g.get(kind, []))
	if cname == "" and anims.has(kind):
		cname = str(anims[kind])
	if cname == "" or anim == null or not anim.has_animation(cname):
		return ""
	if kind == "magic":
		battle_until = Time.get_ticks_msec() + 6000
	play(cname, false, true)
	return cname


func _on_anim_finished(_name: StringName) -> void:
	busy = false
	anim.speed_scale = 1.0
	if hold_last:
		held = true
		hold_last = false
		anim.seek(anim.get_animation(current).length, true)
		anim.pause()
		action_finished.emit(str(_name))
		return
	current = ""
	action_finished.emit(str(_name))
	idle()


func animation_names() -> PackedStringArray:
	return anim.get_animation_list() if anim else PackedStringArray()


# ---------- bo da (ao) ----------
# The skeleton the skinned parts hang on (the glTF import puts them under it)
func skeleton() -> Skeleton3D:
	if model == null:
		return null
	var l := model.find_children("*", "Skeleton3D", true, false)
	return l[0] as Skeleton3D if not l.is_empty() else null


# The costume of the armour worn: AssetPool_Skin.Post(modelId) of the reference [TK] swaps the body / head / shoes skins
# of the base skeleton for the ones of a model_list row; here the row's parts come as their own glTF (export_npc.py
# --costumes, no animation) and are re-parented under our Skeleton3D - the Skin binds by bone name, the rig is the same
# bone prefab - while the base parts (mesh_*) are hidden.  "" puts the base parts back.
func set_costume(dir: String, file: String) -> bool:
	if file == costume_file:
		return true
	costume_file = file
	for n in _costume_nodes:
		if is_instance_valid(n):
			n.queue_free()
	_costume_nodes.clear()
	var skel := skeleton()
	if skel == null:
		return false
	for c in skel.get_children():
		if c is MeshInstance3D and str(c.name).begins_with("mesh_"):
			c.visible = file == ""
	if file == "":
		return true
	var inst: Node3D = _gltf.instantiate(dir + "/" + file)
	if inst == null:
		for c in skel.get_children():
			if c is MeshInstance3D and str(c.name).begins_with("mesh_"):
				c.visible = true
		costume_file = ""
		return false
	var moved := 0
	for src in inst.find_children("*", "Skeleton3D", true, false):
		for c in src.get_children():
			if c is MeshInstance3D:
				# the glTF import binds the skin by bone index of its own skeleton; the two files list the bones in
				# different orders (only the joints a skin uses become bones), so bind by name on ours
				if c.skin != null:
					var sk: Skin = c.skin.duplicate()
					for i in sk.get_bind_count():
						var bname: StringName = sk.get_bind_name(i)
						if bname == StringName(""):
							bname = StringName(src.get_bone_name(sk.get_bind_bone(i)))
						sk.set_bind_name(i, bname)
						sk.set_bind_bone(i, skel.find_bone(bname))
					c.skin = sk
				src.remove_child(c)
				c.name = "costume_" + str(c.name)
				skel.add_child(c)
				c.skeleton = NodePath("..")
				c.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_ON
				_costume_nodes.append(c)
				moved += 1
	inst.queue_free()
	if moved == 0:
		for c in skel.get_children():
			if c is MeshInstance3D and str(c.name).begins_with("mesh_"):
				c.visible = true
		costume_file = ""
		return false
	return true


# ---------- vu khi ----------
func clear_weapons() -> void:
	for w in weapon_nodes:
		if is_instance_valid(w):
			w.queue_free()
	weapon_nodes.clear()


# The hinge node of a hang point (HangItemMgr row: path = the node or bone it sits under, pos/rot/scale = its own
# offset): the prefab's own hinge object when the bundle had one (hang@wq_r), else a BoneAttachment3D on the bone
# (Bip001 Spine1 of a horse, Bip001 L Hand of the dual weapons - Godot keeps joints inside the Skeleton3D) plus a
# child carrying the offset. RideUnit.CreateRide [TK 0x5bd930] parents the rider to GetItemTrans(hinge) with
# localPosition zero and localRotation identity, so whatever hangs here goes in with an identity transform.
# The height over the feet of a hang point the reference makes on the root rather than on a bone [TK HangItemMgr: sys_foot
# = the root, sys_bar / sys_bar_sit (sitting) / sys_state at the model's own heights]; -1 for a bone hinge (hang_node) or
# an unknown name
func hang_height(hname: String) -> float:
	match hname:
		"sys_foot", "":
			return 0.0
		"sys_bar", "sys_bar_sit":
			return bar_y_m
		"sys_state", "sys_state@buf_head", "sys_state@buf":
			return state_height
	return -1.0


func hang_node(hname: String) -> Node3D:
	var h: Dictionary = hangs.get(hname, {})
	if h.is_empty() or model == null:
		return null
	var existing := model.find_child("hinge_" + hname, true, false)
	if existing is Node3D:
		return existing
	var target: Node = model.find_child(str(h.get("node_godot", "")), true, false)
	if target == null:
		target = model.find_child(str(h.get("node", "")), true, false)
	if target == null:
		for sk in model.find_children("*", "Skeleton3D", true, false):
			var bi: int = sk.find_bone(str(h.get("node", "")))
			if bi < 0:
				bi = sk.find_bone(str(h.get("node_godot", "")))
			if bi >= 0:
				var ba := BoneAttachment3D.new()
				ba.name = "bone_" + str(h.get("node_godot", ""))
				sk.add_child(ba)
				ba.bone_idx = bi
				target = ba
				break
	if not (target is Node3D):
		Log.warn("map3d", "hang point bone missing", {"hang": hname, "node": str(h.get("node", "")), "model": str(info.get("name", ""))})
		return null
	var hinge := Node3D.new()
	hinge.name = "hinge_" + hname
	(target as Node3D).add_child(hinge)
	hinge.position = Vector3(h["pos"][0], h["pos"][1], h["pos"][2])
	hinge.quaternion = Quaternion(h["quat"][0], h["quat"][1], h["quat"][2], h["quat"][3])
	hinge.scale = Vector3(h["scale"][0], h["scale"][1], h["scale"][2])
	return hinge


# winfo: muc trong weapons.json (file, hangs = ten diem treo). Tra ve so mau treo duoc.
func attach_weapon(weapon_dir: String, winfo: Dictionary) -> int:
	clear_weapons()
	if winfo.is_empty() or model == null:
		return 0
	var full := weapon_dir + "/" + str(winfo.get("file", ""))
	var n := 0
	for hname in winfo.get("hangs", []):
		var hinge := hang_node(str(hname))
		if hinge == null:
			continue
		var w: Node3D = _gltf.instantiate(full)
		if w == null:
			continue
		w.name = "weapon_%s" % hname
		hinge.add_child(w)
		weapon_nodes.append(w)
		n += 1
	return n
