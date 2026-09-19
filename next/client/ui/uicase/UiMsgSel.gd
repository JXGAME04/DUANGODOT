# UiMsgSel - the question and its answers of a npc script (KUiMsgSel of the 2004 client, Ui/UiCase/UiMsgSel.cpp; the
# 2.0 client loads "%s\滚动选择界面.ini" at 0x0051CF66: [Main] 455x287 at (182,130) 玩家...NPC对话框2.spr, [InfoText] the
# sentence (30,24 380x96 font 14 colour 202,230,171), [Select] + [Select_List] the answers (30,136 400x122, 9 lines,
# SelColor 255,253,122, HighLightColor 0,255,0), [Select_Scroll]).  KPlayer::OnScriptAction 0x006004C0 case
# UI_SELECTDIALOG (0x00600AE0) builds KUiQuestionAndAnswer {Question, Answer[i]} from the 0x63 packet; KUiMsgSel::Show
# (2.0 0x0051D0F0) lists the answers, or the one line G_UiMsgSel_0 "Kết thúc đối thoại" when there are none; a click on
# a line (OnClickMsg 0x0051D020) closes the window and sends OperationRequest(GOI_QUESTION_CHOOSE 9, 0, index) ->
# KPlayer::OnSelectFromUI 0x005FC7D0 -> the 0x5f packet {0x5f, int index, 0, 0, 0} when the dialog is the server's
# (g_bUISelIntelActiveWithServer); with no answers nothing is sent (g_bUISelLastSelCount == 0).  docs/CLIENT-2.0.md §24.
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndList := preload("res://ui/elem/KWndList.gd")
const KUiDialogMath := preload("res://ui/KUiDialogMath.gd")

const SCHEME := "hop-thoai-chon"

signal chosen(index: int)   # an answer was clicked (its index in the packet)
signal dismissed            # the window closed without an answer (no answers, or Esc)

var _info := KWndText.new()
var _list := KWndList.new()
var _has_answers := false
var _answers: Array = []


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiMsgSel"
	add_child(_info)
	_info.init_from(ini, "InfoText")
	add_child(_list)
	_list.init_from(ini, "Select_List")
	# [Select_List] is a KWndMessageListBox: its keys are MsgColor / MsgBorderColor / SelBgColor (KWndList reads Color / SelItemBgColor)
	_list.item_color = ini.get_color("Select_List", "MsgColor", _list.item_color)
	_list.item_border_color = ini.get_color("Select_List", "MsgBorderColor", Color.BLACK)
	if ini.get_string("Select_List", "SelBgColor", "") != "":
		_list.sel_bg_color = ini.get_color("Select_List", "SelBgColor", Color.BLACK)
	_list.item_selected.connect(_on_click)
	_list.item_activated.connect(_on_click)
	return true


func open_dialog(text: String, answers: Array) -> void:
	_answers = answers.duplicate()
	_has_answers = not answers.is_empty()
	_info.set_text(text)
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
