# The 2.0 client's sprites in the 3D world (the 2.5D fallback of docs/LO-TRINH-3D.md M3D-4): every Sprite2D under a
# canvas node (a KNpc's KNpcRes parts, a KMissle's frame) is mirrored onto a Sprite3D of a board that always faces the
# camera around Y.  The canvas node stays invisible; its state machine and KNpcRes keep painting the frames (the 2.0
# rules), this node only copies textures and offsets.  Sizes: 1 px = UNIT across, and UNIT / cos 30 up - the 2.0
# pictures were drawn for a 30 degree view (KRepresentShell2::CoordinateTransform), a standing sprite of h px is a
# h / cos 30 unit tall thing (docs/3D-QUY-UOC.md §2).
extends Node3D

const KScene3DMath := preload("res://scenes3d/KScene3DMath.gd")
const LAYER_STEP := 0.004      # metres between two parts toward the camera (the child order is the draw order)

var source: Node2D = null      # the canvas node whose Sprite2D descendants are shown
var _pool: Array = []          # Sprite3D nodes in use, in draw order
var _cam: Camera3D = null


func bind(canvas: Node2D) -> void:
	source = canvas


func _process(_delta: float) -> void:
	if source == null or not is_instance_valid(source):
		queue_free()
		return
	if _cam == null or not is_instance_valid(_cam):
		_cam = get_viewport().get_camera_3d()
	if _cam != null:
		# the board faces the camera around Y: the parts' x offsets stay screen-aligned
		global_rotation = Vector3(0.0, _cam.global_transform.basis.get_euler().y, 0.0)
	var sprites: Array = []
	_collect(source, sprites)
	var n := 0
	for sp in sprites:
		var s2: Sprite2D = sp
		if not s2.visible or s2.texture == null:
			continue
		var s3: Sprite3D
		if n < _pool.size():
			s3 = _pool[n]
		else:
			s3 = _make_sprite3d()
			_pool.append(s3)
		n += 1
		if s3.texture != s2.texture:
			s3.texture = s2.texture
		var size := s2.texture.get_size()
		# the canvas node's origin is the feet: a part at (position) with centered = false spans position .. position + size
		var cx: float = s2.position.x + (0.0 if s2.centered else size.x * 0.5)
		var cy: float = s2.position.y + (0.0 if s2.centered else size.y * 0.5)
		s3.position = Vector3(cx * KScene3DMath.UNIT, -cy * KScene3DMath.UNIT / KScene3DMath.COS30, LAYER_STEP * float(n))
		s3.flip_h = s2.flip_h
		s3.flip_v = s2.flip_v
		s3.modulate = s2.modulate
		s3.visible = true
	for i in range(n, _pool.size()):
		_pool[i].visible = false


# every Sprite2D below the node, in tree order (KNpcRes reorders its children into the draw order)
func _collect(n: Node, out: Array) -> void:
	for c in n.get_children():
		if c is Sprite2D:
			out.append(c)
		if c.get_child_count() > 0 and not (c is Label):
			_collect(c, out)


func _make_sprite3d() -> Sprite3D:
	var s3 := Sprite3D.new()
	s3.centered = true
	s3.pixel_size = KScene3DMath.UNIT
	s3.scale = Vector3(1.0, 1.0 / KScene3DMath.COS30, 1.0)
	s3.shaded = false
	s3.double_sided = false
	s3.alpha_cut = SpriteBase3D.ALPHA_CUT_DISCARD
	s3.alpha_scissor_threshold = 0.5
	s3.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
	add_child(s3)
	return s3


# the topmost drawn part's height over the feet, metres (the name block sits over it)
func top_height() -> float:
	var top := 0.0
	for s3 in _pool:
		if s3.visible and s3.texture != null:
			top = maxf(top, s3.position.y + s3.texture.get_size().y * 0.5 * KScene3DMath.UNIT / KScene3DMath.COS30)
	return top
