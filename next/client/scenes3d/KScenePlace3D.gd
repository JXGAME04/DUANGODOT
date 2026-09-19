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
var _ground_plane: StaticBody3D = null   # the flat ground of a map without a bundle
var mode := ""            # "3d" (a map3d bundle), "2.5d" (the 2D bundle as boards and ground pictures), "flat" (nothing)
var ground25: Node3D = null   # KGround25D in 2.5D mode


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
	_shader_mats.clear()
	_tex_cache.clear()
	info = {}
	scene = {}
	stats = {"nodes": 0, "surfaces": 0, "lm_surfaces": 0, "terrain": 0, "load_ms": 0}


func map_name() -> String:
	return str(info.get("name", ""))


func region_count() -> int:
	return ground25.region_count() if ground25 != null else 0


func anim_count() -> int:
	return ground25.anim_count() if ground25 != null else 0


# per frame: the 2.5D ground streams around the character
func update(focus_scene: Vector2, delta: float) -> void:
	if ground25 != null:
		ground25.update(focus_scene, delta)


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
	_sun = DirectionalLight3D.new()
	_sun.name = "Sun"
	var d := Vector3(-0.3, -0.85, 0.4)
	if l.has("dir"):
		d = Vector3(l["dir"][0], l["dir"][1], l["dir"][2])
	_sun.position = Vector3(0, 60, 0)
	_sun.look_at_from_position(_sun.position, _sun.position + d, Vector3.UP)
	_sun.light_color = _col(l.get("color", [1, 1, 0.95]))
	_sun.light_energy = 1.35
	_sun.shadow_enabled = true
	_sun.directional_shadow_max_distance = 90.0
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
			sm.set_shader_parameter("use_lm", true)
			stats["lm_surfaces"] += 1
		else:
			sm.set_shader_parameter("use_lm", false)
		_shader_mats.append(sm)
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
		sm2.set_shader_parameter("use_lm", true)
		var c = mm.get("color", null)
		if c is Array and c.size() >= 4 and kind == "static":
			sm2.set_shader_parameter("tint", Color(c[0], c[1], c[2], c[3]))
		_shader_mats.append(sm2)
		stats["lm_surfaces"] += 1
		return sm2
	return null
