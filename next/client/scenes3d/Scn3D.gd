# Scn3D - thu nghiem map 3D (nhanh exp/3d-baling): nap glTF do tools/scn3d/export_scene.py xuat vao
# assets3d/<map>/ (ngoai git), gan lai lightmap/dia hinh bang shader, dat camera quy dao + nhan vat tam.
#
#   godot --path client scenes3d/Scn3D.tscn -- --map=world_baling          (mo cua so, dieu khien tay)
#   godot --path client scenes3d/Scn3D.tscn -- --map=world_baling --auto   (chup 5 goc vao user://logs/ roi thoat)
#
# Phim: chuot phai keo = xoay camera, con lan/chuot giua = zoom, Q/E xoay 15 do, PgUp/PgDn nghieng,
#       chuot trai = di toi, WASD = di theo camera, L = bat/tat lightmap, [ ] = do sang lightmap,
#       F12 = chup man hinh, ESC = thoat.
extends Node3D

const ASSETS3D := "res://assets3d"
const SH_LM := preload("res://scenes3d/scn3d_lm.gdshader")
const SH_LM2 := preload("res://scenes3d/scn3d_lm_2side.gdshader")
const SH_TER := preload("res://scenes3d/scn3d_terrain.gdshader")
const CAM_SCRIPT := preload("res://scenes3d/Scn3DCamera.gd")
const PLAYER_SCRIPT := preload("res://scenes3d/Scn3DPlayer.gd")
const NPC_SCRIPT := preload("res://scenes3d/Scn3DNpc.gd")
const SFX_SCRIPT := preload("res://scenes3d/Scn3DSfx.gd")
const TRAIL_SCRIPT := preload("res://scenes3d/Scn3DTrail.gd")

var map_name := "world_baling"
var auto := false
var dir := ""
var info := {}
var tex_cache := {}
var lightmaps: Array = []
var shader_mats: Array = []
var use_lm := true
var lm_gain := 1.0
var stats := {"nodes": 0, "surfaces": 0, "lm_surfaces": 0, "terrain": 0, "collision": 0, "load_ms": 0, "npcs": 0, "npc_models": 0}
var npcs: Array = []
var npc_models := {}
var weapons := {}          # weapons.json: id -> {name, name_vi, type, type_vi, file, hangs, animgrp}
var weapon_by_type := {}   # type -> [id...]
var cur_weapon := ""      # id dang cam ("" = tay khong)
var trail: MeshInstance3D   # vet dao
var sfx_dir := ""
# ky nang thu: phim -> [ten hieu ung thi trien (tai nhan vat), ten hieu ung chinh (tai muc tieu), khoang cach muc tieu m, bay toi (s)]
const SKILL_KEYS := {KEY_Z: ["Skill_shifa_huoxi", "Skill_wd_nuleizhi", 3.0, 0.0], KEY_X: ["Skill_shifa_huoxi", "Skill_wd_wuwowj", 0.0, 0.0], KEY_C: ["Skill_shifa_huoxi", "Skill_wd_jianfei", 1.0, 8.0]}
var last_action := ""
const WEAPON_KEYS := {KEY_1: 1, KEY_2: 2, KEY_3: 3, KEY_4: 4, KEY_5: 5, KEY_6: 6, KEY_7: 7}
const TYPE_VI := {0: "Tay không", 1: "Kiếm", 2: "Đao", 3: "Thương", 4: "Côn", 5: "Song đao", 6: "Song chùy", 7: "Quyền"}
var hud: Label
var name_layer: Control
var name_labels := {}      # npc -> Label 2D
var _lod_timer := 0.0
var spawn_mark := ""     # --at=<diem danh dau>: dung tai diem do (so anh voi game goc)
var test_sfx := ""       # --sfx=<ten tep hieu ung> de chup rieng mot hieu ung
var nav_region: NavigationRegion3D
var nav_map: RID
var show_nav := false
const NAME_DIST := 60.0
const ANIM_DIST := 45.0
var cam_rig: Node3D   # Scn3DCamera (preload, khong phu thuoc cache class_name)
var player: Node3D    # Scn3DPlayer
var _shot := 0


