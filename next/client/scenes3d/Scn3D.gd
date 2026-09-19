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

var map_name := "world_baling"
var auto := false
var dir := ""
var info := {}
var tex_cache := {}
var lightmaps: Array = []
var shader_mats: Array = []
var use_lm := true
var lm_gain := 1.0
var stats := {"nodes": 0, "surfaces": 0, "lm_surfaces": 0, "terrain": 0, "collision": 0, "load_ms": 0}
var hud: Label
var cam_rig: Node3D   # Scn3DCamera (preload, khong phu thuoc cache class_name)
var player: Node3D    # Scn3DPlayer
var _shot := 0


func _ready() -> void:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("--map="):
			map_name = a.substr(6)
		elif a == "--auto":
			auto = true
	dir = ProjectSettings.globalize_path(ASSETS3D) + "/" + map_name
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
	_setup_player_camera()
	_setup_hud()
	stats["load_ms"] = Time.get_ticks_msec() - t0
	print("SCN3D map=%s nodes=%d surfaces=%d lightmapped=%d terrain=%d load_ms=%d" % [
		map_name, stats["nodes"], stats["surfaces"], stats["lm_surfaces"], stats["terrain"], stats["load_ms"]])
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
	light.light_energy = 1.2
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
	for k in ["BeginPoint01", "BeginPoint", "BeginPointJ"]:
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
	player.snap_to_ground()


func _setup_hud() -> void:
	var layer := CanvasLayer.new()
	layer.name = "HUD"
	add_child(layer)
	hud = Label.new()
	hud.position = Vector2(8, 8)
	hud.add_theme_color_override("font_color", Color.WHITE)
	hud.add_theme_color_override("font_shadow_color", Color.BLACK)
	hud.add_theme_constant_override("shadow_offset_x", 1)
	hud.add_theme_constant_override("shadow_offset_y", 1)
	layer.add_child(hud)


func _process(_delta: float) -> void:
	if hud == null or cam_rig == null or player == null:
		return
	var p: Vector3 = player.global_position
	var t: Dictionary = info.get("table", {})
	hud.text = "%s  %s  |  FPS %d  |  node %d  mat lightmap %d  |  nap %d ms\ncamera yaw %.0f  pitch %.0f  dist %.1f   lightmap %s (gain %.2f)\nnhan vat (Godot) %.1f %.1f %.1f   (Unity) %.1f %.1f %.1f\nchuot phai: xoay | con lan: zoom | Q/E xoay | PgUp/PgDn nghieng | trai: di | WASD | L lightmap | [ ] gain | F12 chup | ESC" % [
		map_name, str(t.get("name", "")), Engine.get_frames_per_second(), stats["nodes"], stats["lm_surfaces"], stats["load_ms"],
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
	var views := [[0.0, 40.0, 19.0], [90.0, 40.0, 19.0], [180.0, 40.0, 19.0], [270.0, 40.0, 19.0], [45.0, 75.0, 21.0]]
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
	print("SCN3D_OK map=%s nodes=%d fps=%.1f draw_calls=%d primitives=%d vram_mb=%.0f" % [
		map_name, stats["nodes"], fps, Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),
		Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME), Performance.get_monitor(Performance.RENDER_VIDEO_MEM_USED) / 1048576.0])
	get_tree().quit()
