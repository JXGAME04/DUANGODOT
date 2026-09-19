# Scn3DPointTrail - the ribbon a moving effect node leaves behind (PigeonCoopToolkit Trail of the reference [TK]: the darts,
# knives and bolts of the ranged normal attacks, Đường Môn's 夺魂镖 / 天罗地网).  Points are added every MinVertexDistance of
# travel, at most MaxNumberOfPoints, each living Lifetime seconds; the ribbon's width follows SizeOverLife (over the trail's
# length when StretchSizeToFit, else over each point's age) and its colour / alpha the gradient (by age, or over the length
# when StretchColorToFit); the width offset is perpendicular to the travel direction and the "forward" vector - the camera
# direction by default, ForwardOverride (in the source's frame when relative) when set.  The material is the trail's own:
# an atlas cell (TexTransSplit / TexTransOffset), u runs head -> tail (MaterialTileLength 0 = stretched once).
extends MeshInstance3D

var source: Node3D = null
var params := {}
var _points: Array = []     # [{pos, t}] newest last
var _im := ImmediateMesh.new()
var _cell := Rect2(0, 0, 1, 1)
var _life := 0.5
var _min_dist := 0.1
var _max_pts := 20
var _last_pos := Vector3.INF
var _fwd = null             # Vector3 or null
var _fwd_rel := true
var _done := false


func _ready() -> void:
	top_level = true
	mesh = _im
	cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF


func setup(src: Node3D, dir: String, p: Dictionary) -> void:
	source = src
	params = p
	_life = maxf(0.05, float(p.get("life", 0.5)))
	_min_dist = maxf(0.01, float(p.get("min_dist", 0.1)))
	_max_pts = maxi(2, int(p.get("max_pts", 20)))
	var f = p.get("fwd", null)
	if f is Array and f.size() == 3:
		_fwd = Vector3(-float(f[0]), float(f[1]), float(f[2]))   # Unity -> Godot: x mirrored
	_fwd_rel = bool(p.get("fwd_rel", true))
	var sp: Array = p.get("split", [1, 1])
	var off: Array = p.get("offset", [0, 0])
	var nx := maxf(1.0, float(sp[0]))
	var ny := maxf(1.0, float(sp[1]))
	_cell = Rect2(float(off[0]) / nx, float(off[1]) / ny, 1.0 / nx, 1.0 / ny)
	var mat := StandardMaterial3D.new()
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mat.cull_mode = BaseMaterial3D.CULL_DISABLED
	mat.vertex_color_use_as_albedo = true
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	mat.depth_draw_mode = BaseMaterial3D.DEPTH_DRAW_DISABLED
	var m = p.get("material", null)
	mat.blend_mode = BaseMaterial3D.BLEND_MODE_ADD if (m is Dictionary and str(m.get("blend", "alpha")) == "add") else BaseMaterial3D.BLEND_MODE_MIX
	if m is Dictionary and m.get("tex", null) != null and str(m["tex"]) != "":
		var img := Image.load_from_file(dir + "/" + str(m["tex"]))
		if img != null:
			mat.albedo_texture = ImageTexture.create_from_image(img)
	material_override = mat


static func _eval(keys: Array, t: float, dflt: float) -> float:
	if keys.is_empty():
		return dflt
	if t <= float(keys[0][0]):
		return float(keys[0][1])
	for i in range(1, keys.size()):
		if t <= float(keys[i][0]):
			var t0 := float(keys[i - 1][0])
			var t1 := float(keys[i][0])
			var f := 0.0 if t1 <= t0 else (t - t0) / (t1 - t0)
			return lerpf(float(keys[i - 1][1]), float(keys[i][1]), f)
	return float(keys[keys.size() - 1][1])


static func _eval_color(keys: Array, t: float) -> Color:
	if keys.is_empty():
		return Color.WHITE
	if t <= float(keys[0][0]):
		return Color(keys[0][1], keys[0][2], keys[0][3])
	for i in range(1, keys.size()):
		if t <= float(keys[i][0]):
			var t0 := float(keys[i - 1][0])
			var t1 := float(keys[i][0])
			var f := 0.0 if t1 <= t0 else (t - t0) / (t1 - t0)
			return Color(keys[i - 1][1], keys[i - 1][2], keys[i - 1][3]).lerp(Color(keys[i][1], keys[i][2], keys[i][3]), f)
	var k = keys[keys.size() - 1]
	return Color(k[1], k[2], k[3])