func _ready() -> void:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("--map="):
			map_name = a.substr(6)
		elif a == "--auto":
			auto = true
		elif a.begins_with("--lm_gain="):
			lm_gain = float(a.substr(10))
		elif a.begins_with("--at="):
			spawn_mark = a.substr(5)
		elif a.begins_with("--sfx="):
			test_sfx = a.substr(6)   # --auto: this effect at the character, a picture every 10 frames (checking one export)
	dir = ProjectSettings.globalize_path(ASSETS3D) + "/" + map_name
	sfx_dir = ProjectSettings.globalize_path(ASSETS3D) + "/sfx"
	var t0 := Time.get_ticks_msec()
	var txt := FileAccess.get_file_as_string(dir + "/scene.json")
	if txt == "":
		push_error("Scn3D: khong doc duoc %s/scene.json (chay tools/scn3d/export_scene.py %s truoc)" % [dir, map_name])
		if auto:
			get_tree().quit(2)
		return
	info = JSON.parse_string(txt)
	_setup_environment()
	var root := _load_gltf()
	if root == null:
		if auto:
			get_tree().quit(3)
		return
	root.name = "Map"
	add_child(root)
	_post_process(root)
	_setup_navmesh()
	_setup_player_camera()
	_setup_npcs()
	_setup_hud()
	stats["load_ms"] = Time.get_ticks_msec() - t0
	print("SCN3D map=%s nodes=%d surfaces=%d lightmapped=%d terrain=%d npcs=%d npc_models=%d load_ms=%d" % [
		map_name, stats["nodes"], stats["surfaces"], stats["lm_surfaces"], stats["terrain"], stats["npcs"], stats["npc_models"], stats["load_ms"]])
	if auto:
		_auto()


func _load_gltf() -> Node:
	var doc := GLTFDocument.new()
	var st := GLTFState.new()
	var err := doc.append_from_file(dir + "/" + map_name + ".gltf", st)
	if err != OK:
		push_error("Scn3D: nap glTF loi %d" % err)
		return null
	return doc.generate_scene(st)


# ---------- moi truong ----------
func _setup_environment() -> void:
	var r: Dictionary = info.get("render", {})
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	var fogc := _col(r.get("fog_color", [0.7, 0.8, 0.9]))
	env.background_color = fogc
	env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	env.ambient_light_color = _col(r.get("ambient_light", [0.5, 0.5, 0.55]))
	env.ambient_light_energy = 1.0
	if r.get("fog", false):
		env.fog_enabled = true
		env.fog_light_color = fogc
		env.fog_sun_scatter = 0.0
		if int(r.get("fog_mode", 1)) == 1:
			env.fog_mode = Environment.FOG_MODE_DEPTH
			env.fog_depth_begin = float(r.get("fog_start", 20.0))
			env.fog_depth_end = float(r.get("fog_end", 120.0))
			env.fog_depth_curve = 1.0
		else:
			env.fog_density = float(r.get("fog_density", 0.01))
	var we := WorldEnvironment.new()
	we.name = "Env"
	we.environment = env
	add_child(we)
	var l: Dictionary = r.get("light", {})
	var light := DirectionalLight3D.new()
	light.name = "Sun"
	var d := Vector3(-0.3, -0.85, 0.4)
	if l.has("dir"):
		d = Vector3(l["dir"][0], l["dir"][1], l["dir"][2])
	light.position = Vector3(0, 60, 0)
	light.look_at_from_position(light.position, light.position + d, Vector3.UP)
	light.light_color = _col(l.get("color", [1, 1, 0.95]))
	light.light_energy = 1.35
	light.shadow_enabled = true
	light.directional_shadow_max_distance = 90.0
	add_child(light)


func _col(a) -> Color:
	if a is Array and a.size() >= 3:
		return Color(clampf(a[0], 0, 1), clampf(a[1], 0, 1), clampf(a[2], 0, 1))
	return Color.WHITE


# ---------- vat lieu ----------
func _tex(file: String) -> Texture2D:
	if file == "":
		return null
	if tex_cache.has(file):
		return tex_cache[file]
	var img := Image.load_from_file(dir + "/" + file)
	var t: Texture2D = null
	if img != null:
		img.generate_mipmaps()
		t = ImageTexture.create_from_image(img)
	tex_cache[file] = t
	return t


func _lightmap(i: int) -> Texture2D:
	var lms: Array = info.get("lightmaps", [])
	if i < 0 or i >= lms.size() or lms[i] == null:
		return null
	return _tex(str(lms[i]))


func _st(v) -> Vector4:
	if v is Array and v.size() >= 4:
		return Vector4(v[0], v[1], v[2], v[3])
	return Vector4(1, 1, 0, 0)


