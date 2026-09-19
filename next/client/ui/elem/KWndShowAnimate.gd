# KWndShowAnimate - a top-level window that slides into place when it is shown
# (Ui\Elem\WndShowAnimate.cpp over WndMovingImage.cpp).
#
# StartPos=x,y is where the window starts from; it then covers a tenth of the way every 50 ms
# (ms_nMoveSpeed = 10 of ms_nFullRange = 100, ms_uMoveTimeInterval = 50), so it is home after half
# a second.  A positive StartPos is measured from an 800x600 screen and moves out with a bigger
# one: StartPos=800,0 always means "from just beyond the right edge".
#
# Where "home" is comes from place_on_screen (PositionType of the 2.0 layouts).
extends "res://ui/elem/KWndImage.gd"

const MOVE_SPEED := 10
const FULL_RANGE := 100
const MOVE_INTERVAL_MS := 50

signal show_completed

# Screenshots and tests want a window where it belongs at once.
static var animate := true

var fix_pos := Vector2.ZERO          # m_oFixPos: where the window rests
var _appear_range := Vector2.ZERO    # offset of the starting point from fix_pos
var _move_value := 0                 # 100 = at the start, 0 = home
var _last_move_ms := 0
var _moving := false
var _anchor_ini = null               # the layout and section the window was placed from: it is placed again on a resize
var _anchor_section := ""


# Reads the window's own section and puts it on a screen of `screen` pixels.
func init_window(ini: KUiScheme, section: String, screen: Vector2i) -> bool:
	if not init_from(ini, section):
		return false
	place_on_screen(ini, section, screen)
	fix_pos = position
	_anchor_ini = ini
	_anchor_section = section
	_watch_screen()
	var start: Vector2i = ini.get_integer2(section, "StartPos", Vector2i(int(fix_pos.x), int(fix_pos.y)))
	_appear_range = Vector2(start) - fix_pos
	if _appear_range.x != 0.0:
		_appear_range.x = (start.x + (screen.x - 800) if start.x > 0 else start.x) - fix_pos.x
	if _appear_range.y != 0.0:
		_appear_range.y = (start.y + (screen.y - 600) if start.y > 0 else start.y) - fix_pos.y
	mouse_filter = Control.MOUSE_FILTER_STOP
	visible = false
	return true


# KWndShowAnimate::Show
func show_window() -> void:
	visible = true
	if animate and _appear_range != Vector2.ZERO:
		_move_value = FULL_RANGE
		_moving = true
		_last_move_ms = Time.get_ticks_msec()
		_apply_move()
	else:
		_moving = false
		position = fix_pos
		show_completed.emit()


func hide_window() -> void:
	_moving = false
	visible = false


# the anchor rule (HANDOVER §0 rule 14): the window follows the screen - PositionType of its layout is applied again on
# every viewport resize, so a centred / docked window stays centred / docked whatever the size becomes
func _watch_screen() -> void:
	if is_inside_tree():
		var vp := get_viewport()
		if vp != null and not vp.size_changed.is_connected(_on_screen_resized):
			vp.size_changed.connect(_on_screen_resized)
	elif not tree_entered.is_connected(_watch_screen):
		tree_entered.connect(_watch_screen, CONNECT_ONE_SHOT)


func _on_screen_resized() -> void:
	if _anchor_ini == null or not is_inside_tree():
		return
	var screen := Vector2i(get_viewport().get_visible_rect().size)
	place_on_screen(_anchor_ini, _anchor_section, screen)
	fix_pos = position
	if not _moving:
		position = fix_pos


func _apply_move() -> void:
	position = (fix_pos + _appear_range * (float(_move_value) / FULL_RANGE)).round()


func _process(_delta: float) -> void:
	if not _moving:
		return
	var now := Time.get_ticks_msec()
	while _moving and now - _last_move_ms >= MOVE_INTERVAL_MS:
		_last_move_ms += MOVE_INTERVAL_MS
		_move_value -= MOVE_SPEED
		if _move_value <= 0:
			_move_value = 0
			_moving = false
	_apply_move()
	if not _moving:
		show_completed.emit()
