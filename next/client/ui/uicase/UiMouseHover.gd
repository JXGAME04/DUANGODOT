# KUiMouseHover - the box that pops up over an item (Ui\Elem\MouseHover.cpp of the 2.0 client -
# KMouseOver, layout <theme>\弹出说明文字.ini [MouseOverWnd]).
#
# For a game object the text goes into the title block (SetMouseHoverInfo(.., false, false): no
# head / tail strip, not following the cursor) and KMouseOver::Update / PaintMouseHoverInfo lay it
# out as the engine's line counter (TGetEncodedTextLineCountAE) does:
#   every "\n" ends a line, an empty line included; text after the last "\n" is a line only when
#   it has characters; a line longer than 64 characters is broken there (no word wrapping);
#   the width is the longest line, at least INFO_MIN_LEN = 26 characters, a character being
#   Font / 2 pixels (TCVN3 text: one byte, one half-width cell), plus Indent on both sides;
#   a line is Font + 1 pixels tall; one shade of TitleBgColor with 0xb0 alpha behind it all;
#   every line is centred: x = left + width / 2 - len * Font / 4.
# The box sits where ALW_GetWndPosition puts it: centred on the cursor's x (kept on screen),
# below the cursor (CURSOR_HEIGHT = 32) while the cursor is in the upper half, above it below.
extends Control

const KUiScheme := preload("res://ui/KUiScheme.gd")
const KFont := preload("res://ui/KFont.gd")
const KTextEncode := preload("res://ui/KTextEncode.gd")

const SCHEME := "chu-thich-vat-pham"
const INFO_MIN_LEN := 26      # half-width characters
const WRAP_LEN := 64          # TGetEncodedTextLineCountAE(.., 64, ..)
const CURSOR_HEIGHT := 32

var img_width := 0
var img_height := 0
var indent := 6
var font_size := 12
var title_bg := Color8(0, 30, 19, 176)
var prop_bg := Color8(0, 30, 19, 176)
var desc_bg := Color8(0, 30, 19, 176)
var border_image = null           # KUiImage of the top / bottom strip (not drawn for game objects)
var lines: Array = []             # [{parts:[{text,color}], len}] after wrapping
var screen := Vector2i(1024, 768)
var _font = null


func load_scheme(the_screen: Vector2i) -> bool:
	screen = the_screen
	name = "UiMouseHover"
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	visible = false
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not ini.has_section("MouseOverWnd"):
		return false
	img_width = maxi(0, ini.get_integer("MouseOverWnd", "ImgWidth", 0))
	img_height = maxi(0, ini.get_integer("MouseOverWnd", "ImgHeight", 0))
	indent = ini.get_integer("MouseOverWnd", "Indent", 6)
	font_size = maxi(8, ini.get_integer("MouseOverWnd", "Font", 12))
	title_bg = _bg(ini.get_color("MouseOverWnd", "TitleBgColor", Color.BLACK))
	prop_bg = _bg(ini.get_color("MouseOverWnd", "PropBgColor", Color.BLACK))
	desc_bg = _bg(ini.get_color("MouseOverWnd", "DescBgColor", Color.BLACK))
	border_image = ini.image("MouseOverWnd", "image")
	_font = KFont.of(font_size)
	return true


static func _bg(c: Color) -> Color:
	c.a8 = 0xb0
	return c


# The lines of a rich text the way the engine counts them: split at "\n" (the tail after the
# last "\n" counts only when it has characters), each broken at WRAP_LEN visible characters.
static func layout_lines(text: String, base: Color) -> Array:
	var out: Array = []
	var pieces := text.split("\n")
	for i in range(pieces.size()):
		var piece: String = pieces[i]
		var last: bool = i == pieces.size() - 1
		var runs := KTextEncode.runs_of(piece, base)
		var visible := 0
		for r in runs:
			visible += str(r.text).length()
		if last and visible == 0:
			break
		# break every WRAP_LEN characters, colour runs carried over
		var cur: Array = []
		var n := 0
		for r in runs:
			var t := str(r.text)
			var color: Color = r.color
			while t != "":
				var room := WRAP_LEN - n
				var take := mini(room, t.length())
				if take > 0:
					cur.append({"text": t.substr(0, take), "color": color})
					n += take
					t = t.substr(take)
				if n >= WRAP_LEN and t != "":
					out.append({"parts": cur, "len": n})
					cur = []
					n = 0
		if cur.is_empty():
			cur = [{"text": "", "color": runs.back().color if runs.size() > 0 else base}]
		out.append({"parts": cur, "len": n})
	return out


# Shows the description for a cursor at `at` (a point of the parent); hides when it is empty.
func show_text(text: String, at: Vector2) -> void:
	lines = []
	if _font == null:
		visible = false
		return
	lines = layout_lines(text, Color.WHITE)
	if lines.is_empty():
		visible = false
		return
	var longest := INFO_MIN_LEN
	for l in lines:
		longest = maxi(longest, int(l.len))
	var w: int = font_size * longest / 2 + indent * 2
	var h: int = (font_size + 1) * lines.size()
	size = Vector2(w, h)
	# ALW_GetWndPosition(cursor)
	var x := int(at.x) - w / 2
	if x + w > screen.x:
		x = screen.x - w
	x = maxi(0, x)
	var y := int(at.y)
	if y > (screen.y - CURSOR_HEIGHT) / 2:
		y -= h
	else:
		y += CURSOR_HEIGHT
	if y + h > screen.y:
		y = screen.y - h
	y = maxi(0, y)
	position = Vector2(x, y)
	visible = true
	queue_redraw()


# The old entry point: lines as KUiItemView.describe gives them ([{text, color, parts}])
func show_lines(new_lines: Array, at: Vector2) -> void:
	var text := ""
	for l in new_lines:
		var parts: Array = l.get("parts", [{"text": str(l.get("text", "")), "color": l.get("color", Color.WHITE)}])
		for p in parts:
			var c: Color = p.color
			text += "<color=0x%02x%02x%02x>%s" % [c.r8, c.g8, c.b8, str(p.text)]
		text += "\n"
	show_text(text, at)


func hide_lines() -> void:
	visible = false


func _draw() -> void:
	if lines.is_empty() or _font == null:
		return
	var w := int(size.x)
	var line_h := font_size + 1
	draw_rect(Rect2(0, 0, w, line_h * lines.size()), title_bg, true)
	var y := 0
	for l in lines:
		# nX = left + width / 2 - lineLen * font / 4: the line centred by its character count
		var x: int = w / 2 - int(l.len) * font_size / 4
		for p in l.parts:
			_font.draw(self, Vector2(x, y), str(p.text), p.color, Color.BLACK)
			x += _font.width_of(str(p.text))
		y += line_h
