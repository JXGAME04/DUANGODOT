# KWndLabeledButton - a button with a line of text on it (Ui\Elem\WndLabeledButton.cpp).
#
# Label= is the text, Font= its size (default 16, and anything under 12 becomes 16), Color /
# BorderColor its colours at rest, OverColor / OverBorderColor under the mouse, SelectColor when
# checked (the over colour when not given).  The text is centred across the button unless
# LabelXOffset > 0 places it; LabelYOffset moves it down.  Without FullText=1 a label longer than
# the button is cut to what fits (Width * 2 / Font characters).
extends "res://ui/elem/KWndButton.gd"

const KFont := preload("res://ui/KFont.gd")

var label := ""
var font_size := 16
var label_x_offset := 0
var label_y_offset := 0
var full_text := false
var font_color := Color.BLACK
var font_border_color := Color.BLACK
var over_color := Color.BLACK
var over_border_color := Color.BLACK
var select_color := Color.BLACK


func init_from(ini: KUiScheme, section: String) -> bool:
	if not super.init_from(ini, section):
		return false
	font_size = ini.get_integer(section, "Font", 16)
	if font_size < 12:
		font_size = 16
	label_x_offset = ini.get_integer(section, "LabelXOffset", 0)
	label_y_offset = ini.get_integer(section, "LabelYOffset", 0)
	full_text = ini.get_bool(section, "FullText", false)
	if not ini.get_bool(section, "Enable", true):
		enable(false)
	font_color = ini.get_color(section, "Color", Color.BLACK)
	font_border_color = ini.get_color(section, "BorderColor", Color.BLACK)
	over_color = ini.get_color(section, "OverColor", Color.BLACK)
	over_border_color = ini.get_color(section, "OverBorderColor", Color.BLACK)
	select_color = ini.get_color(section, "SelectColor", Color.BLACK)
	if select_color == Color.BLACK:
		select_color = over_color
	label = ini.get_string(section, "Label", "")
	return true


func set_label(text: String) -> void:
	label = text
	queue_redraw()


func _draw() -> void:
	super._draw()
	if label == "":
		return
	var shown := label
	if not full_text:
		var max_len := int(size.x) * 2 / font_size
		if shown.length() > max_len:
			shown = shown.substr(0, max_len)
	var color := font_color
	var border := font_border_color
	if over:
		color = over_color
		border = over_border_color
	elif down:
		color = select_color
	var x := (int(size.x) - shown.length() * font_size / 2) / 2
	if label_x_offset > 0:
		x = label_x_offset
	var font = KFont.of(font_size)
	if font != null:
		font.draw(self, Vector2(x, label_y_offset), shown, color, border)
	else:
		draw_string(get_theme_default_font(), Vector2(x, label_y_offset + font_size), shown, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, color)


func _mouse_over() -> void:
	super._mouse_over()
	queue_redraw()


func _mouse_leave() -> void:
	super._mouse_leave()
	queue_redraw()
