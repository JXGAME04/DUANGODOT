# KWndList - a list of one-line items (Ui\Elem\WndList.cpp) and its 2.0 descendant with a picture
# behind every row.
#
# The plain list (JX1 source): rows are Font + 1 pixels tall, NumColumn items share a row, the
# picked item takes SelColor / SelBorderColor and, when SelItemBgColor is given, a tinted bar.
#
# The 2.0 list (the class gamecl.exe builds for KUiSelServer, vtable 0x790E84; Init at 0x481890,
# PaintWindow at 0x481970) adds three keys and changes the row:
#   SprImg=           a two-frame sprite drawn behind EVERY item: frame 0, frame 1 for the picked one
#   TextXStart=       how far right of the row's edge the text starts (default 7)
#   PrayBorderColor=  the outline of the picked item while the list is NOT the one in use
#                     (set_gray); SelBorderColor is its outline while it is
#   rows are 18 pixels apart (a constant in the code, not Font + 1), the picture sits 1 pixel and
#   the text 3 pixels below the row's top, and the hit test divides by 18 as well.
# An item whose text contains G_STR_SERVERLIST_STATUS1 "(Đầy)" is written in red, STATUS2
# "(Đề cử)" in green - set mark_red / mark_green to switch that on.
extends "res://ui/elem/KWndWindow.gd"

const KFont := preload("res://ui/KFont.gd")
const SPR_ROW_PITCH := 18

signal item_selected(index: int)       # WND_N_LIST_ITEM_SEL
signal item_activated(index: int)      # WND_N_LIST_ITEM_D_CLICK
signal item_highlighted(index: int)    # WND_N_LIST_ITEM_HIGHLIGHT

var font_size := 16
var num_column := 1
var halign := 0
var item_color := Color.BLACK
var item_border_color := Color.BLACK
var sel_color := Color.BLACK
var sel_border_color := Color.BLACK
var sel_bg_color = null               # Color, or null for no bar
var highlight_enabled := false
var highlight_color := Color.BLACK
var highlight_border_color := Color.BLACK
var row_image = null                  # KUiImage of SprImg=
var text_x_start := 7
var gray_border_color := Color.BLACK
var mark_red := ""
var mark_green := ""

var items: Array = []                 # of String
var top_index := 0                    # m_nTopItemIndex
var selected := -1                    # m_nSelItemIndex
var highlighted := -1

var _focus_border := Color.BLACK      # SelBorderColor as the layout gave it
var _gray := false


func init_from(ini: KUiScheme, section: String) -> bool:
	if not super.init_from(ini, section):
		return false
	num_column = maxi(ini.get_integer(section, "NumColumn", 1), 1)
	font_size = maxi(ini.get_integer(section, "Font", 16), 8)
	halign = ini.get_integer(section, "HAlign", 0)
	item_color = ini.get_color(section, "Color", Color.BLACK)
	item_border_color = ini.get_color(section, "BorderColor", Color.BLACK)
	sel_color = ini.get_color(section, "SelColor", Color.BLACK)
	sel_border_color = ini.get_color(section, "SelBorderColor", Color.BLACK)
	_focus_border = sel_border_color
	sel_bg_color = null
	if ini.get_string(section, "SelItemBgColor", "") != "":
		sel_bg_color = ini.get_color(section, "SelItemBgColor", Color.BLACK)
	highlight_enabled = ini.get_bool(section, "HighLight", false)
	if highlight_enabled:
		highlight_color = ini.get_color(section, "HighLightColor", Color.BLACK)
		highlight_border_color = ini.get_color(section, "HighLightBorderColor", Color.BLACK)
	row_image = ini.image(section, "SprImg")
	text_x_start = ini.get_integer(section, "TextXStart", 7)
	gray_border_color = ini.get_color(section, "PrayBorderColor", _focus_border)
	mouse_filter = Control.MOUSE_FILTER_STOP
	return true


func set_items(list: Array) -> void:
	items = list.duplicate()
	top_index = 0
	highlighted = -1
	if selected >= items.size():
		selected = -1
	queue_redraw()


func count() -> int:
	return items.size()


# KWndList::SetCurSel - no notification, like the old one when called from code.
func set_cur_sel(index: int) -> void:
	selected = index if index >= 0 and index < items.size() else -1
	queue_redraw()


