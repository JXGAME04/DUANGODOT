# KScenePlaceC - draws an exported map bundle around the camera the way the old client did.
#
# Layers (children, in draw order):
#   Ground  : tiles                                          static, per region
#   Covers  : cover objects (roads, carpets)                 RUIMAGE_RENDER_FLAG_FRAME_DRAW
#   Objects : buildin objects + entities in the draw order of the old KIpoTree sorting
#             rules (KIpotBranch / KIpotLeaf); the order is written to the children's z_index
#   Above   : "above head" objects (smoke, banners) in (oPos1.y, oPos1.z) order
# Regions are 512 x 512 screen px; those within `margin` regions of the camera rect are kept.
extends Node2D

const KIpoTree := preload("res://scenes/KIpoTree.gd")
const KIpotLeaf := preload("res://scenes/KIpotLeaf.gd")
const KSceneMath := preload("res://scenes/KSceneMath.gd")

const REGION_PX := 512
const REGION_SCENE_H := 1024   # RWPP_AREGION_HEIGHT (scene units)
const MAX_Z := 4000            # z_index budget of the object layer (Godot allows up to 4096)

var map_id := 0
var info: Dictionary = {}
var ground: Node2D
var covers: Node2D
var objects: Node2D
var above: Node2D
var _regions := {}          # key -> {nodes: Array[Node], leaves: Array, above: Array}
var _present := {}          # key -> true (regions that exist in the bundle)
var _cols := 0
var _rows := 0
var _margin := 1
var _last_view := Rect2(-99999, -99999, 0, 0)
var _load_queue: Array[String] = []
var _anims: Array = []      # [{sprite, atlas, frame, n, acc, key, base}]
var _tree := KIpoTree.new()
var _tree_dirty := false    # regions came or went: rebuild the tree
var _order_dirty := false   # something moved inside the tree: rewrite the z order
var _entities := {}         # entity node -> runtime KIpotLeaf
var _parts: Array = []      # Sprite2D pool drawing the cut parts of tree objects
var _above_items: Array = []  # [{key: Vector2i(y, z), node}] sorted like m_pObjsAbove


func _ready() -> void:
	ground = Node2D.new()
	ground.name = "Ground"
	add_child(ground)
	covers = Node2D.new()
	covers.name = "Covers"
	add_child(covers)
	objects = Node2D.new()
	objects.name = "Objects"
	add_child(objects)
	above = Node2D.new()
	above.name = "Above"
	above.z_index = MAX_Z + 90
	add_child(above)


# The tree's RefCounted leaves reference each other: fell it explicitly so nothing (and no
# atlas texture held by a leaf) outlives the renderer.
func _exit_tree() -> void:
	clear()


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
	_tree.fell()
	for sp in _parts:
		sp.queue_free()
	_parts.clear()
	_above_items.clear()
	info = {}
	map_id = 0
	_last_view = Rect2(-99999, -99999, 0, 0)


func screen_width() -> int:
	return int(info.get("scene_w", 0))


func screen_height() -> int:
	return int(info.get("scene_h", 0)) / 2


# Region key for a bundle-relative region column/row.
func _key(col: int, row: int) -> String:
	return "%03d_%03d" % [col + int(info.get("region_left", 0)), row + int(info.get("region_top", 0))]


# ---- entities (KScenePlaceC::AddObject / MoveObject / RemoveObject) --------------------------

# Registers an entity node (a child of `objects` with a `scene_pos` in scene units) with the
# sorting tree.  Its foot point is re-read every frame.
func add_entity(node: Node2D) -> void:
	if _entities.has(node):
		return
	var leaf := KIpotLeaf.new()
	leaf.type = KIpotLeaf.Type.RUNTIME
	leaf.item = node
	leaf.line_start = _foot(node)
	leaf.position = leaf.line_start
	_entities[node] = leaf
	if not _tree_dirty:
		_tree.add_leaf_point(leaf)
	_order_dirty = true


func remove_entity(node: Node2D) -> void:
	var leaf = _entities.get(node)
	if leaf == null:
		return
	_tree.pluck_rto(leaf)
	_entities.erase(node)
	_order_dirty = true


