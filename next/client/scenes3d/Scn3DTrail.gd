# Scn3DTrail - vet dao (刀光) khi danh: dai giua hai diem start/end cua vu khi (weapons.json anchors) lay mau moi khung
# trong luc animation danh dang phat, ve bang ImmediateMesh cong sang, mo dan theo tuoi. Thay cho SFXMeshTrailDrag cua game goc.
extends MeshInstance3D

var weapon: Node3D            # node vu khi (goc glTF) da gan vao tay
var a_start := Vector3.ZERO   # anchor trong khong gian vu khi
var a_end := Vector3(0, 0, 1)
var color := Color(1.0, 0.95, 0.75, 1.0)
var max_age := 0.22           # giay
var active := false           # true khi dang danh
var _samples: Array = []      # [pos_start, pos_end, time]
var _im := ImmediateMesh.new()


func _ready() -> void:
	top_level = true
	mesh = _im
	var mat := StandardMaterial3D.new()
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mat.blend_mode = BaseMaterial3D.BLEND_MODE_ADD
	mat.cull_mode = BaseMaterial3D.CULL_DISABLED
	mat.vertex_color_use_as_albedo = true
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	mat.depth_draw_mode = BaseMaterial3D.DEPTH_DRAW_DISABLED
	material_override = mat
	cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF


func setup(w: Node3D, anchors: Dictionary) -> void:
	weapon = w
	if anchors.has("start"):
		var s = anchors["start"]
		a_start = Vector3(s[0], s[1], s[2])
	if anchors.has("end"):
		var e = anchors["end"]
		a_end = Vector3(e[0], e[1], e[2])


func _process(_delta: float) -> void:
	var now := Time.get_ticks_msec() / 1000.0
	if active and is_instance_valid(weapon):
		var gt := weapon.global_transform
		_samples.append([gt * a_start, gt * a_end, now])
	while not _samples.is_empty() and now - float(_samples[0][2]) > max_age:
		_samples.pop_front()
	_im.clear_surfaces()
	if _samples.size() < 2:
		return
	_im.surface_begin(Mesh.PRIMITIVE_TRIANGLES)
	for i in range(1, _samples.size()):
		var s0: Array = _samples[i - 1]
		var s1: Array = _samples[i]
		var f0: float = 1.0 - (now - float(s0[2])) / max_age
		var f1: float = 1.0 - (now - float(s1[2])) / max_age
		var c0 := Color(color.r, color.g, color.b, clampf(f0, 0.0, 1.0) * color.a)
		var c1 := Color(color.r, color.g, color.b, clampf(f1, 0.0, 1.0) * color.a)
		var cin0 := Color(c0.r, c0.g, c0.b, c0.a * 0.15)
		var cin1 := Color(c1.r, c1.g, c1.b, c1.a * 0.15)
		# tam giac 1: start0, end0, end1 ; tam giac 2: start0, end1, start1  (mau trong = mo o goc, sang o mui)
		_im.surface_set_color(cin0); _im.surface_add_vertex(s0[0])
		_im.surface_set_color(c0); _im.surface_add_vertex(s0[1])
		_im.surface_set_color(c1); _im.surface_add_vertex(s1[1])
		_im.surface_set_color(cin0); _im.surface_add_vertex(s0[0])
		_im.surface_set_color(c1); _im.surface_add_vertex(s1[1])
		_im.surface_set_color(cin1); _im.surface_add_vertex(s1[0])
	_im.surface_end()