func _post_process(root: Node) -> void:
	var nodes: Array = info.get("nodes", [])
	var mats: Array = info.get("materials", [])
	var stack: Array = [root]
	while not stack.is_empty():
		var n: Node = stack.pop_back()
		for c in n.get_children():
			stack.push_back(c)
		if not (n is MeshInstance3D):
			continue
		var mi := n as MeshInstance3D
		var idx := -1
		if mi.name.begins_with("g") and mi.name.substr(1).is_valid_int():
			idx = int(mi.name.substr(1))
		var meta: Dictionary = nodes[idx] if idx >= 0 and idx < nodes.size() else {}
		stats["nodes"] += 1
		var mesh := mi.mesh
		if mesh == null:
			continue
		var is_terrain := false
		for s in mesh.get_surface_count():
			stats["surfaces"] += 1
			var orig := mesh.surface_get_material(s)
			var mname := orig.resource_name if orig else ""
			var mi_idx := -1
			if mname.begins_with("m") and mname.substr(1).is_valid_int():
				mi_idx = int(mname.substr(1))
			var mm: Dictionary = mats[mi_idx] if mi_idx >= 0 and mi_idx < mats.size() else {}
			var sm := _make_material(mm, meta)
			if sm != null:
				mi.set_surface_override_material(s, sm)
			if mm.get("kind", "") == "terrain":
				is_terrain = true
		if is_terrain or meta.get("group", "") == "terrain":
			stats["terrain"] += 1
			mi.create_trimesh_collision()
			stats["collision"] += 1
			mi.add_to_group("terrain")


func _make_material(mm: Dictionary, meta: Dictionary) -> Material:
	if mm.is_empty():
		return null
	var texs: Dictionary = mm.get("tex", {})
	var lm = meta.get("lm", null)
	var kind: String = mm.get("kind", "static")
	if kind == "terrain" and texs.has("_Control"):
		var sm := ShaderMaterial.new()
		sm.shader = SH_TER
		sm.set_shader_parameter("control_tex", _tex(texs["_Control"]["file"]))
		var layers := 0
		for i in 4:
			var k := "_Splat%d" % i
			if texs.has(k):
				sm.set_shader_parameter("splat%d" % i, _tex(texs[k]["file"]))
				sm.set_shader_parameter("st%d" % i, _st(texs[k].get("st")))
				layers = i + 1
		sm.set_shader_parameter("layers", layers)
		if lm != null:
			sm.set_shader_parameter("lightmap_tex", _lightmap(int(lm[0])))
			sm.set_shader_parameter("lm_st", Vector4(lm[1], lm[2], lm[3], lm[4]))
			sm.set_shader_parameter("use_lm", use_lm)
			stats["lm_surfaces"] += 1
		else:
			sm.set_shader_parameter("use_lm", false)
		shader_mats.append(sm)
		return sm
	var base: String = str(mm.get("base", ""))
	if lm != null and base != "" and texs.has(base) and kind == "static":
		var sm2 := ShaderMaterial.new()
		sm2.shader = SH_LM2 if mm.get("double", false) else SH_LM
		sm2.set_shader_parameter("albedo_tex", _tex(texs[base]["file"]))
		sm2.set_shader_parameter("lightmap_tex", _lightmap(int(lm[0])))
		sm2.set_shader_parameter("lm_st", Vector4(lm[1], lm[2], lm[3], lm[4]))
		sm2.set_shader_parameter("alpha_test", mm.get("alpha", "OPAQUE") == "MASK")
		sm2.set_shader_parameter("cutoff", float(mm.get("cutoff", 0.5)))
		sm2.set_shader_parameter("use_lm", use_lm)
		var c = mm.get("color", null)
		if c is Array and c.size() >= 4 and kind == "static":
			sm2.set_shader_parameter("tint", Color(c[0], c[1], c[2], c[3]))
		shader_mats.append(sm2)
		stats["lm_surfaces"] += 1
		return sm2
	return null


func _set_lm(on: bool) -> void:
	use_lm = on
	for sm in shader_mats:
		sm.set_shader_parameter("use_lm", on)
		sm.set_shader_parameter("lm_gain", lm_gain)


# ---------- navmesh (AIS cua game goc: vung di duoc; ngoai vung = tuong, nha, nuoc) ----------
func _setup_navmesh() -> void:
	var nav: Dictionary = info.get("marks", {}).get("nav", {})
	var verts: Array = nav.get("verts", [])
	var tris: Array = nav.get("tris", [])
	if verts.is_empty() or tris.is_empty():
		print("SCN3D nav: khong co navmesh trong scene.json")
		return
	var nm := NavigationMesh.new()
	var pv := PackedVector3Array()
	pv.resize(verts.size())
	for i in verts.size():
		var v: Array = verts[i]
		pv[i] = Vector3(v[0], v[1], v[2])
	nm.vertices = pv
	for t in tris:
		# da dao truc X luc xuat -> dao chieu de da giac nhin tu tren xuong nguoc chieu kim dong ho
		nm.add_polygon(PackedInt32Array([int(t[0]), int(t[2]), int(t[1])]))
	nav_region = NavigationRegion3D.new()
	nav_region.name = "Nav"
	nav_region.navigation_mesh = nm
	add_child(nav_region)
	nav_map = get_world_3d().navigation_map
	NavigationServer3D.map_set_cell_size(nav_map, 0.1)
	NavigationServer3D.map_set_edge_connection_margin(nav_map, 0.5)
	stats["nav_tris"] = tris.size()
	print("SCN3D nav: %d dinh, %d tam giac" % [verts.size(), tris.size()])


