# KUiPartMath - the part of a bar picture a value shows (KSpriteImagePart of the 2.0 client, gamecl.exe
# 0x00450F00: what KWndPartImage::SetPart 0x004584D0 -> 0x00450F00 leaves in the sprite's part rect
# +0xac..+0xb8 from PartType +0xc4), kept apart from the drawing scripts so the headless tests reach it.
#   cur >= max  -> the whole picture; cur < 0 -> nothing
#   PartType 0  -> from the left:   right  = w * cur / max
#   PartType 1  -> from the right:  left   = w - w * cur / max
#   PartType 2  -> from the top:    bottom = h * cur / max
#   PartType 3  -> from the bottom: top    = h - h * cur / max
# (integer division, like the idiv of the binary; max of 0 draws the whole picture: 0x004584D6 skips the call)
extends RefCounted


static func part_rect(part_type: int, cur: int, max_value: int, w: int, h: int) -> Rect2i:
	if max_value == 0 or cur >= max_value:
		return Rect2i(0, 0, w, h)
	if cur < 0:
		return Rect2i(0, 0, 0, 0)
	match part_type:
		1:
			@warning_ignore("integer_division")
			var left: int = w - (w * cur) / max_value
			return Rect2i(left, 0, w - left, h)
		2:
			@warning_ignore("integer_division")
			var bottom: int = (h * cur) / max_value
			return Rect2i(0, 0, w, bottom)
		3:
			@warning_ignore("integer_division")
			var top: int = h - (h * cur) / max_value
			return Rect2i(0, top, w, h - top)
		_:
			@warning_ignore("integer_division")
			var right: int = (w * cur) / max_value
			return Rect2i(0, 0, right, h)


# Player_Exp 0x0044AD20: how far into its level a character is, in hundredths (the bar is SetPart(percent, 100))
static func exp_percent(exp: int, level_exp: int, next_level_exp: int) -> int:
	var span: int = next_level_exp - level_exp
	if span <= 0:
		return 100 if exp >= next_level_exp else 0
	@warning_ignore("integer_division")
	var pct: int = ((exp - level_exp) * 100) / span
	return clampi(pct, 0, 100)
