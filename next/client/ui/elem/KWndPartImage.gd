# KWndPartImage - a picture that shows only a part of itself (KWndPartImage of the 2.0 client: the life / mana /
# stamina / exp bars of 顶部控制条.ini).  Init 0x004583D0 reads [Section] PartType (0..3, anything else 0) on top
# of KWndImage; SetPart(cur, max) 0x004584D0 hands the two numbers to the sprite (0x00450F00, KUiPartMath) and the
# paint 0x00458200 draws the part rect only.
extends "res://ui/elem/KWndImage.gd"

const KUiPartMath := preload("res://ui/KUiPartMath.gd")

var part_type := 0
var part_cur := 1
var part_max := 1


func init_from(ini: KUiScheme, section: String) -> bool:
	if not super.init_from(ini, section):
		return false
	part_type = ini.get_integer(section, "PartType", 0)
	if part_type < 0 or part_type > 3:
		part_type = 0
	return true


# KWndPartImage::SetPart: max of 0 leaves the picture whole (0x004584D6)
func set_part(cur: int, max_value: int) -> void:
	if max_value == 0:
		return
	if part_cur == cur and part_max == max_value:
		return
	part_cur = cur
	part_max = max_value
	queue_redraw()


func _draw() -> void:
	if image == null or image.texture == null or image.frames.is_empty():
		return
	var f: Dictionary = image.frames[clampi(frame, 0, image.frames.size() - 1)]
	var part := KUiPartMath.part_rect(part_type, part_cur, part_max, int(f.w), int(f.h))
	if part.size.x <= 0 or part.size.y <= 0:
		return
	draw_texture_rect_region(image.texture,
		Rect2(img_offset.x + float(f.ox) + part.position.x, img_offset.y + float(f.oy) + part.position.y, part.size.x, part.size.y),
		Rect2(float(f.x) + part.position.x, float(f.y) + part.position.y, part.size.x, part.size.y))
