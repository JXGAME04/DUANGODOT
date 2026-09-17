# MapView - draws an exported map bundle around the camera.
#
# Layers (children, in draw order):
#   ground : tiles (z 0) and cover objects (z 1)      static, per region
#   ysort  : buildin objects + entities, sorted by y  entities are added here by World
#   above  : objects flagged "above" (roofs, bridges) drawn over everything
# Regions are 512 x 512 screen px; those within `margin` regions of the camera rect are kept.
extends Node2D

const REGION_PX := 512
const TICK_SECONDS := 1.0 / 18.0   # old game animation frame

var map_id := 0
var info: Dictionary = {}
var ground: Node2D
var ysort: Node2D
var above: Node2D
var _regions := {}          # key -> {nodes: Array[Node], anims: Array}
var _present := {}          # key -> true (regions that exist in the bundle)
var _cols := 0
var _rows := 0
var _margin := 1
var _anim_time := 0.0
var _last_view := Rect2()
var _anims: Array = []      # [{sprite: Sprite2D, atlas: SpriteAtlas, frame: int, n: int, acc: float}]


func _ready() -> void:
	ground = Node2D.new()
	ground.name = "Ground"
	add_child(ground)
	ysort = Node2D.new()
	ysort.name = "YSort"
	ysort.y_sort_enabled = true
	ysort.z_index = 1
	add_child(ysort)
	above = Node2D.new()
	above.name = "Above"
	above.z_index = 2
	add_child(above)


func load_map(id: int) -> bool:
	clear()
	var data = Assets.map_info(id)
	if data == null:
		return false
	map_id = id
	info = data
	_cols = int(info.get("region_cols", 0))
	_rows = int(info.get("region_rows", 0))
	for key in info.get("regions", []):
		_present[key] = true
	Log.info("map", "map bundle loaded", {"id": id, "name": info.get("name", ""), "regions": _present.size(),
		"screen": "%dx%d" % [screen_width(), screen_height()]})
	return true


func clear() -> void:
	for key in _regions.keys():
		_unload_region(key)
	_regions.clear()
	_present.clear()
	_anims.clear()
	info = {}
	map_id = 0


func screen_width() -> int:
	return int(info.get("scene_w", 0))


func screen_height() -> int:
	return int(info.get("scene_h", 0)) / 2


# Region key for a bundle-relative region column/row.
func _key(col: int, row: int) -> String:
	return "%03d_%03d" % [col + int(info.get("region_left", 0)), row + int(info.get("region_top", 0))]


# Call every frame with the camera's visible rectangle in screen pixels.
func update_view(view: Rect2, delta: float) -> void:
	if map_id == 0:
		return
	_anim_time += delta
	_animate(delta)
	if view == _last_view:
		return
	_last_view = view
	# tall buildings are anchored in the region below the pixels they cover: keep two extra rows
	var c0 := clampi(int(floor(view.position.x / REGION_PX)) - _margin, 0, _cols - 1)
	var c1 := clampi(int(floor(view.end.x / REGION_PX)) + _margin, 0, _cols - 1)
	var r0 := clampi(int(floor(view.position.y / REGION_PX)) - _margin, 0, _rows - 1)
	var r1 := clampi(int(floor(view.end.y / REGION_PX)) + _margin + 1, 0, _rows - 1)
	var wanted := {}
	for row in range(r0, r1 + 1):
		for col in range(c0, c1 + 1):
			var key := _key(col, row)
			if _present.has(key):
				wanted[key] = true
	for key in _regions.keys():
		if not wanted.has(key):
			_unload_region(key)
	for key in wanted.keys():
		if not _regions.has(key):
			_load_region(key)


func _load_region(key: String) -> void:
	var data = Assets.region(map_id, key)
	if data == null:
		_present.erase(key)
		return
	var nodes: Array = []
	var ox := int(data.get("origin_x", 0))
	var oy := int(data.get("origin_y", 0))
	var tiles: Array = data.get("tiles", [])
	var t0 := Time.get_ticks_usec()
	for t in tiles:
		var atlas := Assets.sprite(str(t.s))
		if atlas == null:
			continue
		var frame := int(t.f)
		var sp := Sprite2D.new()
		sp.centered = false
		sp.texture = atlas.frame_texture(frame)
		sp.position = Vector2(ox + int(t.x), oy + int(t.y)) + atlas.frame_offset(frame)
		ground.add_child(sp)
		nodes.append(sp)
	for o in data.get("objects", []):
		var atlas := Assets.sprite(str(o.s))
		if atlas == null:
			continue
		var frame := int(o.f)
		var layer := str(o.l)
		var sp := Sprite2D.new()
		sp.centered = false
		sp.texture = atlas.frame_texture(frame)
		var x := int(o.x)
		var y := int(o.y)
		if layer == "cover":
			sp.position = Vector2(x, y) + atlas.frame_offset(frame)
			sp.z_index = 1
			ground.add_child(sp)
		elif layer == "above":
			sp.position = Vector2(x, y) + atlas.frame_offset(frame)
			above.add_child(sp)
		else:
			# a holder positioned on the object's base line so y-sort works against entities
			var holder := Node2D.new()
			var sy := int(o.get("sy", y))
			holder.position = Vector2(x, sy)
			sp.position = Vector2(0, y - sy) + atlas.frame_offset(frame)
			holder.add_child(sp)
			ysort.add_child(holder)
			nodes.append(holder)
			sp = null
		if sp != null:
			nodes.append(sp)
		var n := int(o.get("n", 0))
		if n > 1 and atlas.frame_count() > 1:
			var anim_sprite: Sprite2D = nodes.back() if sp != null else nodes.back().get_child(0)
			_anims.append({"sprite": anim_sprite, "atlas": atlas, "frame": frame, "n": mini(n, atlas.frame_count()), "acc": 0.0, "key": key,
				"base": anim_sprite.position - atlas.frame_offset(frame)})
	_regions[key] = {"nodes": nodes}
	Log.trace("map", "region loaded", {"key": key, "tiles": tiles.size(), "objects": data.get("objects", []).size(),
		"ms": (Time.get_ticks_usec() - t0) / 1000})


func _unload_region(key: String) -> void:
	var r = _regions.get(key)
	if r == null:
		return
	for n in r.nodes:
		n.queue_free()
	_regions.erase(key)
	var keep: Array = []
	for a in _anims:
		if a.key != key:
			keep.append(a)
	_anims = keep


func _animate(delta: float) -> void:
	for a in _anims:
		a.acc += delta
		var step: float = TICK_SECONDS * a.atlas.interval
		if a.acc < step:
			continue
		a.acc -= step
		a.frame = (a.frame + 1) % a.n
		var sp: Sprite2D = a.sprite
		if is_instance_valid(sp):
			sp.texture = a.atlas.frame_texture(a.frame)
			sp.position = a.base + a.atlas.frame_offset(a.frame)


func region_count() -> int:
	return _regions.size()
