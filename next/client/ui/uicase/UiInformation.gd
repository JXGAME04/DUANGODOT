# UiInformation - the message box of the game screen (KUiInformation of the 2004 client, Ui\UiCase\UiInformation.cpp;
# gamecl.exe 2.0 loads "%s\提示.ini" at 0x004AC748: [Main] 433 x 120 at (182,180) on 对话条2.spr, [Info] a multi-line
# text, [FirstBtn] and [SecondBtn] two pure-text buttons at y = 91).  KUiInformation::Show(text, first, second, caller,
# param): one button sits at the centre ("Xác nhận" when nothing else is asked - UIMessageBox), two keep their places;
# a click hides the box and tells the caller which button (0 the first, 1 the second - UiSysMsgCentre:
# nSelAction == 0 means "agree").  The team invitations / applications go through it (docs/CLIENT-2.0.md §21).
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndLabeledButton := preload("res://ui/elem/KWndLabeledButton.gd")
const KUiItemView := preload("res://ui/KUiItemView.gd")

const SCHEME := "hop-thoai"

signal answered(index: int, param: Variant)   # 0 = the first button, 1 = the second; param as given to show_box

var _info := KWndText.new()
var _first := KWndLabeledButton.new()
var _second := KWndLabeledButton.new()
var _first_x := 0.0     # m_nOrigFirstBtnXPos
var _centre_x := 0.0    # m_nCentreBtnXPos: half way between the two buttons when only one shows
var _param: Variant = null


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiInformation"
	add_child(_info)
	_info.init_from(ini, "Info")
	add_child(_first)
	_first.init_from(ini, "FirstBtn")
	_first.clicked.connect(func(): _answer(0))
	add_child(_second)
	_second.init_from(ini, "SecondBtn")
	_second.clicked.connect(func(): _answer(1))
	_first_x = _first.position.x
	_centre_x = (_first.position.x + _second.position.x) / 2.0
	return true


# KUiInformation::Show: the text, the labels of the buttons ("" hides the second), what to hand back with the answer
func show_box(text: String, first_label: String = "", second_label: String = "", param: Variant = null) -> void:
	_param = param
	_info.text = text
	_first.set_label(first_label if first_label != "" else KUiItemView.client_string("G_CONFIRM_WORD"))
	if second_label == "":
		_second.visible = false
		_first.position.x = _centre_x
	else:
		_second.visible = true
		_second.set_label(second_label)
		_first.position.x = _first_x
	show_window()


func close_box() -> void:
	hide_window()


func _answer(index: int) -> void:
	if not visible:
		return
	hide_window()
	answered.emit(index, _param)