# diem gan nhat tren navmesh (mat ngang); tra ve pos neu chua co navmesh
func nav_clamp(pos: Vector3) -> Vector3:
	if nav_region == null:
		return pos
	var c := NavigationServer3D.map_get_closest_point(nav_map, pos)
	return c


func nav_path(from: Vector3, to: Vector3) -> PackedVector3Array:
	if nav_region == null:
		return PackedVector3Array([to])
	return NavigationServer3D.map_get_path(nav_map, from, to, true)


# ---------- nhan vat + camera ----------
func _setup_player_camera() -> void:
	player = PLAYER_SCRIPT.new()
	player.name = "Player"
	add_child(player)
	var spawn := Vector3.ZERO
	var yaw := 0.0
	var marks: Dictionary = info.get("marks", {})
	var pts: Dictionary = marks.get("points", {})
	var key := ""
	if spawn_mark != "" and pts.has(spawn_mark) and pts[spawn_mark].size() > 0:
		key = spawn_mark
	for k in ["BeginPoint01", "BeginPoint", "BeginPointJ"]:
		if key != "":
			break
		if pts.has(k) and pts[k].size() > 0:
			key = k
			break
	if key == "":
		for k in pts.keys():
			if pts[k].size() > 0:
				key = k
				break
	if key != "":
		var p: Dictionary = pts[key][0]
		spawn = Vector3(p["pos"][0], p["pos"][1], p["pos"][2])
		yaw = 180.0 + float(p.get("angle", 0.0))
	elif marks.has("nav"):
		var bb: Array = marks["nav"]["bbox"]
		spawn = Vector3((bb[0] + bb[2]) * 0.5, 0, (bb[1] + bb[3]) * 0.5)
	player.global_position = spawn
	player.yaw = deg_to_rad(yaw)
	cam_rig = CAM_SCRIPT.new()
	cam_rig.name = "CameraRig"
	cam_rig.target = player
	add_child(cam_rig)
	var table: Dictionary = info.get("table", {})
	var fov := 40.0
	if info.get("render", {}).has("camera_unity"):
		fov = float(info["render"]["camera_unity"].get("fov", 40.0))
	cam_rig.setup(table.get("camera", {}), fov)
	cam_rig.yaw = yaw
	player.cam_rig = cam_rig
	player.world = self
	player.snap_to_ground()


# NPC: assets3d/<map>/npcs.json (diem dung -> cha_pic id, vi tri, goc) + assets3d/npc/npc_models.json (tep glTF, scale, ten)
func _setup_npcs() -> void:
	var npc_dir := ProjectSettings.globalize_path(ASSETS3D) + "/npc"
	var mtxt := FileAccess.get_file_as_string(npc_dir + "/npc_models.json")
	var ptxt := FileAccess.get_file_as_string(dir + "/npcs.json")
	if mtxt == "" or ptxt == "":
		print("SCN3D npc: chua co npcs.json / npc_models.json (chay tools/scn3d/export_npc.py --map %s)" % map_name)
		return
	npc_models = JSON.parse_string(mtxt)
	var pl: Dictionary = JSON.parse_string(ptxt)
	for p in pl.get("placements", []):
		var mi: Dictionary = npc_models.get(str(int(p["cha"])), {})
		if mi.is_empty():
			continue
		var npc := NPC_SCRIPT.new()
		npc.name = "npc_%s" % p["mark"]
		npc.cha = int(p["cha"])
		add_child(npc)
		var shown: String = str(mi.get("name_vi", "")) if str(mi.get("name_vi", "")) != "" else str(mi.get("name", ""))
		if not npc.setup(npc_dir, mi["file"], float(mi.get("scale", 1.0)), shown, float(mi.get("sizeY", 0.0) if mi.get("sizeY") != null else 0.0), mi):
			npc.queue_free()
			continue
		npc.global_position = Vector3(p["pos"][0], p["pos"][1], p["pos"][2])
		npc.rotation.y = deg_to_rad(180.0 + float(p.get("angle", 0.0)))
		_snap(npc)
		npcs.append(npc)
	stats["npcs"] = npcs.size()
	stats["npc_models"] = npc_models.size()
	# nhan vat chinh
	var pc: Dictionary = npc_models.get(str(int(pl.get("player", 1))), {})
	if not pc.is_empty() and player:
		var me := NPC_SCRIPT.new()
		me.name = "Me"
		if me.setup(npc_dir, pc["file"], float(pc.get("scale", 1.0)), "", 0.0, pc):
			player.set_model(me)
			_load_weapons()
			_equip_type(1)
		else:
			me.queue_free()


