# The 3D map of a zone map (KScenePlaceC's part in 3D, ADR-008): client/assets3d/maps/<id>/map3d.json names a glTF
# scene exported by tools/scn3d/export_scene.py (or, later, our own), its scene.json (render settings, lightmaps,
# materials, marks) and where the zone's scene origin sits in it.  Loads the glTF at run time, puts the lightmap /
# terrain shaders back (scn3d_lm*.gdshader, scn3d_terrain.gdshader), the environment and the sun, and a trimesh
# collision on the terrain so heights and cursor picks can be ray cast.  Lighting rule (THU-NGHIEM-3D.md): the
# reference lightmaps hold indirect light only (Bakery, Mixed / Baked Indirect) - the sun is real time.
extends Node3D

const SH_LM := preload("res://scenes3d/scn3d_lm.gdshader")
const SH_LM2 := preload("res://scenes3d/scn3d_lm_2side.gdshader")
const SH_TER := preload("res://scenes3d/scn3d_terrain.gdshader")
const SH_LM_FADE := preload("res://scenes3d/scn3d_lm_fade.gdshader")
const SH_LM2_FADE := preload("res://scenes3d/scn3d_lm_2side_fade.gdshader")
const SH_SWAY := preload("res://scenes3d/scn3d_lm_sway.gdshader")
const SH_WATER := preload("res://scenes3d/scn3d_water.gdshader")
# CameraBuildingFade of the reference client (GameAssembly.dll, static defaults in global-metadata fieldDefaultValues [TK]):
# the renderers of the building layer between the camera and the character fade to FadeAlpha at FadeSpeed per second,
# the occluders are looked for every DetectInterval seconds along the camera -> target segment shortened by RayPadding.
# M3D-6: the quality setting (user://settings3d.json {"quality": "low" | "medium" | "high"}, or --quality=): shadows, fog,
# how far trees / grass / stones are drawn (visibility_range_end) - the numbers are ours [tự chọn]
const QUALITY := {"low": {"shadows": false, "shadow_dist": 0.0, "tree_range": 60.0, "grass_range": 30.0, "far": 200.0},
	"medium": {"shadows": true, "shadow_dist": 50.0, "tree_range": 120.0, "grass_range": 50.0, "far": 300.0},
	"high": {"shadows": true, "shadow_dist": 90.0, "tree_range": 0.0, "grass_range": 0.0, "far": 400.0}}
static var quality := ""
const FADE_ALPHA := 0.25
const FADE_SPEED := 10.0
const RAY_PADDING := 0.15
const DETECT_INTERVAL := 0.3
const KScene3DMath := preload("res://scenes3d/KScene3DMath.gd")
const TERRAIN_LAYER := 1
const Ground25DScript := preload("res://scenes3d/KGround25D.gd")

var map_id := 0
var info := {}            # map3d.json
var scene := {}           # scene.json of the exported map
var dir := ""             # folder of the glTF (textures, lightmaps)
var origin := Vector3.ZERO   # where the zone's (0, 0) sits, metres
var unit_scale := 1.0     # map3d.scale: 1.0 when the map is at UNIT
var ground_y := 0.0
var stats := {"nodes": 0, "surfaces": 0, "lm_surfaces": 0, "terrain": 0, "load_ms": 0}
var _tex_cache := {}
var _shader_mats: Array = []
var _map_root: Node = null
var _env: WorldEnvironment = null
var _sun: DirectionalLight3D = null
var _indoor := false            # no directional light in the scene's render settings (caves): lightmap only
var _ground_plane: StaticBody3D = null   # the flat ground of a map without a bundle
var mode := ""            # "3d" (a map3d bundle), "2.5d" (the 2D bundle as boards and ground pictures), "flat" (nothing)
var ground25: Node3D = null   # KGround25D in 2.5D mode
var _fade_meshes: Array = []   # MeshInstance3D of the "Buildings" group (the reference building layer) with their AABBs
var _fade_state := {}          # MeshInstance3D -> current alpha (< 1 while faded)
var _occluders := {}           # MeshInstance3D -> true, found at the last detection
var _next_detect := 0.0