static func _foot(node: Node2D) -> Vector2i:
	return Vector2i(int(node.scene_pos.x), int(node.scene_pos.y) + KIpotLeaf.Y_ADJUST)


func _update_entities() -> void:
	for node in _entities.keys():
		var leaf = _entities[node]
		if not is_instance_valid(node):
			_tree.pluck_rto(leaf)
			_entities.erase(node)
			_order_dirty = true
			continue
		var foot := _foot(node)
		if foot == leaf.position:
			continue
		leaf.line_start = foot
		if _tree_dirty:
			leaf.position = foot     # the rebuild inserts it
			continue
		_tree.pluck_rto(leaf)
		leaf.position = foot
		_tree.add_leaf_point(leaf)
		_order_dirty = true


# ---- per frame ------------------------------------------------------------------------------

# Call every frame with the camera's visible rectangle in screen pixels.
func update_view(view: Rect2, delta: float) -> void:
	if map_id == 0:
		return
	_animate(delta)
	_drain_load_queue()
	# recompute the region set only when the camera moved a good part of a region
	if not (_last_view.size == view.size and _last_view.position.distance_to(view.position) < 64.0):
		_last_view = view
		_recompute_regions(view)
	_update_entities()
	if _tree_dirty:
		_rebuild_tree()
	if _order_dirty:
		_apply_order()


func _recompute_regions(view: Rect2) -> void:
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
	_load_queue.clear()
	for key in wanted.keys():
		if not _regions.has(key):
			_load_queue.append(key)
	# regions nearest the camera first
	var centre := view.get_center()
	_load_queue.sort_custom(func(a: String, b: String) -> bool:
		return _region_centre(a).distance_squared_to(centre) < _region_centre(b).distance_squared_to(centre))
	if _regions.is_empty():
		_drain_load_queue(64)   # first frame in the world: load everything in view at once


func _region_centre(key: String) -> Vector2:
	var parts := key.split("_")
	var col := int(parts[0]) - int(info.get("region_left", 0))
	var row := int(parts[1]) - int(info.get("region_top", 0))
	return Vector2(col * REGION_PX + REGION_PX / 2, row * REGION_PX + REGION_PX / 2)


# Loads a few regions per frame so walking never stalls the frame.
func _drain_load_queue(limit: int = 2) -> void:
	var n := 0
	while n < limit and not _load_queue.is_empty():
		var key: String = _load_queue.pop_front()
		if not _regions.has(key):
			_load_region(key)
		n += 1


func _load_region(key: String) -> void:
	var data = Assets.region(map_id, key)
	if data == null:
		_present.erase(key)
		return
	var nodes: Array = []
	var leaves: Array = []
	var above_nodes: Array = []
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
		var n := int(o.get("n", 0))
		var animated := n > 1 and atlas.frame_count() > 1
		var sp := Sprite2D.new()
		sp.centered = false
		sp.texture = atlas.frame_texture(frame)
		var base := Vector2(int(o.x), int(o.y))
		# static images are placed exactly (FRAME_DRAW); animated ones are drawn from their
		# reference spot and add each frame's own offset (REF_SPOT, DrawScaleSprite)
		sp.position = base + atlas.frame_offset(frame) if animated else base
		if layer == "cover":
			covers.add_child(sp)
		elif layer == "above":
			var p1: Array = o.get("p1", [0, 0])
			var entry := {"key": Vector2i(int(p1[1]), int(o.get("z1", 0))), "node": sp}
			_insert_above(entry)
			above_nodes.append(entry)
		else:
			objects.add_child(sp)
			leaves.append(_make_leaf(o, sp, atlas, frame, base))
		nodes.append(sp)
		if animated:
			_anims.append({"sprite": sp, "atlas": atlas, "frame": frame, "n": mini(n, atlas.frame_count()), "acc": 0.0,
				"key": key, "base": base})
	_regions[key] = {"nodes": nodes, "leaves": leaves, "above": above_nodes}
	_tree_dirty = true
	Log.trace("map", "region loaded", {"key": key, "tiles": tiles.size(), "objects": data.get("objects", []).size(),
		"ms": (Time.get_ticks_usec() - t0) / 1000})