func _process(_delta: float) -> void:
	var now := Time.get_ticks_msec() / 1000.0
	if source != null and is_instance_valid(source) and source.is_inside_tree():
		var p := source.global_position
		if _last_pos == Vector3.INF or p.distance_to(_last_pos) >= _min_dist:
			_points.append({"pos": p, "t": now})
			_last_pos = p
			while _points.size() > _max_pts:
				_points.pop_front()
	else:
		_done = true
	while not _points.is_empty() and now - float(_points[0]["t"]) > _life:
		_points.pop_front()
	if _done and _points.is_empty():
		queue_free()
		return
	_rebuild(now)


func _rebuild(now: float) -> void:
	_im.clear_surfaces()
	var n := _points.size()
	if n < 2:
		return
	var cam := get_viewport().get_camera_3d() if is_inside_tree() else null
	var size_keys: Array = params.get("size", [])
	var col_keys: Array = params.get("color", [])
	var alpha_keys: Array = params.get("alpha", [])
	var stretch_size := bool(params.get("stretch_size", true))
	var stretch_color := bool(params.get("stretch_color", false))
	# the head (newest point) is the source itself: index n-1
	var lefts: Array = []
	var rights: Array = []
	var cols: Array = []
	for i in n:
		var pt: Vector3 = _points[i]["pos"]
		var dir: Vector3
		if i < n - 1:
			dir = (_points[i + 1]["pos"] as Vector3) - pt
		else:
			dir = pt - (_points[i - 1]["pos"] as Vector3)
		if dir.length_squared() < 1e-8:
			dir = Vector3.FORWARD
		dir = dir.normalized()
		var fwd: Vector3
		if _fwd != null:
			fwd = (source.global_basis * _fwd) if (_fwd_rel and source != null and is_instance_valid(source)) else _fwd
		elif cam != null:
			fwd = (pt - cam.global_position).normalized()
		else:
			fwd = Vector3.UP
		var side := dir.cross(fwd)
		if side.length_squared() < 1e-6:
			side = dir.cross(Vector3.UP)
		side = side.normalized()
		var along := 1.0 - float(i) / float(n - 1)          # 0 at the head, 1 at the tail
		var age := clampf((now - float(_points[i]["t"])) / _life, 0.0, 1.0)
		var w := maxf(0.0, _eval(size_keys, along if stretch_size else age, 0.1))
		var c := _eval_color(col_keys, along if stretch_color else age)
		c.a = clampf(_eval(alpha_keys, along if stretch_color else age, 1.0), 0.0, 1.0)
		lefts.append(pt - side * w * 0.5)
		rights.append(pt + side * w * 0.5)
		cols.append(c)
	_im.surface_begin(Mesh.PRIMITIVE_TRIANGLES)
	for i in range(n - 1):
		var u0 := 1.0 - float(i) / float(n - 1)
		var u1 := 1.0 - float(i + 1) / float(n - 1)
		var uv_a := Vector2(_cell.position.x + _cell.size.x * u0, _cell.position.y)
		var uv_b := Vector2(_cell.position.x + _cell.size.x * u0, _cell.position.y + _cell.size.y)
		var uv_c := Vector2(_cell.position.x + _cell.size.x * u1, _cell.position.y)
		var uv_d := Vector2(_cell.position.x + _cell.size.x * u1, _cell.position.y + _cell.size.y)
		_im.surface_set_color(cols[i]); _im.surface_set_uv(uv_a); _im.surface_add_vertex(lefts[i])
		_im.surface_set_color(cols[i]); _im.surface_set_uv(uv_b); _im.surface_add_vertex(rights[i])
		_im.surface_set_color(cols[i + 1]); _im.surface_set_uv(uv_d); _im.surface_add_vertex(rights[i + 1])
		_im.surface_set_color(cols[i]); _im.surface_set_uv(uv_a); _im.surface_add_vertex(lefts[i])
		_im.surface_set_color(cols[i + 1]); _im.surface_set_uv(uv_d); _im.surface_add_vertex(rights[i + 1])
		_im.surface_set_color(cols[i + 1]); _im.surface_set_uv(uv_c); _im.surface_add_vertex(lefts[i + 1])
	_im.surface_end()