# Loads the bundle of a map; false when there is none (the caller falls back to a flat ground).
func load_map(id: int) -> bool:
	clear()
	map_id = id
	var t0 := Time.get_ticks_msec()
	var m3 = Assets.map3d_info(id)
	if not (m3 is Dictionary):
		# no 3D bundle: the 2D bundle as the 2.5D world (M3D-4), or a flat floor when there is none either
		if Assets.has_map(id):
			var m2 = Assets.map_info(id)
			if m2 is Dictionary:
				_setup_25d(id, m2)
				stats["load_ms"] = Time.get_ticks_msec() - t0
				return true
		_make_flat_ground()
		mode = "flat"
		return false
	info = m3
	var base := "%s/maps/%d" % [Assets.assets3d_root(), id]
	var gltf_path := base.path_join(str(info.get("scene", "")))
	dir = gltf_path.get_base_dir()
	var stxt := FileAccess.get_file_as_string(base.path_join(str(info.get("scene_json", "scene.json"))))
	scene = JSON.parse_string(stxt) if stxt != "" else {}
	if scene == null:
		scene = {}
	var o = info.get("origin", [0, 0])
	origin = Vector3(float(o[0]), 0.0, float(o[1]))
	unit_scale = float(info.get("scale", 1.0))
	ground_y = float(info.get("ground_y", 0.0))
	_setup_environment()
	var doc := GLTFDocument.new()
	var st := GLTFState.new()
	var err := doc.append_from_file(gltf_path, st)
	if err != OK:
		Log.error("map3d", "gltf load failed", {"path": gltf_path, "error": error_string(err)})
		_make_flat_ground()
		return false
	_map_root = doc.generate_scene(st)
	if _map_root == null:
		_make_flat_ground()
		return false
	_map_root.name = "Map"
	add_child(_map_root)
	_post_process(_map_root)
	mode = "3d"
	stats["load_ms"] = Time.get_ticks_msec() - t0
	Log.info("map3d", "map loaded", {"map": id, "nodes": stats["nodes"], "lightmapped": stats["lm_surfaces"], "terrain": stats["terrain"],
		"load_ms": stats["load_ms"], "origin": str(origin)})
	return true


func clear() -> void:
	for c in get_children():
		c.queue_free()
	_map_root = null
	_env = null
	_sun = null
	_ground_plane = null
	ground25 = null
	mode = ""
	_fade_meshes.clear()
	_fade_state.clear()
	_occluders.clear()
	_shader_mats.clear()
	_tex_cache.clear()
	info = {}
	scene = {}
	stats = {"nodes": 0, "surfaces": 0, "lm_surfaces": 0, "terrain": 0, "load_ms": 0}


func map_name() -> String:
	return str(info.get("name", ""))


# map3d.json of a 3D map; the 2D map.json (region_left/top for the 2.0 minimap picture) when this is the 2.5D world
func map_info() -> Dictionary:
	if mode == "2.5d" and ground25 != null:
		return ground25.info
	return info


# The quality setting: --quality= on the command line, else user://settings3d.json, else "high"
static func quality_settings() -> Dictionary:
	if quality == "":
		quality = "high"
		for a in OS.get_cmdline_user_args():
			if a.begins_with("--quality="):
				quality = a.substr(10)
		if not QUALITY.has(quality):
			var f := FileAccess.open("user://settings3d.json", FileAccess.READ)
			if f != null:
				var d = JSON.parse_string(f.get_as_text())
				if d is Dictionary and QUALITY.has(str(d.get("quality", ""))):
					quality = str(d["quality"])
		if not QUALITY.has(quality):
			quality = "high"
	return QUALITY[quality]


