# Scn3DLineMesh - the line a child object draws back to its birth point (SFXLineMesh of the reference [TK], skill event 26
# "子物体连线": Côn Lôn's 天际迅雷 lightning): a camera-facing ribbon of `width` metres from the birth position to the node's
# current position, the material's atlas cell (uvNum_X x uvNum_Y grid, uvGrow frames chosen at random every key of uiCurve
# when useRandomGrow) stretched along it; the attach nodes (glow balls) sit at the two ends.
extends MeshInstance3D

var end_node: Node3D = null
var start_pos := Vector3.ZERO
var width := 2.0
var grid := Vector2i(4, 8)
var offset := Vector2i(0, 0)
var frames := 1.0            # uiCurve's last value: frames 0 .. frames-1 are drawn (random when use_random)
var period := 0.09
var use_random := true
var _im := ImmediateMesh.new()
var _cell := 0
var _next := 0.0
var _start_attach: Node3D = null
var _started := false        # the birth point is read on the first frame: the missile view binds its effect before it is placed


func _ready() -> void:
	top_level = true
	mesh = _im
	cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF


func setup(node: Node3D, dir: String, lm: Dictionary, mat_desc) -> void:
	end_node = node
	width = maxf(0.05, float(lm.get("width", 2.0)))
	grid = Vector2i(maxi(1, int(lm.get("nx", 1))), maxi(1, int(lm.get("ny", 1))))
	offset = Vector2i(int(lm.get("ox", 0)), int(lm.get("oy", 0)))
	var curve: Array = lm.get("curve", [])
	frames = maxf(1.0, float(curve[curve.size() - 1][1])) if not curve.is_empty() else float(lm.get("grow", 1))
	period = maxf(0.03, float(curve[curve.size() - 1][0])) if curve.size() > 1 else 0.09
	use_random = bool(lm.get("random", false))
	var mat := StandardMaterial3D.new()
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mat.cull_mode = BaseMaterial3D.CULL_DISABLED
	mat.vertex_color_use_as_albedo = true
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	mat.depth_draw_mode = BaseMaterial3D.DEPTH_DRAW_DISABLED
	mat.blend_mode = BaseMaterial3D.BLEND_MODE_ADD if (mat_desc is Dictionary and str(mat_desc.get("blend", "alpha")) == "add") else BaseMaterial3D.BLEND_MODE_MIX
	if mat_desc is Dictionary and mat_desc.get("tex", null) != null and str(mat_desc["tex"]) != "":
		var img := Image.load_from_file(dir + "/" + str(mat_desc["tex"]))
		if img != null:
			mat.albedo_texture = ImageTexture.create_from_image(img)
	material_override = mat
	var c = lm.get("color", [1, 1, 1, 1])
	_color = Color(c[0], c[1], c[2], c[3])


var _color := Color.WHITE


func hold_start(attach: Node3D) -> void:
	# the start attach node stays at the birth point while the effect itself flies on
	_start_attach = attach
	attach.top_level = true
	if _started:
		attach.global_position = start_pos


func _process(delta: float) -> void:
	if end_node == null or not is_instance_valid(end_node) or not end_node.is_inside_tree():
		queue_free()
		return
	if not _started:
		_started = true
		start_pos = end_node.global_position
		if _start_attach != null and is_instance_valid(_start_attach):
			_start_attach.global_position = start_pos
		return
	_next -= delta
	if _next <= 0.0:
		_next = period
		_cell = (randi() % int(frames)) if use_random else int(_cell + 1) % int(frames)
	if _start_attach != null and is_instance_valid(_start_attach):
		_start_attach.global_position = start_pos
	var a := start_pos
	var b := end_node.global_position
	var cam := get_viewport().get_camera_3d() if is_inside_tree() else null
	var dir := b - a
	if dir.length_squared() < 1e-6:
		_im.clear_surfaces()
		return
	var view: Vector3 = ((a + b) * 0.5 - cam.global_position).normalized() if cam != null else Vector3.UP
	var side := dir.normalized().cross(view)
	if side.length_squared() < 1e-6:
		side = dir.normalized().cross(Vector3.UP)
	side = side.normalized() * width * 0.5
	var col := int(offset.x + _cell) % grid.x
	@warning_ignore("integer_division")
	var row := int(offset.y + (offset.x + _cell) / grid.x)
	var u0 := float(col) / float(grid.x)
	var u1 := float(col + 1) / float(grid.x)
	var v0 := float(row) / float(grid.y)
	var v1 := float(row + 1) / float(grid.y)
	_im.clear_surfaces()
	_im.surface_begin(Mesh.PRIMITIVE_TRIANGLES)
	_im.surface_set_color(_color); _im.surface_set_uv(Vector2(u0, v0)); _im.surface_add_vertex(a - side)
	_im.surface_set_color(_color); _im.surface_set_uv(Vector2(u0, v1)); _im.surface_add_vertex(a + side)
	_im.surface_set_color(_color); _im.surface_set_uv(Vector2(u1, v1)); _im.surface_add_vertex(b + side)
	_im.surface_set_color(_color); _im.surface_set_uv(Vector2(u0, v0)); _im.surface_add_vertex(a - side)
	_im.surface_set_color(_color); _im.surface_set_uv(Vector2(u1, v1)); _im.surface_add_vertex(b + side)
	_im.surface_set_color(_color); _im.surface_set_uv(Vector2(u1, v0)); _im.surface_add_vertex(b - side)
	_im.surface_end()