# ---------- vu khi ----------
func _load_weapons() -> void:
	var wdir := ProjectSettings.globalize_path(ASSETS3D) + "/weapon"
	var txt := FileAccess.get_file_as_string(wdir + "/weapons.json")
	if txt == "":
		print("SCN3D weapon: chua co weapons.json (chay tools/scn3d/export_weapon.py)")
		return
	weapons = JSON.parse_string(txt)
	for id in weapons.keys():
		var t := int(weapons[id].get("type", 0))
		if not weapon_by_type.has(t):
			weapon_by_type[t] = []
		weapon_by_type[t].append(id)
	for t in weapon_by_type.keys():
		weapon_by_type[t].sort_custom(func(a, b): return int(a) < int(b))


# type: 1 kiem 2 dao 3 thuong 4 con 5 song dao 6 song chuy 7 quyen; 0 = tay khong. idx: mau thu idx trong loai
func _equip_type(t: int, idx := 0) -> void:
	if player == null or player.model == null:
		return
	var m: Node = player.model
	if t == 0 or not weapon_by_type.has(t) or weapon_by_type[t].is_empty():
		m.clear_weapons()
		cur_weapon = ""
		m.set_group("1")
		return
	var ids: Array = weapon_by_type[t]
	var id: String = str(ids[idx % ids.size()])
	var w: Dictionary = weapons[id]
	var n: int = m.attach_weapon(ProjectSettings.globalize_path(ASSETS3D) + "/weapon", w)
	cur_weapon = id
	_setup_trail(w)
	var g := str(int(w.get("animgrp", 0)))
	if g == "0":
		g = "1"
	m.set_group(g)
	print("SCN3D weapon: %s (%s) treo %d mau, nhom animation %s" % [w.get("name", ""), w.get("type_vi", ""), n, g])


func _setup_trail(w: Dictionary) -> void:
	if trail:
		trail.queue_free()
		trail = null
	var m: Node = player.model
	if m.weapon_nodes.is_empty():
		return
	trail = TRAIL_SCRIPT.new()
	trail.name = "Trail"
	add_child(trail)
	trail.setup(m.weapon_nodes[0], w.get("anchors", {}))


# ky nang thu: animation noi cong + hieu ung thi trien tai nhan vat + hieu ung chinh tai muc tieu (truoc mat)
func _cast_skill(key: int) -> void:
	if player == null or player.model == null or not SKILL_KEYS.has(key):
		return
	var m: Node = player.model
	var def: Array = SKILL_KEYS[key]
	last_action = m.act("magic")
	if last_action == "":
		last_action = m.attack()
	var fwd := Vector3(-sin(player.yaw), 0, -cos(player.yaw))
	var origin: Vector3 = player.global_position
	if str(def[0]) != "":
		SFX_SCRIPT.spawn(self, sfx_dir, str(def[0]), origin + Vector3(0, 0.9, 0), player.yaw, 0.0, false)
	var target: Vector3 = origin + fwd * float(def[2])
	var fx: Node3D = SFX_SCRIPT.spawn(self, sfx_dir, str(def[1]), target + Vector3(0, 0.3, 0), player.yaw, 0.0, false)
	if fx and float(def[3]) > 0.0:
		var tw := create_tween()
		tw.tween_property(fx, "global_position", target + fwd * float(def[3]) + Vector3(0, 0.6, 0), 0.6)


func _cycle_weapon() -> void:
	if cur_weapon == "" or not weapons.has(cur_weapon):
		return
	var t := int(weapons[cur_weapon].get("type", 0))
	var ids: Array = weapon_by_type.get(t, [])
	var i := ids.find(cur_weapon)
	_equip_type(t, (i + 1) % max(1, ids.size()))


func _snap(n: Node3D) -> void:
	var space := get_world_3d().direct_space_state
	var q := PhysicsRayQueryParameters3D.create(n.global_position + Vector3(0, 50, 0), n.global_position + Vector3(0, -50, 0), 1)
	var hit := space.intersect_ray(q)
	if hit:
		n.global_position.y = hit.position.y


func _setup_hud() -> void:
	var layer := CanvasLayer.new()
	layer.name = "HUD"
	add_child(layer)
	name_layer = Control.new()
	name_layer.name = "Names"
	name_layer.mouse_filter = Control.MOUSE_FILTER_IGNORE
	name_layer.set_anchors_preset(Control.PRESET_FULL_RECT)
	layer.add_child(name_layer)
	for npc in npcs:
		var lb := Label.new()
		lb.text = npc.display_name
		lb.add_theme_font_size_override("font_size", 15)
		lb.add_theme_color_override("font_color", Color(1.0, 0.93, 0.55) if npc.cha >= 1000 else Color(1.0, 1.0, 1.0))
		lb.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.9))
		lb.add_theme_constant_override("outline_size", 4)
		lb.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		lb.visible = false
		name_layer.add_child(lb)
		name_labels[npc] = lb
	hud = Label.new()
	hud.position = Vector2(8, 8)
	hud.add_theme_color_override("font_color", Color.WHITE)
	hud.add_theme_color_override("font_shadow_color", Color.BLACK)
	hud.add_theme_constant_override("shadow_offset_x", 1)
	hud.add_theme_constant_override("shadow_offset_y", 1)
	layer.add_child(hud)


