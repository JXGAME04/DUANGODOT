# KWndPopupMenu - the little menu that pops up on a player (the 2.0 client's 0x00475690 for the name menu, the entries
# gamecl.exe 0x004C2450 picks from G_UIGAME_0..17 by the target's sign and my own state; the 2004 KUiGame's
# SEL_ACTION_MENU -> ProcessPeople).  The look is a plain one - a dark box with the game font, one line per entry, the
# entry under the mouse in the highlight colour - until the menu sprites of the 2.0 popup are read (docs/CLIENT-2.0.md §22).
extends Control

const KFont := preload("res://ui/KFont.gd")

const FONT := 14
const PAD_X := 8
const PAD_Y := 4
const LINE := FONT + 3

signal picked(index: int)   # the entry's index in `entries` (the caller keeps what each means)
signal dismissed

var entries: Array = []       # of String, or {"text", "color"} (the channel menu 0x00472620 colours each line by 0x004B6530)
var _hover := -1
var text_color := Color(218.0 / 255.0, 255.0 / 255.0, 165.0 / 255.0)
var border_color := Color(23.0 / 255.0, 68.0 / 255.0, 0.0)
var hover_color := Color(255.0 / 255.0, 253.0 / 255.0, 122.0 / 255.0)
var bg_color := Color(0.0, 0.0, 0.0, 0.8)


func _ready() -> void:
	visible = false
	mouse_filter = Control.MOUSE_FILTER_STOP


func open_at(items: Array, at: Vector2, screen: Vector2i) -> void:
	entries = items.duplicate()
	if entries.is_empty():
		hide_menu()
		return
	var font = KFont.of(FONT)
	var w := 60
	for e in entries:
		var t := entry_text(e)
		w = maxi(w, (font.width_of(t) if font != null else t.length() * FONT / 2) + PAD_X * 2)
	size = Vector2(w, entries.size() * LINE + PAD_Y * 2)
	position = Vector2(clampf(at.x, 0.0, screen.x - size.x), clampf(at.y, 0.0, screen.y - size.y))
	_hover = -1
	visible = true
	queue_redraw()


func hide_menu() -> void:
	if visible:
		visible = false
		dismissed.emit()


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		var i := _index_at(event.position)
		if i != _hover:
			_hover = i
			queue_redraw()
	elif event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_LEFT:
			var i := _index_at(event.position)
			visible = false
			if i >= 0:
				picked.emit(i)
			else:
				dismissed.emit()
		else:
			hide_menu()
		accept_event()


func _index_at(local: Vector2) -> int:
	if local.x < 0.0 or local.y < PAD_Y or local.x >= size.x:
		return -1
	var i := int((local.y - PAD_Y) / LINE)
	return i if i >= 0 and i < entries.size() else -1


func _draw() -> void:
	draw_rect(Rect2(Vector2.ZERO, size), bg_color)
	draw_rect(Rect2(Vector2.ZERO, size), border_color, false, 1.0)
	var font = KFont.of(FONT)
	for i in entries.size():
		var y := PAD_Y + i * LINE
		var color := hover_color if i == _hover else entry_color(entries[i])
		var t := entry_text(entries[i])
		if font != null:
			font.draw(self, Vector2(PAD_X, y), t, color, border_color)
		else:
			draw_string(get_theme_default_font(), Vector2(PAD_X, y + FONT), t, HORIZONTAL_ALIGNMENT_LEFT, -1, FONT, color)


static func entry_text(e) -> String:
	return str(e.get("text", "")) if e is Dictionary else str(e)


func entry_color(e) -> Color:
	return e.get("color", text_color) if e is Dictionary else text_color
