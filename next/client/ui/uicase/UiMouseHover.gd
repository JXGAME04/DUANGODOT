# KUiMouseHover - the box that pops up over an item (Ui\Elem\MouseHover.cpp; layout
# <theme>\弹出说明文字.ini [MouseOverWnd]).
#
# As the old one measured it: width = the longest line + Indent on both sides, a line is
# Font + 1 pixels tall, the border sprite (ImgWidth pixels wide) is repeated along the top and the
# bottom and adds ImgHeight above and below; the box sits to the lower right of the cursor and
# jumps to its left when it would leave the screen.  The name is centred on TitleBgColor, the
# rest starts at Indent on PropBgColor / DescBgColor (0xb0 alpha).
extends Control

const KUiScheme := preload("res://ui/KUiScheme.gd")
const KFont := preload("res://ui/KFont.gd")

const SCHEME := "chu-thich-vat-pham"
const FOLLOW_OFFSET := Vector2(16, 16)

var img_width := 0
var img_height := 0
var indent := 6
var font_size := 12
var title_bg := Color8(0, 30, 19, 176)
var prop_bg := Color8(0, 30, 19, 176)
var desc_bg := Color8(0, 30, 19, 176)
var border_image = null           # KUiImage of the top / bottom strip
var lines: Array = []             # [{text, color}]; the first is the title
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
	font_size = ini.get_integer("MouseOverWnd", "Font", 12)
	title_bg = _bg(ini.get_color("MouseOverWnd", "TitleBgColor", Color.BLACK))
	prop_bg = _bg(ini.get_color("MouseOverWnd", "PropBgColor", Color.BLACK))
	desc_bg = _bg(ini.get_color("MouseOverWnd", "DescBgColor", Color.BLACK))
	border_image = ini.image("MouseOverWnd", "image")
	_font = KFont.of(font_size)
	return true


static func _bg(c: Color) -> Color:
	c.a8 = 0xb0
	return c


# Shows the lines next to `at` (a point of the parent); hides when there is nothing to say.
func show_lines(new_lines: Array, at: Vector2) -> void:
	lines = new_lines
	if lines.is_empty() or _font == null:
		visible = false
		return
	var longest := 0
	for l in lines:
		longest = maxi(longest, _font.width_of(str(l.text)))
	var w := longest + indent * 2
	var h := (font_size + 1) * lines.size() + img_height * 2
	size = Vector2(w, h)
	var pos := at + FOLLOW_OFFSET
	if pos.x + w > screen.x:
		pos.x = at.x - w
	if pos.y + h > screen.y:
		pos.y = maxf(0.0, at.y - h)
	position = pos.floor()
	visible = true
	queue_redraw()


func hide_lines() -> void:
	visible = false


func _draw() -> void:
	if lines.is_empty() or _font == null:
		return
	var w := int(size.x)
	var y := img_height
	var line_h := font_size + 1
	# the title on its own shade, centred
	draw_rect(Rect2(0, y, w, line_h), title_bg, true)
	var title: Dictionary = lines[0]
	_font.draw(self, Vector2(maxi(indent, w / 2 - _font.width_of(str(title.text)) / 2), y), str(title.text), title.color, Color.BLACK)
	y += line_h
	if lines.size() > 1:
		draw_rect(Rect2(0, y, w, line_h * (lines.size() - 1)), prop_bg, true)
		for i in range(1, lines.size()):
			var l: Dictionary = lines[i]
			_font.draw(self, Vector2(indent, y), str(l.text), l.color, Color.BLACK)
			y += line_h
	if border_image != null and img_width > 0:
		var x := 0
		while x < w:
			border_image.draw(self, Vector2(x, 0), 0)
			border_image.draw(self, Vector2(x, y), 0)
			x += img_width
