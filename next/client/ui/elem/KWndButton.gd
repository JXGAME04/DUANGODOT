# KWndButton - a button drawn from the frames of one sprite (Ui\Elem\WndButton.cpp).
#
#   Up=, Down=     the frame at rest and the frame while held (defaults 0 and 1)
#   Over=1         the button answers to the mouse being over it; `Over` is ONLY this switch
#   OverFrame=     where the over animation starts (default 2): while the mouse is over, the button
#                  plays the sprite from OverFrame to its last frame, again and again
#   CheckBox=1     a two-state button: a press flips it, there is no "held" look
#   CheckOver=     the frame of a checked box under the mouse
#   DisableFrame=  the frame of a disabled button (-1 = it keeps its look)
#   Tip=           the text shown after resting the mouse on it
#
# A plain button clicks on RELEASE, and only when the mouse did not leave it in between; a check
# box clicks on PRESS.  WNDBTN_F_CHECKED is the same bit as WNDBTN_F_DOWN in the old code, so there
# is one flag here as well.
extends "res://ui/elem/KWndImage.gd"

signal clicked                      # WND_N_BUTTON_CLICK of a plain button
signal toggled(checked: bool)       # WND_N_BUTTON_CLICK of a check box
signal hovered                      # WND_N_BUTTON_OVER
signal double_clicked               # WND_N_BUTTON_DB_CLICK

var up_frame := 0
var down_frame := 1
var over_start_frame := 2
var check_over_frame := 0
var disable_frame := -1
var animate_over := false           # WNDBTN_ES_ANIMATION
var is_checkbox := false            # WNDBTN_ES_CHECKBOX
var tip := ""

var down := false                   # WNDBTN_F_DOWN / WNDBTN_F_CHECKED
var over := false                   # WNDBTN_F_OVER
var _pressed_here := false          # m_pPressedDownBtn == this


func init_from(ini: KUiScheme, section: String) -> bool:
	if not super.init_from(ini, section):
		return false
	up_frame = ini.get_integer(section, "Up", 0)
	down_frame = ini.get_integer(section, "Down", 1)
	disable_frame = ini.get_integer(section, "DisableFrame", -1)
	animate_over = ini.get_bool(section, "Over", false)
	if animate_over:
		over_start_frame = ini.get_integer(section, "OverFrame", 2)
	is_checkbox = ini.get_bool(section, "CheckBox", false)
	if is_checkbox:
		check_over_frame = ini.get_integer(section, "CheckOver", 0)
	tip = ini.get_string(section, "Tip", "")
	tooltip_text = tip
	mouse_filter = Control.MOUSE_FILTER_STOP
	_show_rest_frame()
	return true


func is_checked() -> bool:
	return down


# KWndButton::CheckButton
func check(on: bool) -> void:
	down = on
	set_frame(down_frame if on else up_frame)


func enable(on: bool) -> void:
	super.enable(on)
	if not on:
		over = false
		_pressed_here = false
	mouse_default_cursor_shape = Control.CURSOR_ARROW
	if disable_frame >= 0:
		set_frame((down_frame if down else up_frame) if on else disable_frame)
	queue_redraw()


func _show_rest_frame() -> void:
	if disabled and disable_frame >= 0:
		set_frame(disable_frame)
	else:
		set_frame(down_frame if down else up_frame)


func _process(_delta: float) -> void:
	# KWndButton::PaintWindow: a plain button under the mouse plays on from OverFrame
	if over and animate_over and not is_checkbox and not down:
		next_frame()
		if frame < over_start_frame:
			set_frame(over_start_frame)


func _gui_input(event: InputEvent) -> void:
	if disabled:
		return
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT:
		if event.pressed:
			_on_left_down()
			if event.double_click and not is_checkbox:
				double_clicked.emit()
		else:
			_on_left_up()
		accept_event()
	elif event is InputEventMouseMotion:
		var inside := _has_point(event.position)
		if inside and _pressed_here and not down and (event.button_mask & MOUSE_BUTTON_MASK_LEFT) != 0:
			down = true                     # OnLBtnDownMove: back over the button it was pressed on
			set_frame(down_frame)
		if inside:
			_mouse_over()
		else:
			_mouse_leave()


func _notification(what: int) -> void:
	if what == NOTIFICATION_MOUSE_EXIT:
		_mouse_leave()


func _mouse_over() -> void:
	if over or _pressed_here:
		return
	over = true
	hovered.emit()
	if animate_over:
		if not down:
			set_frame(over_start_frame)
		elif is_checkbox:
			set_frame(check_over_frame)
	queue_redraw()


func _mouse_leave() -> void:
	if not over and not (_pressed_here and down):
		return
	over = false
	if _pressed_here and down:
		down = false
		set_frame(up_frame)
	elif animate_over:
		set_frame(down_frame if down else up_frame)
	queue_redraw()


func _on_left_down() -> void:
	over = false
	if not is_checkbox:
		_pressed_here = true
		down = true
		set_frame(down_frame)
		return
	_pressed_here = false
	if down:
		down = false
		set_frame(over_start_frame if animate_over else up_frame)
	else:
		down = true
		set_frame(check_over_frame if animate_over else down_frame)
	queue_redraw()
	toggled.emit(down)


func _on_left_up() -> void:
	var fire := _pressed_here and down
	_pressed_here = false
	if fire:
		down = false
		set_frame(up_frame)
		clicked.emit()