func _make_leaf(o: Dictionary, sp: Sprite2D, atlas, frame: int, base: Vector2):
	var leaf := KIpotLeaf.new()
	leaf.type = KIpotLeaf.Type.BUILDIN
	var k := str(o.get("k", "p"))
	if k == "l":
		leaf.sort = KIpotLeaf.Sort.LINE
	elif k == "t":
		leaf.sort = KIpotLeaf.Sort.TREE
	else:
		leaf.sort = KIpotLeaf.Sort.POINT
	var p1: Array = o.get("p1", [0, 0])
	var p2: Array = o.get("p2", p1)
	leaf.line_start = Vector2i(int(p1[0]), int(p1[1]))
	leaf.line_end = Vector2i(int(p2[0]), int(p2[1]))
	leaf.angle = float(o.get("ang", 0.0))
	leaf.nodical = float(o.get("nod", 0.0))
	leaf.item = sp
	leaf.image = atlas
	leaf.frame = frame
	leaf.base = base
	leaf.reset()
	return leaf


# m_pObjsAbove: kept sorted by (oPos1.y, oPos1.z); the child order of `above` is the draw order.
func _insert_above(entry: Dictionary) -> void:
	var key: Vector2i = entry.key
	var lo := 0
	var hi := _above_items.size()
	while lo < hi:
		var mid := (lo + hi) / 2
		var k: Vector2i = _above_items[mid].key
		if k.x < key.x or (k.x == key.x and k.y < key.y):
			lo = mid + 1
		else:
			hi = mid
	_above_items.insert(lo, entry)
	above.add_child(entry.node)
	above.move_child(entry.node, lo)


func _unload_region(key: String) -> void:
	var r = _regions.get(key)
	if r == null:
		return
	for leaf in r.leaves:
		leaf.item = null
		leaf.image = null
		leaf.reset()
	for entry in r.above:
		_above_items.erase(entry)
	for n in r.nodes:
		n.queue_free()
	_regions.erase(key)
	var keep: Array = []
	for a in _anims:
		if a.key != key:
			keep.append(a)
	_anims = keep
	_tree_dirty = true


# ---- sorting tree (KScenePlaceC::EnterProcessArea) -------------------------------------------

# Rebuilds the tree from every loaded region in the old client's order: tree objects grouped
# by line (longest line first, x order inside), line objects longest first, point objects in
# region order, then the characters.
func _rebuild_tree() -> void:
	_tree_dirty = false
	_tree.fell()
	var focus := _last_view.get_center()
	var fx := int(focus.x)
	var fy := int(focus.y) * 2
	_tree.set_permanent_branch_pos(fx - REGION_PX * 2, fx + REGION_PX * 2, fy - REGION_SCENE_H * 2)
	var trees: Array = []
	var lines: Array = []
	var points: Array = []
	var keys := _regions.keys()
	keys.sort()
	for key in keys:
		for leaf in _regions[key].leaves:
			leaf.reset()
			if leaf.sort == KIpotLeaf.Sort.TREE:
				trees.append(leaf)
			elif leaf.sort == KIpotLeaf.Sort.LINE:
				lines.append(leaf)
			else:
				points.append(leaf)
	# tree objects lying on one line become one branch line (TreeObjSet)
	var groups: Array = []
	for obj in trees:
		var g = null
		for cand in groups:
			if KSceneMath.is_line_linkable(obj.angle, obj.nodical, cand.angle, cand.nodical):
				g = cand
				break
		if g == null:
			groups.push_front({"angle": obj.angle, "nodical": obj.nodical, "objs": [obj], "lp1": obj.position, "lp2": obj.end_pos})
			continue
		obj.angle = g.angle
		obj.nodical = g.nodical
		var idx := 0
		while idx < g.objs.size() and g.objs[idx].position.x <= obj.position.x:
			idx += 1
		g.objs.insert(idx, obj)
		if g.lp1.x > obj.position.x:
			g.lp1 = obj.position
		if g.lp2.x < obj.end_pos.x:
			g.lp2 = obj.end_pos
	for g in groups:
		var d: Vector2i = g.lp1 - g.lp2
		g["len2"] = d.x * d.x + d.y * d.y
	while not groups.is_empty():
		var best := 0
		for i in range(1, groups.size()):
			if groups[best].len2 < groups[i].len2:
				best = i
		for obj in groups[best].objs:
			_tree.add_branch(obj)
		groups.remove_at(best)
	# line objects: longest base line first
	var sorted_lines: Array = []
	var lens: Array = []
	for leaf in lines:
		var d: Vector2i = leaf.end_pos - leaf.position
		var len2 := d.x * d.x + d.y * d.y
		var k := 0
		while k < lens.size() and len2 <= lens[k]:
			k += 1
		sorted_lines.insert(k, leaf)
		lens.insert(k, len2)
	for leaf in sorted_lines:
		_tree.add_leaf_line(leaf)
	for leaf in points:
		_tree.add_leaf_point(leaf)
	for node in _entities.keys():
		var leaf = _entities[node]
		leaf.reset()
		_tree.add_leaf_point(leaf)
	_order_dirty = true