func region_count() -> int:
	return ground25.region_count() if ground25 != null else 0


func anim_count() -> int:
	return ground25.anim_count() if ground25 != null else 0


# per frame: the 2.5D ground streams around the character; on a 3D map the buildings between the camera and the
# character fade (CameraBuildingFade.Update of the reference client)
func update(focus_scene: Vector2, delta: float, camera_pos: Vector3 = Vector3.INF, target_pos: Vector3 = Vector3.INF) -> void:
	if ground25 != null:
		ground25.update(focus_scene, delta)
	if mode == "3d" and camera_pos != Vector3.INF and target_pos != Vector3.INF:
		_update_building_fade(camera_pos, target_pos, delta)


func _update_building_fade(camera_pos: Vector3, target_pos: Vector3, delta: float) -> void:
	if _fade_meshes.is_empty():
		return
	var now := Time.get_ticks_msec() / 1000.0
	if now >= _next_detect:
		_next_detect = now + DETECT_INTERVAL
		var dir := target_pos - camera_pos
		var dist := dir.length()
		_occluders.clear()
		if dist > RAY_PADDING:
			dir /= dist
			var test_dist := dist - RAY_PADDING
			# CollectByBounds: every building whose bounds the camera -> target segment crosses
			for mi in _fade_meshes:
				if not is_instance_valid(mi) or not mi.visible:
					continue
				var aabb: AABB = mi.global_transform * mi.get_aabb()
				if aabb.intersects_segment(camera_pos, camera_pos + dir * test_dist) or aabb.has_point(camera_pos):
					_occluders[mi] = true
	# UpdateFadeStates: toward FadeAlpha for the occluders, back to 1 for the rest, FadeSpeed a second
	var step := FADE_SPEED * delta
	for mi in _occluders.keys():
		var a: float = float(_fade_state.get(mi, 1.0))
		_fade_state[mi] = maxf(a - step, FADE_ALPHA)
	for mi in _fade_state.keys():
		if not is_instance_valid(mi):
			_fade_state.erase(mi)
			continue
		if not _occluders.has(mi):
			var a: float = float(_fade_state[mi]) + step
			if a >= 1.0:
				_restore(mi)
				_fade_state.erase(mi)
				continue
			_fade_state[mi] = a
		_apply_alpha(mi, float(_fade_state[mi]))


# ApplyAlpha: the surface materials swapped for their fade twin (the same shader writing ALPHA) with the alpha set
func _apply_alpha(mi: MeshInstance3D, alpha: float) -> void:
	if mi.mesh == null:
		return
	for s in mi.mesh.get_surface_count():
		var m := mi.get_surface_override_material(s)
		if m is ShaderMaterial:
			var sm := m as ShaderMaterial
			if sm.shader == SH_LM or sm.shader == SH_LM2:
				var twin := sm.duplicate() as ShaderMaterial
				twin.shader = SH_LM2_FADE if sm.shader == SH_LM2 else SH_LM_FADE
				twin.set_meta("solid", sm)
				mi.set_surface_override_material(s, twin)
				sm = twin
			if sm.shader == SH_LM_FADE or sm.shader == SH_LM2_FADE:
				sm.set_shader_parameter("fade", alpha)


# Restore: the solid material again
func _restore(mi: MeshInstance3D) -> void:
	if mi.mesh == null:
		return
	for s in mi.mesh.get_surface_count():
		var m := mi.get_surface_override_material(s)
		if m is ShaderMaterial and m.has_meta("solid"):
			mi.set_surface_override_material(s, m.get_meta("solid"))