func _update_names() -> void:
	if cam_rig == null or cam_rig.cam == null:
		return
	var cam: Camera3D = cam_rig.cam
	var ppos: Vector3 = player.global_position
	for npc in name_labels.keys():
		var lb: Label = name_labels[npc]
		if not is_instance_valid(npc):
			lb.visible = false
			continue
		var wp: Vector3 = (npc as Node3D).global_position + Vector3(0, float(npc.bar_height), 0)
		if wp.distance_to(ppos) > NAME_DIST or cam.is_position_behind(wp):
			lb.visible = false
			continue
		var sp := cam.unproject_position(wp)
		lb.visible = true
		lb.position = sp - Vector2(lb.size.x * 0.5, lb.size.y)


# NPC xa hon ANIM_DIST: dung AnimationPlayer (494 NPC cung phat animation ton CPU)
func _update_lod() -> void:
	var ppos: Vector3 = player.global_position
	for npc in npcs:
		if not is_instance_valid(npc) or npc.anim == null:
			continue
		var near: bool = (npc as Node3D).global_position.distance_to(ppos) < ANIM_DIST
		if npc.anim.active != near:
			npc.anim.active = near


func _process(delta: float) -> void:
	if hud == null or cam_rig == null or player == null:
		return
	_update_names()
	if trail:
		trail.active = player.model != null and player.model.busy and player.model.current.begins_with("gj")
	_lod_timer -= delta
	if _lod_timer <= 0.0:
		_lod_timer = 0.5
		_update_lod()
	var p: Vector3 = player.global_position
	var t: Dictionary = info.get("table", {})
	var wname := "tay không"
	if cur_weapon != "" and weapons.has(cur_weapon):
		var w: Dictionary = weapons[cur_weapon]
		wname = "%s (%s)" % [w.get("name_vi", "") if w.get("name_vi", "") != "" else w.get("name", ""), w.get("type_vi", "")]
	var mgroup := str(player.model.group) if player.model else ""
	hud.text = "%s  %s  |  FPS %d  |  node %d  mat lightmap %d  NPC %d  nav %d tam giác |  nap %d ms
vũ khí: %s   nhóm anim %s   đòn: %s   [1-7 vũ khí, 0 tay không, Tab đổi mẫu, Space đánh, F nội công, Z/X/C kỹ năng Võ Đang thử, G bị đánh, H chết]
camera yaw %.0f  pitch %.0f  dist %.1f   lightmap %s (gain %.2f)
nhan vat (Godot) %.1f %.1f %.1f   (Unity) %.1f %.1f %.1f
chuot phai: xoay | con lan: zoom | Q/E xoay | PgUp/PgDn nghieng | trai: di | WASD | L lightmap | [ ] gain | F12 chup | ESC" % [
		map_name, (str(t.get("name_vi", "")) if str(t.get("name_vi", "")) != "" else str(t.get("name", ""))), Engine.get_frames_per_second(), stats["nodes"], stats["lm_surfaces"], stats["npcs"], stats.get("nav_tris", 0), stats["load_ms"], wname, mgroup, last_action,
		cam_rig.yaw, cam_rig.pitch, cam_rig.dist, "bat" if use_lm else "tat", lm_gain, p.x, p.y, p.z, -p.x, p.y, p.z]


func _unhandled_input(ev: InputEvent) -> void:
	if ev is InputEventKey and ev.pressed and not ev.echo:
		var k := ev as InputEventKey
		if k.keycode == KEY_ESCAPE:
			get_tree().quit()
		elif k.keycode == KEY_L:
			_set_lm(not use_lm)
		elif k.keycode == KEY_BRACKETRIGHT:
			lm_gain = minf(lm_gain + 0.1, 4.0)
			_set_lm(use_lm)
		elif k.keycode == KEY_BRACKETLEFT:
			lm_gain = maxf(lm_gain - 0.1, 0.1)
			_set_lm(use_lm)
		elif k.keycode == KEY_F12:
			_screenshot("user://logs/scn3d_%s_%d.png" % [map_name, _shot])
			_shot += 1
		elif WEAPON_KEYS.has(k.keycode):
			_equip_type(WEAPON_KEYS[k.keycode])
		elif k.keycode == KEY_0:
			_equip_type(0)
		elif k.keycode == KEY_TAB:
			_cycle_weapon()
		elif k.keycode == KEY_SPACE and player and player.model:
			last_action = player.model.attack()
		elif k.keycode == KEY_F and player and player.model:
			last_action = player.model.act("magic")
		elif k.keycode == KEY_G and player and player.model:
			last_action = player.model.act("ss")
		elif k.keycode == KEY_H and player and player.model:
			last_action = player.model.act("sw")
		elif SKILL_KEYS.has(k.keycode):
			_cast_skill(k.keycode)