# Writes the tree's draw order to z_index (KIpoTree::Paint).  Cut tree objects draw as parts.
func _apply_order() -> void:
	_order_dirty = false
	var out: Array = []
	_tree.paint(out)
	var n := out.size()
	var scale := 1.0 if n <= MAX_Z else float(MAX_Z) / float(n)
	var part_index := 0
	for i in n:
		var leaf = out[i]
		var z := 1 + int(i * scale)
		if leaf.type == KIpotLeaf.Type.RUNTIME:
			if is_instance_valid(leaf.item):
				leaf.item.z_index = z
			continue
		var sp: Sprite2D = leaf.item
		if sp == null:
			continue
		if leaf.is_clone:
			sp = _part_sprite(part_index)
			part_index += 1
		if leaf.img_part:
			_apply_part(leaf, sp)
		elif sp.texture != leaf.image.frame_texture(leaf.frame) and not _is_animated(sp):
			sp.texture = leaf.image.frame_texture(leaf.frame)
			sp.position = leaf.base
			sp.visible = true
		if sp.z_index != z:
			sp.z_index = z
	for j in range(part_index, _parts.size()):
		_parts[j].visible = false


func _is_animated(sp: Sprite2D) -> bool:
	for a in _anims:
		if a.sprite == sp:
			return true
	return false


func _part_sprite(index: int) -> Sprite2D:
	while index >= _parts.size():
		var sp := Sprite2D.new()
		sp.centered = false
		objects.add_child(sp)
		_parts.append(sp)
	return _parts[index]


# KIpotBranch::PaintABranchObject with bImgPart: the columns of the image between the part's
# start and end, measured along the full base line.
@warning_ignore("integer_division")
func _apply_part(leaf, sp: Sprite2D) -> void:
	var fr: Dictionary = leaf.image.frames[clampi(leaf.frame, 0, leaf.image.frame_count() - 1)]
	var w := int(fr.w)
	var entire: int = leaf.line_end.x - leaf.line_start.x
	var lt := 0
	var rb := w
	if entire != 0:
		lt = clampi((w * (leaf.position.x - leaf.line_start.x)) / entire, 0, w)
		rb = clampi((w * (leaf.end_pos.x - leaf.line_start.x)) / entire, lt, w)
	sp.texture = leaf.image.frame_part_texture(leaf.frame, lt, rb)
	sp.position = leaf.base + Vector2(lt, 0)
	sp.visible = rb > lt


func _animate(delta: float) -> void:
	for a in _anims:
		a.acc += delta
		# BuildinObjNextFrame in the old client: the sprite's own interval in milliseconds, never
		# faster than 20 ms, cycling through every frame of the sprite
		var step: float = maxf(20.0, float(a.atlas.interval)) / 1000.0
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


func anim_count() -> int:
	return _anims.size()


# Draw order of the object layer as a list of leaves (debugging / tests).
func draw_order() -> Array:
	var out: Array = []
	_tree.paint(out)
	return out