# The 2D map bundle in 3D (KGround25D): a flat collision floor for the rays, a sky-lit environment, the regions stream
# around the character.  The scene origin is the bundle's own (0, 0); the ground is y = 0.
func _setup_25d(id: int, m2: Dictionary) -> void:
	info = {"id": id, "name": str(m2.get("name", "")), "camera": {"mode": "classic"}, "origin": [0, 0], "scale": 1.0, "ground_y": 0.0}
	scene = {}
	origin = Vector3.ZERO
	unit_scale = 1.0
	ground_y = 0.0
	_make_flat_ground()
	if _ground_plane != null:
		for c in _ground_plane.get_children():
			if c is MeshInstance3D:
				c.visible = false   # the pictures are the floor; the box stays for the rays
	ground25 = Node3D.new()
	ground25.set_script(Ground25DScript)
	ground25.name = "Ground25D"
	add_child(ground25)
	ground25.setup(id, m2)
	mode = "2.5d"
	Log.info("map3d", "2.5D map", {"map": id, "name": info["name"], "regions": m2.get("regions", []).size()})


func camera_table() -> Dictionary:
	return info.get("camera", {})


# ---- scene units <-> metres of this map ---------------------------------------------------------

func to_world(scene_pos: Vector2, height_units: float = 0.0) -> Vector3:
	var w := KScene3DMath.to_world(scene_pos, height_units) * unit_scale
	return Vector3(origin.x + w.x, w.y, origin.z + w.z)


func to_scene(world: Vector3) -> Vector2:
	return KScene3DMath.to_scene(Vector3(world.x - origin.x, 0.0, world.z - origin.z) / unit_scale)


# The ground under a point (ray cast on the terrain collision); ground_y when nothing is there.
func ground_height(x: float, z: float) -> float:
	var space := get_world_3d().direct_space_state
	if space == null:
		return ground_y
	var q := PhysicsRayQueryParameters3D.create(Vector3(x, ground_y + 80.0, z), Vector3(x, ground_y - 80.0, z), TERRAIN_LAYER)
	var hit := space.intersect_ray(q)
	return float(hit.position.y) if hit else ground_y


# The terrain point a viewport ray hits; the ground plane at ground_y when the ray misses everything.
func ray_ground(from: Vector3, direction: Vector3) -> Vector3:
	var space := get_world_3d().direct_space_state
	if space != null:
		var q := PhysicsRayQueryParameters3D.create(from, from + direction * 600.0, TERRAIN_LAYER)
		var hit := space.intersect_ray(q)
		if hit:
			return hit.position
	if absf(direction.y) < 0.0001:
		return from + direction * 100.0
	var t := (ground_y - from.y) / direction.y
	return from + direction * maxf(t, 0.0)


# ---- environment (scene.json render) ----------------------------------------------------------

func _setup_environment() -> void:
	var r: Dictionary = scene.get("render", {})
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
	_env = WorldEnvironment.new()
	_env.name = "Env"
	_env.environment = env
	add_child(_env)
	var l: Dictionary = r.get("light", {})
	# a scene without a directional light in its render settings (the caves / mazes: maze_wuling has ambient_mode 3 and
	# lightmaps only) is indoor: no sun, the lightmap carries every light (decoded x2 below) [TK scene.json render]
	_indoor = not l.has("dir") and not bool(scene.get("own", false))
	_sun = DirectionalLight3D.new()
	_sun.name = "Sun"
	var d := Vector3(-0.3, -0.85, 0.4)
	if l.has("dir"):
		d = Vector3(l["dir"][0], l["dir"][1], l["dir"][2])
	_sun.position = Vector3(0, 60, 0)
	_sun.look_at_from_position(_sun.position, _sun.position + d, Vector3.UP)
	_sun.light_color = _col(l.get("color", [1, 1, 0.95]))
	# the reference lightmaps carry only indirect light, so their maps take a strong real-time sun; a map of our own
	# has plain PBR materials lit by the sun and the ambient alone [tự chọn]
	_sun.light_energy = 1.0 if bool(scene.get("own", false)) else (1.0 if _indoor else 1.35)   # indoor: the maze terrain shader (地形_迷宫_A高度) has no lightmap path yet -> a fill sun [tự chọn, F6]
	if _indoor:
		env.ambient_light_energy = 0.35   # a little fill so unlit faces are not pitch black [tự chọn]
	if bool(scene.get("own", false)):
		env.ambient_light_energy = 0.6
	var q := quality_settings()
	_sun.shadow_enabled = bool(q["shadows"])
	_sun.directional_shadow_max_distance = float(q["shadow_dist"])
	add_child(_sun)


