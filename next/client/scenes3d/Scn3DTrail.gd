# Scn3DTrail - vet dao (刀光) khi danh: dai giua hai diem start/end cua vu khi (weapons.json anchors) lay mau moi khung
# trong luc animation danh dang phat, ve bang ImmediateMesh, mo dan theo tuoi.  Tham so theo XWeaponTrail cua bo tham khao
# [TK] (assets3d/sfx/Daoguang_dg_xw_*.json "xtrail": MaxFrame khung o Fps -> tuoi vet, MyColor + EmissiveColor, o atlas
# cua MyMaterial): anim_effect dong 1..6 chon vet theo pham chat vu khi (trang/xanh/tim/vang/bach kim/huyen kim).
extends MeshInstance3D

var weapon: Node3D            # node vu khi (goc glTF) da gan vao tay
var a_start := Vector3.ZERO   # anchor trong khong gian vu khi
var a_end := Vector3(0, 0, 1)
var color := Color(1.0, 0.95, 0.75, 1.0)
var max_age := 0.22           # giay
var active := false           # true khi dang danh
var _samples: Array = []      # [pos_start, pos_end, time]
var _im := ImmediateMesh.new()
var _cell := Rect2(0, 0, 1, 1)   # o atlas (u, v tu tren) cua vet
var _textured := false


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


# The reference trail's parameters (export_sfx.py read_xtrail of a Daoguang/dg_xw_* prefab): age = MaxFrame / Fps, the colour
# MyColor + EmissiveColor (SFXMeshModify-like: added), the atlas cell of its material's texture (v from the top)
func apply_params(dir: String, xt: Dictionary) -> void:
	var fps := maxf(1.0, float(xt.get("fps", 30.0)))
	max_age = maxf(0.05, float(xt.get("max_frame", 5)) / fps)
	var c = xt.get("color", [1, 1, 1, 1])
	var e = xt.get("emissive", [0, 0, 0, 0])
	color = Color(c[0] + e[0], c[1] + e[1], c[2] + e[2], c[3])
	var m = xt.get("material", null)
	var mat := material_override as StandardMaterial3D
	if m is Dictionary and mat != null:
		var texf = m.get("tex", null)
		if texf != null and str(texf) != "":
			var p := dir + "/" + str(texf)
			var img := Image.load_from_file(p)
			if img != null:
				mat.albedo_texture = ImageTexture.create_from_image(img)
				_textured = true
		mat.blend_mode = BaseMaterial3D.BLEND_MODE_ADD if str(m.get("blend", "add")) == "add" else BaseMaterial3D.BLEND_MODE_MIX
	var cell = xt.get("cell", [1, 1, 0, 0])
	var nx := maxf(1.0, float(cell[0]))
	var ny := maxf(1.0, float(cell[1]))
	_cell = Rect2(float(cell[2]) / nx, float(cell[3]) / ny, 1.0 / nx, 1.0 / ny)


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
		var cin0 := Color(c0.r, c0.g, c0.b, c0.a * (1.0 if _textured else 0.15))
		var cin1 := Color(c1.r, c1.g, c1.b, c1.a * (1.0 if _textured else 0.15))
		# u = tuoi doc vet (mui 0 .. duoi 1), v = goc (start) 0 .. mui (end) 1, trong o atlas
		var u0 := _cell.position.x + _cell.size.x * clampf(1.0 - f0, 0.0, 1.0)
		var u1 := _cell.position.x + _cell.size.x * clampf(1.0 - f1, 0.0, 1.0)
		var vs := _cell.position.y
		var ve := _cell.position.y + _cell.size.y
		# tam giac 1: start0, end0, end1 ; tam giac 2: start0, end1, start1  (mau trong = mo o goc, sang o mui)
		_im.surface_set_color(cin0); _im.surface_set_uv(Vector2(u0, vs)); _im.surface_add_vertex(s0[0])
		_im.surface_set_color(c0); _im.surface_set_uv(Vector2(u0, ve)); _im.surface_add_vertex(s0[1])
		_im.surface_set_color(c1); _im.surface_set_uv(Vector2(u1, ve)); _im.surface_add_vertex(s1[1])
		_im.surface_set_color(cin0); _im.surface_set_uv(Vector2(u0, vs)); _im.surface_add_vertex(s0[0])
		_im.surface_set_color(c1); _im.surface_set_uv(Vector2(u1, ve)); _im.surface_add_vertex(s1[1])
		_im.surface_set_color(cin1); _im.surface_set_uv(Vector2(u1, vs)); _im.surface_add_vertex(s1[0])
	_im.surface_end()
