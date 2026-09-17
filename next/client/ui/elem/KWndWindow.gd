# KWndWindow - the base of every part of a window (Ui\Elem\WndWindow.cpp).
#
# Init reads what the old one read: Left, Top, Width, Height, Disable.  A TOP-LEVEL window is then
# placed on the screen by place_on_screen, which is KWndWindow::Init of gamecl.exe 2.0 (VA 0x46CB50,
# docs/VLTK20-CLIENT.md): PositionType 1 centres it and ignores Left/Top, 2..9 dock it, and
# PositionByRate=1 scales a position drawn for 800x600.
extends Control

const KUiScheme := preload("res://ui/KUiScheme.gd")

var section_name := ""
var disabled := false          # WND_S_DISABLE


# Reads the section; false when the layout has no such section (the part then stays hidden, the
# way a window of the old client without its section simply had no size).
func init_from(ini: KUiScheme, section: String) -> bool:
	section_name = section
	name = section
	if ini == null or not ini.has_section(section):
		visible = false
		return false
	position = Vector2(ini.get_integer(section, "Left", 0), ini.get_integer(section, "Top", 0))
	size = Vector2(ini.get_integer(section, "Width", 0), ini.get_integer(section, "Height", 0))
	disabled = ini.get_bool(section, "Disable", false)
	return true


# Where a top-level window goes on a screen of `screen` pixels.
func place_on_screen(ini: KUiScheme, section: String, screen: Vector2i) -> void:
	var w := int(size.x)
	var h := int(size.y)
	var left := ini.get_integer(section, "Left", 0)
	var top := ini.get_integer(section, "Top", 0)
	if ini.get_bool(section, "PositionByRate", false):
		left = left * screen.x / 800
		top = top * screen.y / 600
	var cx := screen.x / 2 - w / 2
	var cy := screen.y / 2 - h / 2
	match ini.get_integer(section, "PositionType", 0):
		1:
			position = Vector2(cx, cy)
		2:
			position = Vector2(cx, screen.y - h)
		3:
			position = Vector2(cx, 0)
		4:
			position = Vector2(0, cy)
		5:
			position = Vector2(screen.x - w, cy)
		6:
			position = Vector2(0, 0)
		7:
			position = Vector2(screen.x - w, 0)
		8:
			position = Vector2(0, screen.y - h)
		9:
			position = Vector2(screen.x - w, screen.y - h)
		_:
			position = Vector2(left, top)


# KWndWindow::Enable
func enable(on: bool) -> void:
	disabled = not on
	queue_redraw()