# A map without a 3D bundle: a flat lit ground the size of the zone map, so the 2.5D fallback and tests have a floor.
func _make_flat_ground() -> void:
	if _env == null:
		var env := Environment.new()
		env.background_mode = Environment.BG_COLOR
		env.background_color = Color(0.55, 0.65, 0.8)
		env.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
		env.ambient_light_color = Color(0.6, 0.6, 0.65)
		_env = WorldEnvironment.new()
		_env.environment = env
		add_child(_env)
		_sun = DirectionalLight3D.new()
		_sun.rotation_degrees = Vector3(-55, 30, 0)
		_sun.shadow_enabled = true
		add_child(_sun)
	var w := float(maxi(Game.scene_w, 1024)) * KScene3DMath.UNIT
	var h := float(maxi(Game.scene_h, 1024)) * KScene3DMath.UNIT
	_ground_plane = StaticBody3D.new()
	_ground_plane.name = "Ground"
	_ground_plane.collision_layer = TERRAIN_LAYER
	var shape := CollisionShape3D.new()
	var box := BoxShape3D.new()
	box.size = Vector3(w, 0.2, h)
	shape.shape = box
	shape.position = Vector3(w * 0.5, -0.1, h * 0.5)
	_ground_plane.add_child(shape)
	var mi := MeshInstance3D.new()
	var pm := PlaneMesh.new()
	pm.size = Vector2(w, h)
	mi.mesh = pm
	mi.position = Vector3(w * 0.5, 0.0, h * 0.5)
	var mat := StandardMaterial3D.new()
	mat.albedo_color = Color(0.36, 0.42, 0.28)
	mi.material_override = mat
	_ground_plane.add_child(mi)
	add_child(_ground_plane)


func _col(a) -> Color:
	if a is Array and a.size() >= 3:
		return Color(clampf(a[0], 0, 1), clampf(a[1], 0, 1), clampf(a[2], 0, 1))
	return Color.WHITE


# ---- materials (scene.json materials / nodes, the exporter's lightmap slots) -------------------------

func _tex(file: String) -> Texture2D:
	if file == "":
		return null
	if _tex_cache.has(file):
		return _tex_cache[file]
	var img := Image.load_from_file(dir + "/" + file)
	var t: Texture2D = null
	if img != null:
		img.generate_mipmaps()
		t = ImageTexture.create_from_image(img)
	_tex_cache[file] = t
	return t


func _lightmap(i: int) -> Texture2D:
	var lms: Array = scene.get("lightmaps", [])
	if i < 0 or i >= lms.size() or lms[i] == null:
		return null
	return _tex(str(lms[i]))


func _st(v) -> Vector4:
	if v is Array and v.size() >= 4:
		return Vector4(v[0], v[1], v[2], v[3])
	return Vector4(1, 1, 0, 0)


