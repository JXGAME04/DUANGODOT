# The 2D map bundle laid out in the 3D world (the 2.5D fallback, docs/LO-TRINH-3D.md M3D-4): every region's ground
# tiles are blitted into one 512 x 512 picture and put on a 10.24 x 20.48 m quad (the 2.0 projection halves y, so
# the picture is stretched twice along the depth), the cover pictures lie flat on it, the objects (houses, trees) and
# the "above" pictures stand as boards facing the camera around Y at their foot line.  Regions stream around a focus
# the way KScenePlaceC.update_view does.  Scene units: docs/MAPS.md §2; metres: docs/3D-QUY-UOC.md.
extends Node3D

const KScene3DMath := preload("res://scenes3d/KScene3DMath.gd")
const REGION_PX := 512               # a region on the 2.0 screen (512 scene units wide, 1024 deep)
const KEEP_RADIUS := 2               # regions kept around the focus region (5 x 5)
const LOAD_PER_FRAME := 1

var map_id := 0
var info := {}                       # map.json
var _present := {}                   # region key -> true
var _regions := {}                   # key -> {nodes: Array}
var _queue: Array[String] = []
var _atlas_images := {}              # sprite id -> Image (the atlas picture, read back once)
var _last_focus := Vector2i(-99999, -99999)
var _anims: Array = []               # [{sprite3d, atlas, frame, n, acc}] the animated objects (a well, a flag)
var stats := {"regions": 0, "objects": 0, "ms": 0}


func setup(id: int, map_info: Dictionary) -> void:
	clear()
	map_id = id
	info = map_info
	for key in info.get("regions", []):
		_present[str(key)] = true


func clear() -> void:
	for key in _regions.keys():
		_unload(key)
	_regions.clear()
	_present.clear()
	_queue.clear()
	_anims.clear()
	_atlas_images.clear()
	_last_focus = Vector2i(-99999, -99999)
	map_id = 0
	info = {}


func region_count() -> int:
	return _regions.size()


func anim_count() -> int:
	return _anims.size()


func _key(col: int, row: int) -> String:
	return "%03d_%03d" % [col + int(info.get("region_left", 0)), row + int(info.get("region_top", 0))]


# Streams the regions around a scene point (the character); a few per frame, nearest first.
func update(focus_scene: Vector2, delta: float) -> void:
	if map_id == 0:
		return
	var fc := Vector2i(int(floorf(focus_scene.x / REGION_PX)), int(floorf(focus_scene.y / (REGION_PX * 2.0))))
	if fc != _last_focus:
		_last_focus = fc
		var cols := int(info.get("region_cols", 0))
		var rows := int(info.get("region_rows", 0))
		var wanted := {}
		for row in range(maxi(fc.y - KEEP_RADIUS, 0), mini(fc.y + KEEP_RADIUS, rows - 1) + 1):
			for col in range(maxi(fc.x - KEEP_RADIUS, 0), mini(fc.x + KEEP_RADIUS, cols - 1) + 1):
				var key := _key(col, row)
				if _present.has(key):
					wanted[key] = true
		for key in _regions.keys():
			if not wanted.has(key):
				_unload(key)
		_queue.clear()
		for key in wanted.keys():
			if not _regions.has(key):
				_queue.append(key)
		_queue.sort_custom(func(a: String, b: String) -> bool:
			return _centre(a).distance_squared_to(focus_scene) < _centre(b).distance_squared_to(focus_scene))
		if _regions.is_empty():
			_drain(64)   # entering the world: everything around at once
	_drain(LOAD_PER_FRAME)
	_animate(delta)


func _centre(key: String) -> Vector2:
	var parts := key.split("_")
	var col := int(parts[0]) - int(info.get("region_left", 0))
	var row := int(parts[1]) - int(info.get("region_top", 0))
	return Vector2(col * REGION_PX + REGION_PX * 0.5, row * REGION_PX * 2.0 + REGION_PX)


func _drain(limit: int) -> void:
	var n := 0
	while n < limit and not _queue.is_empty():
		var key: String = _queue.pop_front()
		if not _regions.has(key):
			_load(key)
		n += 1


func _atlas_image(atlas) -> Image:
	if _atlas_images.has(atlas.id):
		return _atlas_images[atlas.id]
	var img: Image = atlas.texture.get_image()
	if img != null and img.is_compressed():
		img.decompress()
	if img != null and img.get_format() != Image.FORMAT_RGBA8:
		img.convert(Image.FORMAT_RGBA8)
	_atlas_images[atlas.id] = img
	return img