func _screenshot(path: String) -> void:
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path("user://logs"))
	await RenderingServer.frame_post_draw
	var img := get_viewport().get_texture().get_image()
	img.save_png(path)
	print("SCN3D_SHOT " + ProjectSettings.globalize_path(path))


# Chup 5 goc (yaw 0/90/180/270 + nghieng cao) roi thoat - de kiem tra khong can nguoi ngoi.
func _auto() -> void:
	for i in 6:
		await get_tree().process_frame
	if test_sfx != "":
		cam_rig.yaw = 20.0
		cam_rig.pitch = 40.0
		cam_rig.dist = 8.0
		for i in 3:
			await get_tree().process_frame
		var fx: Node3D = SFX_SCRIPT.spawn(self, sfx_dir, test_sfx, player.global_position + Vector3(0, 0.9, 0), player.yaw, 0.0, false)
		var shot := 0
		for k in 5:
			for i in 8:
				await get_tree().process_frame
			await _screenshot("user://logs/scn3d_sfx_%s_%d.png" % [test_sfx, shot])
			shot += 1
			if is_instance_valid(fx) and fx.get("_pc2s") is Array:
				# --sfx: the PC2Anim point caches playing (vertex 0 of the rebuilt mesh against sample 0, the frame time)
				for pc in fx.get("_pc2s"):
					var pmi: MeshInstance3D = pc["mi"]
					var v0 := (pmi.mesh as ArrayMesh).surface_get_arrays(0)[Mesh.ARRAY_VERTEX][0] as Vector3 if is_instance_valid(pmi) and pmi.mesh is ArrayMesh else Vector3.INF
					print("SCN3D_SFX_PC2 %s frames=%d fps=%d t=%.3f play=%s v0=%s base=%s" % [pmi.name if is_instance_valid(pmi) else "-", (pc["frames"] as Array).size(), int(pc["fps"]), float(pc["t"]), str(pc["play"]), str(v0), str((pc["frames"][0] as PackedVector3Array)[0])])
		print("SCN3D_SFX %s alive=%s children=%d" % [test_sfx, is_instance_valid(fx), fx.get_child_count() if is_instance_valid(fx) else 0])
		if is_instance_valid(fx):
			# --sfx: every particle node with what it emits (the check of one export)
			for p in fx.find_children("*", "CPUParticles3D", true, false):
				var pm = p.mesh
				var mat = pm.surface_get_material(0) if pm != null and pm.get_surface_count() > 0 else null
				print("SCN3D_SFX_PS %s emitting=%s amount=%d life=%.2f vel=%.2f..%.2f dir=%s scale=%.3f..%.3f color=%s mesh=%s mat_tex=%s blend=%s bb=%s frames=%dx%d pos=%s vis=%s" % [
					p.name, p.emitting, p.amount, p.lifetime, p.initial_velocity_min, p.initial_velocity_max, str(p.direction), p.scale_amount_min, p.scale_amount_max,
					str(p.color), str(pm.get_class()) if pm else "-", str(mat.albedo_texture != null) if mat is StandardMaterial3D else "-",
					str(mat.blend_mode) if mat is StandardMaterial3D else "-", str(mat.billboard_mode) if mat is StandardMaterial3D else "-",
					mat.particles_anim_h_frames if mat is StandardMaterial3D else 0, mat.particles_anim_v_frames if mat is StandardMaterial3D else 0,
					str(p.global_position), str(p.visible)])
				print("SCN3D_SFX_PS2 %s one_shot=%s expl=%.2f shape=%d spread=%.0f anim_off=%.4f..%.4f anim_speed=%.3f ramp=%s albedo=%s transp=%s alpha_scissor=%s local=%s lifetime_rand=%.2f" % [
					p.name, p.one_shot, p.explosiveness, p.emission_shape, p.spread, p.anim_offset_min, p.anim_offset_max, p.anim_speed_max,
					str(p.color_ramp != null), str(mat.albedo_color) if mat is StandardMaterial3D else "-", str(mat.transparency) if mat is StandardMaterial3D else "-",
					str(mat.alpha_scissor_threshold) if mat is StandardMaterial3D else "-", str(p.local_coords), p.lifetime_randomness])
		get_tree().quit()
		return
	var views := [[0.0, 40.0, 19.0], [90.0, 40.0, 19.0], [180.0, 40.0, 19.0], [270.0, 40.0, 19.0], [45.0, 75.0, 21.0], [20.0, 40.0, 10.0]]
	var n := 0
	for v in views:
		cam_rig.yaw = v[0]
		cam_rig.pitch = v[1]
		cam_rig.dist = v[2]
		for i in 3:
			await get_tree().process_frame
		await _screenshot("user://logs/scn3d_%s_auto%d.png" % [map_name, n])
		n += 1
	# do FPS that: 2 giay sau khi chup (shader da bien dich, texture da len GPU)
	cam_rig.yaw = 0.0
	cam_rig.pitch = 40.0
	cam_rig.dist = 19.0
	var t0 := Time.get_ticks_msec()
	var frames := 0
	while Time.get_ticks_msec() - t0 < 2000:
		await get_tree().process_frame
		frames += 1
	var fps := frames * 1000.0 / maxf(1.0, float(Time.get_ticks_msec() - t0))
	# dung canh NPC dau tien (neu co): dua nhan vat toi canh no roi chup
	if not npcs.is_empty():
		var target: Node3D = npcs[0]
		player.global_position = target.global_position + Vector3(2.5, 0, 2.5)
		player.snap_to_ground()
		cam_rig.yaw = 200.0
		cam_rig.pitch = 40.0
		cam_rig.dist = 10.0
		for i in 3:
			await get_tree().process_frame
		await _screenshot("user://logs/scn3d_%s_auto%d.png" % [map_name, n])
		n += 1
	# vu khi + don danh: kiem, cam gan, chup giua don
	if player and player.model and not weapons.is_empty():
		_equip_type(1)
		cam_rig.yaw = 160.0
		cam_rig.pitch = 40.0
		cam_rig.dist = 10.0
		for i in 3:
			await get_tree().process_frame
		await _screenshot("user://logs/scn3d_%s_auto%d.png" % [map_name, n])
		n += 1
		last_action = player.model.attack()
		for i in 14:
			await get_tree().process_frame
		await _screenshot("user://logs/scn3d_%s_auto%d.png" % [map_name, n])
		n += 1
		_equip_type(3)
		for i in 3:
			await get_tree().process_frame
		last_action = player.model.attack()
		for i in 14:
			await get_tree().process_frame
		await _screenshot("user://logs/scn3d_%s_auto%d.png" % [map_name, n])
		n += 1
		_equip_type(1)
		for i in 3:
			await get_tree().process_frame
		_cast_skill(KEY_Z)
		for i in 12:
			await get_tree().process_frame
		await _screenshot("user://logs/scn3d_%s_auto%d.png" % [map_name, n])
		n += 1
		for i in 20:
			await get_tree().process_frame
		await _screenshot("user://logs/scn3d_%s_auto%d.png" % [map_name, n])
		n += 1
		_cast_skill(KEY_C)
		for i in 15:
			await get_tree().process_frame
		await _screenshot("user://logs/scn3d_%s_auto%d.png" % [map_name, n])
		n += 1
	# kiem thu navmesh: di thang ve phia NPC dau tien (lo ren) 3 giay, do lech khoi navmesh va khoang cach con lai
	if nav_region and not npcs.is_empty():
		var target: Node3D = npcs[0]
		var start: Vector3 = player.global_position
		var d0 := start.distance_to(target.global_position)
		var worst := 0.0
		player.test_dir = (target.global_position - start).normalized()
		player.test_dir.y = 0.0
		for i in 180:
			await get_tree().process_frame
			var off: float = nav_clamp(player.global_position).distance_to(player.global_position)
			worst = maxf(worst, off)
		player.test_dir = Vector3.ZERO
		var d1: float = player.global_position.distance_to(target.global_position)
		await _screenshot("user://logs/scn3d_%s_auto%d.png" % [map_name, n])
		n += 1
		# diem trong nha (vi tri NPC lo ren) co bi day ra khong
		var inside: Vector3 = target.global_position
		var clamped: Vector3 = nav_clamp(inside)
		print("SCN3D_NAVTEST start_dist=%.1f end_dist=%.1f max_off_mesh=%.3f npc_point_clamp=%.2f" % [d0, d1, worst, clamped.distance_to(inside)])
	print("SCN3D_OK map=%s nodes=%d fps=%.1f draw_calls=%d primitives=%d vram_mb=%.0f" % [
		map_name, stats["nodes"], fps, Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),
		Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME), Performance.get_monitor(Performance.RENDER_VIDEO_MEM_USED) / 1048576.0])
	get_tree().quit()