func _post_process(root: Node) -> void:
	var nodes: Array = scene.get("nodes", [])
	var mats: Array = scene.get("materials", [])
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
		if bool(scene.get("own", false)):
			# a map of our own (import_map3d.py, docs/3D-HOA-SI.md): the node name's prefix says what it is
			meta = {"group": _own_group(mi.name)}
		stats["nodes"] += 1
		var mesh := mi.mesh
		if mesh == null:
			continue
		if bool(scene.get("own", false)):
			_own_materials(mi, str(meta.get("group", "")))
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
			mi.add_to_group("terrain")
		elif str(meta.get("group", "")) == "Buildings":
			_fade_meshes.append(mi)
		var q := quality_settings()
		var group := str(meta.get("group", ""))
		if group == "Tree" and float(q["tree_range"]) > 0.0:
			mi.visibility_range_end = float(q["tree_range"])
			mi.visibility_range_end_margin = 8.0
			mi.visibility_range_fade_mode = GeometryInstance3D.VISIBILITY_RANGE_FADE_SELF
		elif (group == "Grass" or group == "Stone") and float(q["grass_range"]) > 0.0:
			mi.visibility_range_end = float(q["grass_range"])
			mi.visibility_range_end_margin = 5.0
			mi.visibility_range_fade_mode = GeometryInstance3D.VISIBILITY_RANGE_FADE_SELF


# docs/3D-HOA-SI.md: terrain / walk / building / tree / grass / water / stone / prop by the node name's prefix
static func _own_group(node_name: String) -> String:
	var n := node_name.to_lower()
	if n.begins_with("terrain"):
		return "terrain"
	if n.begins_with("building"):
		return "Buildings"
	if n.begins_with("tree"):
		return "Tree"
	if n.begins_with("grass"):
		return "Grass"
	if n.begins_with("water"):
		return "Water"
	if n.begins_with("walk"):
		return "walk"
	return "Stone"


