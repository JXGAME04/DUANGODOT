# KUiInit - the first menu: Bắt Đầu Trò Chơi / Tùy Chọn Hệ Thống / Xem Ghi Hình / Thoát Khỏi Trò Chơi
# (Ui\UiCase\UiInit.cpp; layout <theme>\UiNewLogin\开始.ini).
#
# The window covers the screen and has no picture of its own: four buttons in the middle and the
# copyright plate at the bottom ([KingSoft], a button that does nothing).  Up / Down move between
# the buttons, Enter presses the one the keys are on, Escape leaves the game - as in the old window.
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndButton := preload("res://ui/elem/KWndButton.gd")

const SCHEME := "bat-dau"
const BUTTONS := ["EnterGame", "GameConfig", "OpenRep", "ExitGame"]

signal enter_game
signal game_config
signal open_rep
signal exit_game

var login_bg := "Login"
var _buttons: Array = []
var _plate := KWndButton.new()
var _key_on := -1


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiInit"
	login_bg = ini.get_string("Main", "LoginBg", "Login")
	for i in BUTTONS.size():
		var b := KWndButton.new()
		add_child(b)
		b.init_from(ini, BUTTONS[i])
		b.clicked.connect(_on_button.bind(i))
		_buttons.append(b)
	add_child(_plate)
	_plate.init_from(ini, "KingSoft")
	_plate.mouse_filter = Control.MOUSE_FILTER_IGNORE
	mouse_filter = Control.MOUSE_FILTER_PASS
	focus_mode = Control.FOCUS_ALL
	return true


func open() -> void:
	_key_on = -1
	show_window()
	grab_focus()


func _on_button(index: int) -> void:
	match index:
		0:
			hide_window()
			enter_game.emit()
		1:
			game_config.emit()
		2:
			open_rep.emit()
		3:
			exit_game.emit()


func _gui_input(event: InputEvent) -> void:
	if not (event is InputEventKey) or not event.pressed:
		return
	match event.keycode:
		KEY_UP:
			_move_key(-1)
		KEY_DOWN:
			_move_key(1)
		KEY_ENTER, KEY_KP_ENTER:
			_on_button(maxi(_key_on, 0))
		KEY_ESCAPE:
			_on_button(3)
		_:
			return
	accept_event()


# The button the keys are on shows its "over" look, like a button under the mouse.
func _move_key(by: int) -> void:
	if _key_on >= 0:
		_buttons[_key_on]._mouse_leave()
	_key_on = posmod(_key_on + by, _buttons.size()) if _key_on >= 0 else (0 if by > 0 else _buttons.size() - 1)
	_buttons[_key_on]._mouse_over()
