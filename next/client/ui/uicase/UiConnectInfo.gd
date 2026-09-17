# KUiConnectInfo - the small box that says what the login flow is waiting for, or why it failed
# (Ui\UiCase\UiConnectInfo.cpp; layout <theme>\UiNewLogin\登陆过程提示.ini).
#
# The text is line <index> of [InfoString] in \Ui\Setting.ini - the game's own words ("Hiện đang kết
# nối với máy chủ", "Kết nối máy chủ thất bại. Xin kiểm tra lại đường truyền.") - drawn in one line,
# centred on [Message] Pos/Size, in MsgColor with ONE letter in MsgColor2 that runs along the text,
# a step every ColorChangeInterval milliseconds (KUiConnectInfo::PaintWindow).
#
# While something is still going on (index 11, entering the game) the box has no button; otherwise
# it has [ConfirmBtn] ("Quay lại"), and pressing it - or Enter / Escape - goes where `next_step`
# says: CI_NS_SEL_SERVER, CI_NS_LOGIN_WND, CI_NS_SEL_ROLE_WND, CI_NS_NEW_ROLE_WND...
#
# The 2.0 layout of the character window has no delete button, so the second-password part of the
# old box (DelRole, CancelDelRole, Password) is not built.
extends "res://ui/elem/KWndImage.gd"

const KFont := preload("res://ui/KFont.gd")
const KWndButton := preload("res://ui/elem/KWndButton.gd")

const SCHEME := "thong-bao-ket-noi"

# LOGIN_BG_INFO_MSG_INDEX
const CI_MI_CONNECTING := 1
const CI_MI_CONNECT_FAILED := 2
const CI_MI_CONNECT_SERV_BUSY := 3
const CI_MI_CONNECT_TIMEOUT := 4
const CI_MI_ACCOUNT_PWD_ERROR := 5
const CI_MI_ACCOUNT_LOCKED := 6
const CI_MI_ERROR_ROLE_NAME := 7
const CI_MI_CREATING_ROLE := 8
const CI_MI_GETTING_ROLE_DATA := 10
const CI_MI_ENTERING_GAME := 11
const CI_MI_SVRDOWN := 12
const CI_MI_INVALID_PROTOCOLVERSION := 14
const CI_MI_ERROR_LOGIN_INPUT := 15
const CI_MI_INVALID_LOGIN_INPUT1 := 17
const CI_MI_INVALID_LOGIN_INPUT2 := 18
const CI_MI_NOT_ENOUGH_ACCOUNT_POINT := 19
const CI_MI_ACCOUNT_FREEZE := 30

# LOGIN_BG_NEXT_STEP
enum { CI_NS_NONE, CI_NS_INIT_WND, CI_NS_SEL_SERVER, CI_NS_LOGIN_WND, CI_NS_SEL_ROLE_WND, CI_NS_NEW_ROLE_WND, CI_NS_EXIT_PROGRAM }

signal confirmed(next_step: int)

var next_step := CI_NS_NONE
var message := ""

var _confirm := KWndButton.new()
var _font_size := 12
var _centre := Vector2i.ZERO
var _color := Color.BLACK
var _border := Color.BLACK
var _color2 := Color.BLACK
var _border2 := Color.BLACK
var _interval_ms := 0
var _lit := 0                        # the letter in the second colour
var _lit_at := 0
var _strings: Dictionary = {}


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_from(ini, "RuningImgBg"):
		return false
	place_on_screen(ini, "RuningImgBg", screen)
	name = "UiConnectInfo"
	mouse_filter = Control.MOUSE_FILTER_STOP
	focus_mode = Control.FOCUS_ALL
	add_child(_confirm)
	_confirm.init_from(ini, "ConfirmBtn")
	_confirm.clicked.connect(_on_confirm)
	_color = ini.get_color("Message", "MsgColor", Color.BLACK)
	_border = ini.get_color("Message", "MsgBorderColor", Color.BLACK)
	_color2 = ini.get_color("Message", "MsgColor2", Color.BLACK)
	_border2 = ini.get_color("Message", "MsgBorderColor2", Color.BLACK)
	_interval_ms = ini.get_integer("Message", "ColorChangeInterval", 0)
	_font_size = ini.get_integer("Message", "Font", 12)
	var pos: Vector2i = ini.get_integer2("Message", "Pos")
	var box: Vector2i = ini.get_integer2("Message", "Size")
	_centre = Vector2i(pos.x + box.x / 2, pos.y)
	var data = Assets.ui_data("thong-diep")
	if data != null:
		for sec in data.get("sections", []):
			if str(sec.get("name", "")).to_lower() == "infostring":
				_strings = sec.get("values", {})
	visible = false
	return true


# The game's own sentence for an index, "" when it has none.
func info_string(index: int) -> String:
	return str(_strings.get(str(index), ""))


# KUiConnectInfo::OpenWindow.  `text` replaces the table's sentence (a detail from our gateway).
func open(index: int, then: int, text: String = "") -> void:
	next_step = then
	set_info_msg(index, text)
	visible = true
	grab_focus()


func close() -> void:
	visible = false


# KUiConnectInfo::SetInfoMsg
func set_info_msg(index: int, text: String = "") -> void:
	message = text if text != "" else info_string(index)
	_lit = 0
	_lit_at = Time.get_ticks_msec()
	# every sentence but "entering the game" comes with the button; while the flow is still waiting
	# (next_step CI_NS_NONE) the button gives up and goes back to the server list, as the old
	# OnClickConfirmBtn did in its default case
	_confirm.visible = index != CI_MI_ENTERING_GAME
	Log.info("ui", "connect info", {"index": index, "text": message, "next": next_step})
	queue_redraw()


func _on_confirm() -> void:
	if not _confirm.visible:
		return
	visible = false
	confirmed.emit(next_step)


func _gui_input(event: InputEvent) -> void:
	if event is InputEventKey and event.pressed and (event.keycode == KEY_ENTER or event.keycode == KEY_KP_ENTER or event.keycode == KEY_ESCAPE):
		_on_confirm()
		accept_event()


func _process(_delta: float) -> void:
	if not visible or message == "" or _interval_ms <= 0:
		return
	var now := Time.get_ticks_msec()
	if now - _lit_at >= _interval_ms:
		_lit_at = now
		_lit = _lit + 1 if _lit + 1 < message.length() else 0
		queue_redraw()


func _draw() -> void:
	super._draw()
	if message == "":
		return
	var font = KFont.of(_font_size)
	var pitch := _font_size / 2
	var x := _centre.x - message.length() * _font_size / 4
	var y := _centre.y
	if font == null:
		draw_string(get_theme_default_font(), Vector2(x, y + _font_size), message, HORIZONTAL_ALIGNMENT_LEFT, -1, _font_size, _color2)
		return
	if _lit > 0:
		font.draw(self, Vector2(x, y), message.substr(0, _lit), _color, _border)
	font.draw(self, Vector2(x + _lit * pitch, y), message.substr(_lit, 1), _color2, _border2)
	if _lit + 1 < message.length():
		font.draw(self, Vector2(x + (_lit + 1) * pitch, y), message.substr(_lit + 1), _color, _border)