func _load(key: String) -> void:
	var data = Assets.region(map_id, key)
	if data == null:
		_present.erase(key)
		return
	var t0 := Time.get_ticks_usec()
	var nodes: Array = []
	var ox := int(data.get("origin_x", 0))   # the region's top-left on the 2.0 screen (px, bundle-relative)
	var oy := int(data.get("origin_y", 0))
	# ---- the ground: every tile into one picture, one quad
	var pic := Image.create(REGION_PX, REGION_PX, false, Image.FORMAT_RGBA8)
	var tiles: Array = data.get("tiles", [])
	for t in tiles:
		var atlas = Assets.sprite(str(t.s))
		if atlas == null:
			continue
		var f := clampi(int(t.f), 0, atlas.frame_count() - 1)
		var fr: Dictionary = atlas.frames[f]
		var src := _atlas_image(atlas)
		if src == null:
			continue
		var at := Vector2i(int(t.x), int(t.y)) + Vector2i(int(fr.ox), int(fr.oy))
		pic.blit_rect(src, Rect2i(int(fr.x), int(fr.y), int(fr.w), int(fr.h)), at)
	if not tiles.is_empty():
		var quad := MeshInstance3D.new()
		var pm := PlaneMesh.new()
		pm.size = Vector2(REGION_PX * KScene3DMath.UNIT, REGION_PX * 2.0 * KScene3DMath.UNIT)
		quad.mesh = pm
		var mat := StandardMaterial3D.new()
		mat.albedo_texture = ImageTexture.create_from_image(pic)
		mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
		mat.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST_WITH_MIPMAPS
		quad.material_override = mat
		quad.position = Vector3((ox + REGION_PX * 0.5) * KScene3DMath.UNIT, 0.0, (oy * 2.0 + REGION_PX) * KScene3DMath.UNIT)
		add_child(quad)
		nodes.append(quad)
	# ---- the objects: cover pictures flat on the ground, the rest as boards at their foot line
	var objects: Array = data.get("objects", [])
	for o in objects:
		var atlas = Assets.sprite(str(o.s))
		if atlas == null:
			continue
		var frame := clampi(int(o.f), 0, atlas.frame_count() - 1)
		var layer := str(o.get("l", "object"))
		var n := int(o.get("n", 0))
		var animated := n > 1 and atlas.frame_count() > 1
		var base := Vector2(int(o.x), int(o.y))
		# static images sit exactly at (x, y) (FRAME_DRAW); animated ones at their reference spot plus the frame's offset
		var pos: Vector2 = base + atlas.frame_offset(frame) if animated else base
		var tex: Texture2D = atlas.frame_texture(frame)
		var size := tex.get_size()
		var spr := Sprite3D.new()
		spr.texture = tex
		spr.pixel_size = KScene3DMath.UNIT
		spr.shaded = false
		spr.alpha_cut = SpriteBase3D.ALPHA_CUT_DISCARD
		spr.alpha_scissor_threshold = 0.5
		spr.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
		spr.centered = true
		if layer == "cover":
			# on the ground: the picture's screen rect (x, y, w, h) covers scene x .. x + w, y * 2 .. (y + h) * 2
			spr.axis = Vector3.AXIS_Y
			spr.scale = Vector3(1.0, 1.0, 2.0)
			spr.position = Vector3((pos.x + size.x * 0.5) * KScene3DMath.UNIT, 0.01, (pos.y + size.y * 0.5) * 2.0 * KScene3DMath.UNIT)
			spr.rotation.y = 0.0
		else:
			# standing: the bottom edge of the picture is the foot line (the object's p1 for the sorted ones), the picture
			# h px tall is h / cos 30 units high (the 2.0 pictures were drawn for the 30 degree view)
			spr.billboard = BaseMaterial3D.BILLBOARD_FIXED_Y
			spr.scale = Vector3(1.0, 1.0 / KScene3DMath.COS30, 1.0)
			var foot_y := (pos.y + size.y) * 2.0
			var p1 = o.get("p1", null)
			if p1 is Array and p1.size() >= 2 and layer == "object":
				foot_y = float(p1[1])
			var h_m := size.y * KScene3DMath.UNIT / KScene3DMath.COS30
			spr.position = Vector3((pos.x + size.x * 0.5) * KScene3DMath.UNIT, h_m * 0.5, foot_y * KScene3DMath.UNIT)
		add_child(spr)
		nodes.append(spr)
		if animated:
			_anims.append({"sprite": spr, "atlas": atlas, "frame": frame, "n": mini(n, atlas.frame_count()), "acc": 0.0, "key": key})
	_regions[key] = {"nodes": nodes}
	stats["regions"] = _regions.size()
	stats["objects"] += objects.size()
	stats["ms"] += (Time.get_ticks_usec() - t0) / 1000
	Log.trace("map3d", "2.5D region", {"key": key, "tiles": tiles.size(), "objects": objects.size(), "ms": (Time.get_ticks_usec() - t0) / 1000})


func _unload(key: String) -> void:
	var r = _regions.get(key)
	if r == null:
		return
	for n in r.nodes:
		if is_instance_valid(n):
			n.queue_free()
	_regions.erase(key)
	var keep: Array = []
	for a in _anims:
		if a.key != key:
			keep.append(a)
	_anims = keep


# BuildinObjNextFrame: a frame every `interval` ms, 20 ms at least
func _animate(delta: float) -> void:
	for a in _anims:
		var spr: Sprite3D = a.sprite
		if not is_instance_valid(spr):
			continue
		a.acc += delta
		var step := maxf(float(a.atlas.interval), 20.0) / 1000.0
		if a.acc >= step:
			a.acc -= step
			a.frame = (a.frame + 1) % int(a.n)
			spr.texture = a.atlas.frame_texture(a.frame)
