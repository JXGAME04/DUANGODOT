# KWndText - a piece of text (Ui\Elem\WndText.cpp).
#
#   Font=        size (default 12)          Color= / BorderColor=   letters and their outline
#   HAlign=      0 left, 1 centre, 2 right  VAlign=  0 top, 1 centre, 2 bottom   (one line only)
#   MultiLine=1  the text wraps across the part's width and shows Height / (Font + 1) lines
#   Text=        what it says at first
#
# The arithmetic is the old one, because it decides pixels:
#   centred x = (Width - chars * Font / 2) / 2, never left of the part
#   centred y = (Height - Font - 1) / 2
#   a line of a multi-line text holds Width * 2 / Font characters and is Font + 1 pixels tall
# Text may carry the colour tags of KText.
#
# One thing is NOT from the source: the old engine broke a Vietnamese line after exactly N
# characters, in the middle of a word.  The 2.0 paragraphs have no manual breaks, so here a line
# ends at the last space that fits and only a word longer than the line is cut.
extends "res://ui/elem/KWndWindow.gd"

const KFont := preload("res://ui/KFont.gd")
const KText := preload("res://ui/KText.gd")

var font_size := 12
var halign := 0
var valign := 0
var multi_line := false
var text_color := Color.BLACK
var border_color := Color.BLACK
var top_line := 0              # m_nTopLine: the first line shown of a multi-line text
var text := "":
	set(value):
		text = value
		_layout()
		queue_redraw()

var _lines: Array = []         # [[run, ...], ...] after wrapping; a run is {"text","color","border"}


func init_from(ini: KUiScheme, section: String) -> bool:
	if not super.init_from(ini, section):
		return false
	font_size = ini.get_integer(section, "Font", 12)
	halign = ini.get_integer(section, "HAlign", 0)
	valign = ini.get_integer(section, "VAlign", 0)
	text_color = ini.get_color(section, "Color", Color.BLACK)
	border_color = ini.get_color(section, "BorderColor", Color.BLACK)
	multi_line = ini.get_bool(section, "MultiLine", false)
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	if text == "":
		text = ini.get_string(section, "Text", "")
	else:
		_layout()
	return true


func set_text(value: String) -> void:
	text = value


func set_text_color(color: Color) -> void:
	text_color = color
	queue_redraw()


func line_count() -> int:
	return _lines.size()


func _layout() -> void:
	_lines = []
	if text == "":
		return
	var max_chars := 0
	if font_size > 0 and size.x > 0.0:
		max_chars = int(size.x) * 2 / font_size
	var line: Array = []
	var used := 0
	for run in KText.parse(text):
		var rest: String = run["text"]
		if not multi_line:
			# one line: OutputRichText still breaks at the part's width and shows the first line
			# only, so the text is cut after Width * 2 / Font characters
			if max_chars > 0 and used + rest.length() > max_chars:
				rest = rest.substr(0, maxi(max_chars - used, 0))
			if rest != "":
				line.append({"text": rest, "color": run["color"], "border": run["border"]})
				used += rest.length()
			if run["br"]:
				break
			continue
		while rest != "":
			if max_chars <= 0 or used + rest.length() <= max_chars:
				line.append({"text": rest, "color": run["color"], "border": run["border"]})
				used += rest.length()
				rest = ""
				break
			var room := max_chars - used
			var cut := rest.rfind(" ", room)
			if cut <= 0 and used > 0:
				cut = 0                      # nothing of this run fits: it starts the next line
			elif cut <= 0:
				cut = room                   # one word longer than a whole line
			if cut > 0:
				line.append({"text": rest.substr(0, cut), "color": run["color"], "border": run["border"]})
			_lines.append(line)
			line = []
			used = 0
			rest = rest.substr(cut).lstrip(" ")
		if run["br"]:
			_lines.append(line)
			line = []
			used = 0
	if not line.is_empty():
		_lines.append(line)


func _line_chars(line: Array) -> int:
	var n := 0
	for run in line:
		n += str(run["text"]).length()
	return n


func _draw() -> void:
	if _lines.is_empty():
		return
	var font = KFont.of(font_size)
	var pitch := font_size / 2
	if not multi_line:
		var chars := _line_chars(_lines[0])
		var x := 0
		if halign == 1:
			x = maxi((int(size.x) - chars * font_size / 2) / 2, 0)
		elif halign == 2:
			x = maxi(int(size.x) - chars * font_size / 2, 0)
		var y := 0
		if valign == 1:
			y = (int(size.y) - font_size - 1) / 2
		elif valign == 2:
			y = int(size.y) - font_size - 1
		_draw_line(font, _lines[0], x, y, pitch)
		return
	var shown := int(size.y) / (font_size + 1)
	for i in shown:
		var n := top_line + i
		if n >= _lines.size():
			break
		_draw_line(font, _lines[n], 0, i * (font_size + 1), pitch)


func _draw_line(font, line: Array, x: int, y: int, pitch: int) -> void:
	for run in line:
		var s: String = run["text"]
		var color: Color = text_color if run["color"] == null else run["color"]
		var border: Color = border_color if run["border"] == null else run["border"]
		if font != null:
			font.draw(self, Vector2(x, y), s, color, border)
		else:
			draw_string(get_theme_default_font(), Vector2(x, y + font_size), s, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, color)
		x += s.length() * pitch
