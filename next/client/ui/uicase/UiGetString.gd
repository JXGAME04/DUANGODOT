# UiGetString - the box a script asks a number or a string through (Lua AskClientForNumber / AskClientForString):
# KUiGetString of the 2.0 client (RTTI .?AVKUiGetString@@ 0x8120dc, vtable 0x7994b4; the window 0x8c038c of 0x2530 bytes,
# ctor 0x0051CB70, Init 0x0051CA10, OpenWindow 0x0051CD00), reached from the 0xa3 packet (sub-command 2: 0x006AC330 -> ui
# message 0x37 -> 0x00429DFF).  Init 0x0051CA10 adds FOUR children - the title +0x55c, the edit box +0xafc, "Đồng ý" +0x11e8,
# "Hủy bỏ" +0x1b88 - and 0x0051C690 loads them from "%s\输入字串界面.ini" as [Main] [Title] [StringInput] [OkBtn] [CancelBtn]:
# the SAME box serves a number and a string (数字键盘.ini / KUiNumberPad is another window, not this flow).
#   OpenWindow(title, default | NULL, callback 0x8468b0, mode 2 + (kind != 0), min, max, 1, kind) 0x0051CD00:
#     +0x558 = kind, +0xaf4 = min, +0xaf8 = max, +0x2528 / +0x252c = callback / mode; the title into +0x55c (0x00468460);
#     kind == 0: the edit's longest text = max (+0xfa0, 0x0051CE13), the default text or nothing (0x00456770 / 0x00456820);
#     kind != 0: Init(kind) made the edit numeric - at most 9 characters (+0xfa0 = 9, 0x0051CA8D), plain mode (0x00455E00) -
#     and the box opens showing the MIN as its number (0x00456890(min), 0x0051CE44).
#   "Đồng ý" 0x0051C800: kind != 0 -> value = the digits as a number (0x004571F0); value < min -> the message box
#     G_UiGetString_0 (0x0051C831, stays open), else callback(0x501, mode 3, value) -> the 0x82 packet {0x82, 7, 3, int}
#     (0x006AC510) and the box closes (0x0051C610).  No check against the max for a number.
#     kind == 0 -> the text (0x00456910, <= 0x1ff); shorter than min -> G_UiGetString_1, longer than max -> G_UiGetString_2
#     (0x0051C8B6 / 0x0051C8BA, stays open), else callback(0x501, mode 2, text) -> {0x82, len + 5, 2, word len, text}
#     (0x006AC470).  The messages come from \lang\vn\stringtable_client.txt (KUiDialogMath.ask_reject_text).
#   "Hủy bỏ" only closes; the server keeps waiting (no packet).
# Layout 输入字串界面.ini: [Main] 154x119 输入字符串底1.spr at (0,0) - the 2.0 client places the box itself (0x0046BE90 before
# the show); centred on the screen here (rule 14: placed again on a resize by KWndShowAnimate); [Title] (15,7) 125x14 font 12
# colour 255,252,178 centred, [StringInput] (10,49) 134x14 font 12 colour 255,253,122 MaxLen 500 FocusBKColor 25,31,11/180,
# [OkBtn] "Đồng ý" (20,90) 46x19 小按钮二字.spr, [CancelBtn] "Hủy bỏ" (90,90).  docs/CLIENT-2.0.md §29.
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndEdit := preload("res://ui/elem/KWndEdit.gd")
const KWndLabeledButton := preload("res://ui/elem/KWndLabeledButton.gd")
const KUiDialogMath := preload("res://ui/KUiDialogMath.gd")

const SCHEME := "nhap-chuoi"
const NUMBER_DIGITS := 9   # +0xfa0 = 9 of Init 0x0051CA10 (0x0051CA8D)

signal confirmed_number(value: int)   # "Đồng ý" of a number ask: the value (>= min) - the 0x82 packet of kind 3
signal confirmed_text(text: String)   # "Đồng ý" of a string ask: the text (min..max characters) - the 0x82 packet of kind 2
signal rejected(reason: int)          # "Đồng ý" refused, the box stays: KUiDialogMath.ASK_TOO_SMALL / TOO_SHORT / TOO_LONG
signal cancelled

var _title := KWndText.new()
var _input := KWndEdit.new()
var _ok := KWndLabeledButton.new()
var _cancel := KWndLabeledButton.new()
var _kind := 0     # +0x558: 0 a string, else a number
var _min := 0      # +0xaf4
var _max := 0      # +0xaf8


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiGetString"
	# the layout sits at (0,0): the 2.0 client places the box itself - the middle of the screen here
	fix_pos = Vector2(float(screen.x - int(size.x)) / 2.0, float(screen.y - int(size.y)) / 2.0)
	position = fix_pos
	add_child(_title)
	_title.init_from(ini, "Title")
	add_child(_input)
	_input.init_from(ini, "StringInput")
	_input.submitted.connect(func(_t: String): _on_ok())
	for pair in [[_ok, "OkBtn"], [_cancel, "CancelBtn"]]:
		add_child(pair[0])
		pair[0].init_from(ini, pair[1])
	_ok.clicked.connect(_on_ok)
	_cancel.clicked.connect(_on_cancel)
	return true


# the 0xa3 packet: OpenWindow 0x0051CD00 - a number (kind != 0) shows the min in a 9-digit numeric box, a string (kind 0)
# shows the default text in a box of at most `max_value` characters
func open_box(title: String, default_text: String, min_value: int, max_value: int, numeric: bool) -> void:
	_kind = 1 if numeric else 0
	_min = min_value
	_max = max_value
	_title.set_text(title)
	if numeric:
		_input.set_limits(NUMBER_DIGITS, true)
		_input.set_text(str(min_value))
	else:
		_input.set_limits(max_value, false)
		_input.set_text(default_text)
	show_window()
	_input.take_focus()


func close_box() -> void:
	if visible:
		hide_window()


func is_numeric() -> bool:
	return _kind != 0


func input_text() -> String:
	return _input.get_text()


func set_input(text: String) -> void:
	_input.set_text(text)


# "Đồng ý" 0x0051C800
func _on_ok() -> void:
	if not visible:
		return
	var text := _input.get_text()
	if _kind != 0:
		var value := KUiDialogMath.ask_number(text)
		var why := KUiDialogMath.ask_check_number(value, _min)
		if why >= 0:
			rejected.emit(why)
			return
		hide_window()
		confirmed_number.emit(value)
		return
	var reason := KUiDialogMath.ask_check_text(text.length(), _min, _max)
	if reason >= 0:
		rejected.emit(reason)
		return
	hide_window()
	confirmed_text.emit(text)


func _on_cancel() -> void:
	if not visible:
		return
	hide_window()
	cancelled.emit()
