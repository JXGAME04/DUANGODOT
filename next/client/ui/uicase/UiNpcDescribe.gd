# UiNpcDescribe - the description a npc script shows with Describe (ui 12 of the 0x63 packet).  No 2004 source has it: the
# 2.0 client's KPlayer::OnScriptAction 0x006004C0 case 12 (0x006007FD) builds the same {text[0x400], len, count, param,
# options[n] of 0x104} as Say's case 0, splits the content at "| |" (0x7a740c), and raises the ui message 0x40
# (0x00600ACC) -> the notify handler 0x00428970 case 0x40 (0x00429664) -> 0x00508410 OpenWindow of the window
# 0x8c0320 (0x4dcc bytes; Init 0x00508090; 0x00507A87 loads "%s\npc描述界面.ini"): [Main] 573x287 at (247,220)
# NPC对话框3.spr (PositionType 1, moveable), [Image] 64x82 at (42,34) + [Text] 64x82 at (42,28) green 16 centred (the
# npc's portrait and name), [MessageList] the description (136,17 378x136 font 14 MsgColor 202,255,202, 20 messages of
# 40 chars) with [Msg_Scroll], [Select_List] the answers (24,155 525x128 font 12, 5 lines, MsgColor 202,230,171 SelColor
# 255,253,122 HighLightColor 0,255,0 SelBgColor 140,121,99) with [Select_Scroll].  The WndProc 0x00508510 takes the
# list's click (0x5c9 -> 0x00507C50) and the answer goes back the way Say's does: OperationRequest GOI_QUESTION_CHOOSE
# (0x005C1C00 case 9, 0x005C2D70) -> KPlayer::OnSelectFromUI 0x005FC7D0 -> the 0x5f packet {index, kind 0}.
# docs/CLIENT-2.0.md §26.
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndList := preload("res://ui/elem/KWndList.gd")
const KWndImage := preload("res://ui/elem/KWndImage.gd")
const KUiDialogMath := preload("res://ui/KUiDialogMath.gd")

const SCHEME := "mo-ta-npc"

signal chosen(index: int)   # an answer was clicked (its index in the packet)
signal dismissed            # the window closed without an answer

var _portrait := KWndImage.new()
var _name := KWndText.new()
var _message := KWndText.new()
var _list := KWndList.new()
var _has_answers := false
var _answers: Array = []


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiNpcDescribe"
	add_child(_portrait)
	_portrait.init_from(ini, "Image")
	add_child(_name)
	_name.init_from(ini, "Text")
	add_child(_message)
	# [MessageList] is a KWndMessageListBox: its lines are MsgLineCount chars of Font (a KWndText wraps the same width)
	_message.init_from(ini, "MessageList")
	_message.multi_line = true
	_message.text_color = ini.get_color("MessageList", "MsgColor", Color(202.0 / 255.0, 1.0, 202.0 / 255.0))
	add_child(_list)
	_list.init_from(ini, "Select_List")
	_list.item_color = ini.get_color("Select_List", "MsgColor", _list.item_color)
	_list.item_border_color = ini.get_color("Select_List", "MsgBorderColor", Color.BLACK)
	if ini.get_string("Select_List", "SelBgColor", "") != "":
		_list.sel_bg_color = ini.get_color("Select_List", "SelBgColor", Color.BLACK)
	_list.item_selected.connect(_on_click)
	_list.item_activated.connect(_on_click)
	return true


# the description and its answers; `npc_name` under the portrait ([Text]) - the 2.0 client fills it from the npc it talks to
func open_dialog(text: String, answers: Array, npc_name: String = "") -> void:
	_answers = answers.duplicate()
	_has_answers = not answers.is_empty()
	_message.set_text(text)
	_name.set_text(npc_name)
	var strings = load("res://ui/KUiItemView.gd")   # loaded here: it needs the Assets autoload the headless tests run without
	_list.set_items(KUiDialogMath.lines_for(answers, strings.client_string("G_UiMsgSel_0")))
	_list.set_cur_sel(-1)
	show_window()


func close_dialog() -> void:
	if visible:
		hide_window()
		dismissed.emit()


func answer_count() -> int:
	return _answers.size()


func _on_click(index: int) -> void:
	if not visible:
		return
	hide_window()
	if _has_answers and index >= 0 and index < _answers.size():
		chosen.emit(index)
	else:
		dismissed.emit()
