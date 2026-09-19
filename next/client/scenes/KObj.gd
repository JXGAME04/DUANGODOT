# An object on the ground (KObj of the old client, client side): a dropped item or a pile of
# money the zone sends as an ENTITY_DROP entity.  Its picture is the ObjData row the zone names
# (template_id) - the sprite `jxassets export-objdata` wrote - drawn from its reference spot like
# a map object (KObj::Draw), with the item's name over it.  A click picks it up (UiGame walks the
# character there first when it is too far: PLAYER_PICKUP_SERVER_DISTANCE).
#
# It answers the same calls UiGame makes on a KNpc, so the entity table can hold both.
extends Node2D

const ENTITY_DROP := 4
const LABEL_COLOR_ITEM := Color8(255, 255, 255)
const LABEL_COLOR_MONEY := Color8(255, 217, 78)

var entity_id := 0
var entity_type := ENTITY_DROP
var display_name := ""
var template_id := 0
var scene_pos := Vector2.ZERO
var count := 0
var life := 0
var life_max := 0
var doing := 0
var is_own := false
var is_target := false
var obj: Dictionary = {}          # the ObjData row
var _sprite: Sprite2D
var _label: Label
var _atlas = null                 # SpriteAtlas
var _frame := 0
var _frame_acc := 0.0
var no_2d := false                # a 3D view draws it: no sprite, no label


static func to_screen(p: Vector2) -> Vector2:
	return Vector2(p.x, p.y * 0.5)


func setup(d: Dictionary, own: bool) -> void:
	entity_id = int(d.id)
	display_name = str(d.name)
	template_id = int(d.get("template_id", 0))
	count = int(d.get("count", 0))
	is_own = own
	scene_pos = Vector2(d.x, d.y)
	position = to_screen(scene_pos)
	obj = Assets.objdata_row(template_id)
	if no_2d:
		visible = false
		return
	if _sprite == null:
		_sprite = Sprite2D.new()
		_sprite.centered = false
		add_child(_sprite)
	if _label == null:
		_label = Label.new()
		_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		_label.size = Vector2(160, 20)
		_label.add_theme_color_override("font_shadow_color", Color.BLACK)
		_label.add_theme_constant_override("shadow_offset_x", 1)
		_label.add_theme_constant_override("shadow_offset_y", 1)
		add_child(_label)
	var money := str(obj.get("kind", "")) == "Money"
	_label.text = display_name if not money else "%d lượng" % count
	_label.add_theme_color_override("font_color", LABEL_COLOR_MONEY if money else LABEL_COLOR_ITEM)
	_label.position = Vector2(-80, -float(int(obj.get("height", 0))) - 22.0)
	_atlas = Assets.sprite(str(obj.get("image", ""))) if obj.get("image", "") != "" else null
	_frame = 0
	if _atlas != null:
		_sprite.texture = _atlas.frame_texture(0)
		# KObj::Draw: the picture's reference spot sits on the object's position
		_sprite.position = _atlas.frame_offset(0) - Vector2(_atlas.center_x, _atlas.center_y)
		_sprite.visible = true
	else:
		_sprite.visible = false
	queue_redraw()


func _process(delta: float) -> void:
	if no_2d:
		return
	# the item's own animation (an item on the ground twinkles now and then: KObj::Activate plays
	# the loop every so often); the interval is the sprite's
	if _atlas == null or _atlas.frame_count() <= 1:
		return
	_frame_acc += delta
	var step := maxf(float(_atlas.interval), 1.0) / 1000.0
	if _frame_acc >= step:
		_frame_acc -= step
		_frame = (_frame + 1) % _atlas.frame_count()
		_sprite.texture = _atlas.frame_texture(_frame)
		_sprite.position = _atlas.frame_offset(_frame) - Vector2(_atlas.center_x, _atlas.center_y)


# ---- what UiGame asks of every entity ----

func apply_move(_mv: Dictionary) -> void:
	pass


func apply_action(_a: Dictionary) -> void:
	pass


func set_life(_l: Dictionary) -> void:
	pass


func set_target(on: bool) -> void:
	is_target = on
	queue_redraw()


func is_dead() -> bool:
	return false


func is_attackable() -> bool:
	return false


func is_pickable() -> bool:
	return true


func is_moving() -> bool:
	return false


func hit_test(local: Vector2) -> bool:
	if _sprite != null and _sprite.visible and _sprite.texture != null:
		return Rect2(_sprite.position, _sprite.texture.get_size()).has_point(local)
	return local.length() <= 12.0


func _draw() -> void:
	if no_2d:
		return
	if _atlas == null:
		draw_circle(Vector2.ZERO, 6.0, LABEL_COLOR_MONEY if str(obj.get("kind", "")) == "Money" else LABEL_COLOR_ITEM)
	if is_target:
		draw_arc(Vector2.ZERO, 14.0, 0.0, TAU, 24, Color(1, 1, 0.5, 0.8), 1.5)
