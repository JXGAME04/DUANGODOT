# KWndImage - a part that shows a picture (Ui\Elem\WndImage.cpp).
#
# Image= names the sprite, Frame= the frame it starts on, Trans=1 lets the mouse through the
# transparent pixels (WNDIMG_ES_EXCLUDE_TRANS).  A part without Width/Height takes the size of its
# sprite.  The sprite's BOX sits at the part's corner and the frame inside it at its own offset
# (see KUiImage).
extends "res://ui/elem/KWndWindow.gd"

var image = null               # KUiImage
var frame := 0
var exclude_trans := false
var img_offset := Vector2.ZERO # m_ImgOffX / m_ImgOffY
var _flip_ms := 0              # KUiImageRef.nFlipTime


func init_from(ini: KUiScheme, section: String) -> bool:
	if not super.init_from(ini, section):
		return false
	exclude_trans = ini.get_bool(section, "Trans", false)
	set_image(ini.image(section, "image"))
	var start := ini.get_integer(section, "Frame", -1)
	if start >= 0:
		frame = start
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	return true


func set_image(img) -> void:
	image = img
	frame = 0
	_flip_ms = Time.get_ticks_msec()
	if image != null:
		if size.x <= 0.0:
			size.x = image.box.x
		if size.y <= 0.0:
			size.y = image.box.y
	queue_redraw()


func set_frame(n: int) -> void:
	if n != frame:
		frame = n
		queue_redraw()


# IR_NextFrame: steps the animation by the sprite's own interval; true when it wrapped around.
func next_frame() -> bool:
	if image == null or image.frame_count() <= 1:
		return false
	var now := Time.get_ticks_msec()
	var interval: int = maxi(image.interval_ms, 1)
	if now - _flip_ms < interval:
		return false
	_flip_ms += interval
	frame += 1
	queue_redraw()
	if frame >= image.frame_count():
		_flip_ms = now
		frame = 0
		return true
	return false


func _draw() -> void:
	if image != null:
		image.draw(self, img_offset, frame)


func _has_point(point: Vector2) -> bool:
	if not Rect2(Vector2.ZERO, size).has_point(point):
		return false
	if exclude_trans and image != null:
		return image.is_opaque_at(point - img_offset, frame)
	return true
