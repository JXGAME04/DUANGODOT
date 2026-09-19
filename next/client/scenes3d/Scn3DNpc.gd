# Scn3DNpc - mot nhan vat/NPC 3D: nap glTF (skin + animation) do tools/scn3d/export_npc.py xuat, phat animation
# theo nhom (anim_group: xx nghi, zp di, gjxx nghi chien dau, attacks danh, magic noi cong, ss bi danh, sw chet),
# nhan ten tren dau (cao do sys_bar), vu khi treo vao diem treo (HangItemMgr -> hangs).
# Model Unity nhin +Z nen con Model quay 180 do de thanh -Z cua Godot; node nay quay theo goc dung
# (rotation.y = 180 + goc trong mark JSON, giong Scn3DPlayer).
extends Node3D

static var _cache := {}   # duong dan -> [GLTFDocument, GLTFState]

var model: Node3D
var anim: AnimationPlayer
var label: Label3D   # khong dung nua (nhan 2D o Scn3D)
var bar_height := 2.1
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

signal action_finished(clip: String)


func setup(dir: String, file: String, scale: float, name_text: String, size_y: float, model_info: Dictionary = {}) -> bool:
	var full := dir + "/" + file
	var pair = _cache.get(full)
	if pair == null:
		var doc := GLTFDocument.new()
		var st := GLTFState.new()
		var err := doc.append_from_file(full, st)
		if err != OK:
			push_error("Scn3DNpc: nap %s loi %d" % [full, err])
			return false
		pair = [doc, st]
		_cache[full] = pair
	# remove_immutable_tracks = false: Godot bo track xoay hang so (vd node Bip001 cua thu) roi AnimationMixer dua xoay ve 0 -> thu nam
	model = pair[0].generate_scene(pair[1], 30.0, false, false)
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
	current = ""
	action_finished.emit(str(_name))
	idle()


func animation_names() -> PackedStringArray:
	return anim.get_animation_list() if anim else PackedStringArray()


# ---------- vu khi ----------
func clear_weapons() -> void:
	for w in weapon_nodes:
		if is_instance_valid(w):
			w.queue_free()
	weapon_nodes.clear()


# winfo: muc trong weapons.json (file, hangs = ten diem treo). Tra ve so mau treo duoc.
func attach_weapon(weapon_dir: String, winfo: Dictionary) -> int:
	clear_weapons()
	if winfo.is_empty() or model == null:
		return 0
	var full := weapon_dir + "/" + str(winfo.get("file", ""))
	var pair = _cache.get(full)
	if pair == null:
		var doc := GLTFDocument.new()
		var st := GLTFState.new()
		if doc.append_from_file(full, st) != OK:
			push_error("Scn3DNpc: nap vu khi %s loi" % full)
			return 0
		pair = [doc, st]
		_cache[full] = pair
	var n := 0
	for hname in winfo.get("hangs", []):
		var h: Dictionary = hangs.get(str(hname), {})
		if h.is_empty():
			continue
		var target: Node = model.find_child(str(h.get("node_godot", "")), true, false)
		if target == null:
			target = model.find_child(str(h.get("node", "")), true, false)
		if target == null or not (target is Node3D):
			continue
		var w: Node3D = pair[0].generate_scene(pair[1])
		if w == null:
			continue
		w.name = "weapon_%s" % hname
		w.position = Vector3(h["pos"][0], h["pos"][1], h["pos"][2])
		w.quaternion = Quaternion(h["quat"][0], h["quat"][1], h["quat"][2], h["quat"][3])
		w.scale = Vector3(h["scale"][0], h["scale"][1], h["scale"][2])
		(target as Node3D).add_child(w)
		weapon_nodes.append(w)
		n += 1
	return n