# Our own map's glTF materials stay as they are (PBR + the sun), except: the walk mesh is not drawn (it is the zone's
# obstacle grid), tree / grass get the sway shader over their albedo, water gets the water shader
func _own_materials(mi: MeshInstance3D, group: String) -> void:
	if group == "walk":
		mi.visible = false
		return
	if group != "Tree" and group != "Grass" and group != "Water":
		return
	for s in mi.mesh.get_surface_count():
		var src := mi.mesh.surface_get_material(s)
		var std := src as StandardMaterial3D if src is StandardMaterial3D else null
		var sm := ShaderMaterial.new()
		if group == "Water":
			sm.shader = SH_WATER
			if std != null and std.albedo_texture != null:
				sm.set_shader_parameter("bottom_tex", std.albedo_texture)
			sm.set_shader_parameter("water_color", std.albedo_color if std != null else Color(0.2, 0.45, 0.6))
			sm.set_shader_parameter("brightness", 1.0)
			sm.set_shader_parameter("alpha_add", 0.7)
		else:
			sm.shader = SH_SWAY
			if std != null and std.albedo_texture != null:
				sm.set_shader_parameter("albedo_tex", std.albedo_texture)
			sm.set_shader_parameter("tint", std.albedo_color if std != null else Color.WHITE)
			sm.set_shader_parameter("use_lm", false)
			sm.set_shader_parameter("alpha_test", std != null and std.transparency == BaseMaterial3D.TRANSPARENCY_ALPHA_SCISSOR)
			sm.set_shader_parameter("cutoff", std.alpha_scissor_threshold if std != null else 0.5)
			sm.set_shader_parameter("sway_mode", 2 if group == "Grass" else 1)
			sm.set_shader_parameter("sway_amount", 0.08 if group == "Grass" else 0.15)
			sm.set_shader_parameter("sway_radius", 0.8 if group == "Grass" else 4.0)
		mi.set_surface_override_material(s, sm)
		_shader_mats.append(sm)


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
			sm.set_shader_parameter("use_lm", true)
			sm.set_shader_parameter("lm_gain", 2.0 if _indoor else 1.0)   # dLDR lightmap: decoded x2 (unity_Lightmap_HDR) [TK]; outdoor stays 1 with the sun
			stats["lm_surfaces"] += 1
		else:
			sm.set_shader_parameter("use_lm", false)
		_shader_mats.append(sm)
		return sm
	var base: String = str(mm.get("base", ""))
	var shader_name := str(mm.get("shader", ""))
	var floats: Dictionary = mm.get("floats", {})
	if kind == "water":
		# 水体高光双浪_水底扭曲: the reference water material's numbers on our water shader
		var w := ShaderMaterial.new()
		w.shader = SH_WATER
		if texs.has("_NormalTex"):
			w.set_shader_parameter("normal_tex", _tex(texs["_NormalTex"]["file"]))
			w.set_shader_parameter("normal_st", _st(texs["_NormalTex"].get("st")))
		if texs.has("_BottomTex"):
			w.set_shader_parameter("bottom_tex", _tex(texs["_BottomTex"]["file"]))
		if texs.has("_ReflectTex"):
			w.set_shader_parameter("reflect_tex", _tex(texs["_ReflectTex"]["file"]))
		var c = mm.get("color", null)
		if c is Array and c.size() >= 4:
			w.set_shader_parameter("water_color", Color(c[0], c[1], c[2], c[3]))
		w.set_shader_parameter("flow_dir", Vector2(float(floats.get("_FlowDirX", 0.6)), float(floats.get("_FlowDirY", 0.0))))
		for pair in [["flow_speed", "_FlowSpeed"], ["ripple", "_ripple"], ["distort", "_DistortStrength"], ["distort_time", "_DistortTimeFactor"],
				["brightness", "_Brightness"], ["spec_size", "_SpecSize"], ["spec_strength", "_SpecSt"], ["alpha", "_Alpha"], ["alpha_add", "_AlphaAdd"],
				["blend_ref_bottom", "_BlendRefBottom"]]:
			if floats.has(pair[1]):
				w.set_shader_parameter(pair[0], float(floats[pair[1]]))
		var sun = scene.get("render", {}).get("light", {}).get("dir", null)
		if sun is Array and sun.size() >= 3:
			w.set_shader_parameter("sun_dir", Vector3(sun[0], sun[1], sun[2]))
		_shader_mats.append(w)
		return w
	var sway := shader_name.contains("晃动")   # the swaying leaf / grass shaders of the reference (Alpha剪裁_树叶_自圆周晃动, Alpha剪裁_草_晃动_...)
	if lm != null and base != "" and texs.has(base) and kind == "static":
		var sm2 := ShaderMaterial.new()
		sm2.shader = SH_LM2 if mm.get("double", false) else SH_LM
		if sway:
			sm2.shader = SH_SWAY
			var grass := shader_name.contains("草")
			sm2.set_shader_parameter("sway_mode", 2 if grass else 1)
			sm2.set_shader_parameter("sway_speed", float(floats.get("speed", 3.0)))
			sm2.set_shader_parameter("sway_amount", float(floats.get("windStr", 0.08)) if grass else float(floats.get("noroff", 0.2)))
			sm2.set_shader_parameter("sway_radius", float(floats.get("gHeight", 0.8)) if grass else float(floats.get("_radius", 10.0)))
			sm2.set_shader_parameter("wind_speed", float(floats.get("windSpeed", 1.5)))
		sm2.set_shader_parameter("albedo_tex", _tex(texs[base]["file"]))
		sm2.set_shader_parameter("lightmap_tex", _lightmap(int(lm[0])))
		sm2.set_shader_parameter("lm_st", Vector4(lm[1], lm[2], lm[3], lm[4]))
		sm2.set_shader_parameter("alpha_test", mm.get("alpha", "OPAQUE") == "MASK")
		sm2.set_shader_parameter("cutoff", float(mm.get("cutoff", 0.5)))
		sm2.set_shader_parameter("use_lm", true)
		sm2.set_shader_parameter("lm_gain", 2.0 if _indoor else 1.0)
		var c = mm.get("color", null)
		if c is Array and c.size() >= 4 and kind == "static":
			sm2.set_shader_parameter("tint", Color(c[0], c[1], c[2], c[3]))
		_shader_mats.append(sm2)
		stats["lm_surfaces"] += 1
		return sm2
	return null