func item_text(index: int) -> String:
	return str(items[index]) if index >= 0 and index < items.size() else ""


# 0x481840 of gamecl.exe: a list that is not the one in use outlines its picked item in
# PrayBorderColor instead of SelBorderColor.
func set_gray(on: bool) -> void:
	_gray = on
	sel_border_color = gray_border_color if on else _focus_border
	queue_redraw()


func is_gray() -> bool:
	return _gray


func row_pitch() -> int:
	return SPR_ROW_PITCH if row_image != null else font_size + 1


func visible_rows() -> int:
	return maxi(int(size.y) / row_pitch(), 1)


# The item under a point of the list, -1 when there is none (0x481C80 for the 2.0 list).
func item_at(at: Vector2) -> int:
	if at.x < 0.0 or at.y < 0.0 or at.x >= size.x or at.y >= size.y:
		return -1
	var index := int(at.y) / row_pitch() * num_column + top_index
	if num_column > 1:
		var column_width := int(size.x) / num_column
		if column_width > 0:
			index += int(at.x) / column_width
	return index if index < items.size() else -1


func _gui_input(event: InputEvent) -> void:
	if disabled:
		return
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
		var index := item_at(event.position)
		if index >= 0:
			var changed := index != selected
			selected = index
			queue_redraw()
			if changed or not event.double_click:
				item_selected.emit(index)
			if event.double_click:
				item_activated.emit(index)
		accept_event()
	elif event is InputEventMouseMotion and highlight_enabled:
		var index := item_at(event.position)
		if index != highlighted:
			highlighted = index
			queue_redraw()
			item_highlighted.emit(index)


func _draw() -> void:
	if items.is_empty():
		return
	var font = KFont.of(font_size)
	if row_image != null:
		_draw_sprite_rows(font)
	else:
		_draw_plain_rows(font)


func _colors_of(index: int) -> Array:
	if index == selected:
		return [sel_color, sel_border_color]
	if index == highlighted:
		return [highlight_color, highlight_border_color]
	return [item_color, item_border_color]


func _draw_sprite_rows(font) -> void:
	for i in items.size():
		row_image.draw(self, Vector2(0, i * SPR_ROW_PITCH + 1), 1 if i == selected else 0)
	var column_width := int(size.x) / num_column
	var last := mini(top_index + visible_rows() * num_column, items.size())
	var x := text_x_start
	var y := 3
	var column := 0
	for i in range(top_index, last):
		if column == num_column:
			column = 0
			x = text_x_start
			y += SPR_ROW_PITCH
		var s := str(items[i])
		var colors := _colors_of(i)
		if mark_red != "" and s.contains(mark_red):
			colors[0] = Color8(255, 0, 0)
		elif mark_green != "" and s.contains(mark_green):
			colors[0] = Color8(0, 255, 0)
		_draw_item(font, s, x, y, column_width, colors)
		x += column_width
		column += 1


func _draw_plain_rows(font) -> void:
	var column_width := int(size.x) / num_column
	var max_len := column_width * 2 / font_size + 1
	var last := mini(top_index + visible_rows() * num_column, items.size())
	var x := 0
	var y := 0
	var column := 0
	for i in range(top_index, last):
		if column == num_column:
			column = 0
			x = 0
			y += font_size + 1
		if i == selected and sel_bg_color != null:
			var bar: Color = sel_bg_color
			bar.a = 0.6
			draw_rect(Rect2(x, y, column_width, font_size + 1), bar)
		var s := str(items[i])
		if s.length() > max_len:
			s = s.substr(0, max_len)
		_draw_item(font, s, x, y, column_width, _colors_of(i))
		x += column_width
		column += 1


func _draw_item(font, s: String, x: int, y: int, column_width: int, colors: Array) -> void:
	var paint_x := x
	if halign == 1:
		paint_x = x + (column_width - s.length() * font_size / 2) / 2
	elif halign == 2:
		paint_x = x + column_width - s.length() * font_size / 2
	if font != null:
		font.draw(self, Vector2(paint_x, y), s, colors[0], colors[1])
	else:
		draw_string(get_theme_default_font(), Vector2(paint_x, y + font_size), s, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, colors[0])
