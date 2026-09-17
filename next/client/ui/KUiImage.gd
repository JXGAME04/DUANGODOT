# KUiImage - one picture of a window (KUiImageRef of Ui\Elem\UiImage.h).
#
# A sprite of the old client is a box (width x height) and a list of frames, each smaller than the
# box and placed inside it at its own offset.  A window image is drawn WITHOUT
# RUIMAGE_RENDER_FLAG_FRAME_DRAW, which means: put the BOX at the position and the frame where it
# belongs inside.  jxassets export-ui keeps exactly that: an atlas .png, and per frame the rectangle
# in the atlas (x, y, w, h) and the offset in the box (ox, oy).
#
# A .jpg (the pictures of the starting villages) is a single frame the size of the file.
extends RefCounted

var game_path := ""
var texture: Texture2D = null
var box := Vector2i.ZERO        # the sprite's own size
var interval_ms := 0            # KImageParam.nInterval
var frames: Array = []          # [{x, y, w, h, ox, oy}]


static func from_entry(entry) -> RefCounted:
	if entry == null or not (entry is Dictionary):
		return null
	var tex := Assets.ui_texture(str(entry.get("file", "")))
	if tex == null:
		return null
	var script: GDScript = load("res://ui/KUiImage.gd")
	var img = script.new()
	img.game_path = str(entry.get("game_path", ""))
	img.texture = tex
	img.box = Vector2i(int(entry.get("width", 0)), int(entry.get("height", 0)))
	img.interval_ms = int(entry.get("interval", 0))
	img.frames = entry.get("frames", [])
	if img.frames.is_empty():
		var sz := tex.get_size()
		img.frames = [{"x": 0, "y": 0, "w": int(sz.x), "h": int(sz.y), "ox": 0, "oy": 0}]
	if img.box.x <= 0 or img.box.y <= 0:
		var f: Dictionary = img.frames[0]
		img.box = Vector2i(int(f.ox) + int(f.w), int(f.oy) + int(f.h))
	return img


func frame_count() -> int:
	return frames.size()


# Draws frame `n` with the sprite's box at `pos`.
func draw(ci: CanvasItem, pos: Vector2, n: int, modulate: Color = Color.WHITE) -> void:
	if texture == null or frames.is_empty():
		return
	var f: Dictionary = frames[clampi(n, 0, frames.size() - 1)]
	if int(f.w) <= 0 or int(f.h) <= 0:
		return
	ci.draw_texture_rect_region(texture, Rect2(pos.x + float(f.ox), pos.y + float(f.oy), float(f.w), float(f.h)),
		Rect2(float(f.x), float(f.y), float(f.w), float(f.h)), modulate)


# True when the pixel at `at` (relative to the box) is drawn by frame `n`: Trans=1 windows let the
# mouse through their transparent parts (WNDIMG_ES_EXCLUDE_TRANS).
func is_opaque_at(at: Vector2, n: int) -> bool:
	if texture == null or frames.is_empty():
		return false
	var f: Dictionary = frames[clampi(n, 0, frames.size() - 1)]
	var x := int(at.x) - int(f.ox)
	var y := int(at.y) - int(f.oy)
	if x < 0 or y < 0 or x >= int(f.w) or y >= int(f.h):
		return false
	var image := Assets.ui_image_data(texture)
	if image == null:
		return true
	return image.get_pixel(int(f.x) + x, int(f.y) + y).a > 0.0
