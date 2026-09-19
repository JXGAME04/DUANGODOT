# UiInformation2 - the pages a npc script speaks (Talk): KUiInformation2 of the 2004 client (Ui/UiCase/UiInformation2.cpp,
# g_UiInformation2.SpeakWords); the 2.0 client loads 提示2.ini ([Main] 455x175 at (182,180) NPC对话框1.spr, [Info] the
# page (27,24 410x75 font 14 colour 202,230,171), [OK] a text button (27,91 377x13, OverColor 255,247,112, SelColor
# 255,130,47)).  KPlayer::OnScriptAction 0x006004C0 case UI_TALKDIALOG (0x00600D64) splits the 0x63 content at "| |"
# into KUiInformationParam pages of 0x244 bytes; every page but the last gets the label G_PLAYER_14 "Tiếp tục", the last
# G_PLAYER_15 "Hoàn thành" (0x00600EB2 / 0x00600EC8), and when the packet's m_nParam is 1 the last page's confirm
# notifies the game (bNeedConfirmNotify, 0x00600EDE -> GOI_INFORMATION_CONFIRM_NOTIFY -> OnSelectFromUI(0, UI_TALKDIALOG)
# -> the 0x5f packet with index 0).  A numbered page (m_bParam1) is g_GetStringRes(atoi(page)).  SpeakWords appends new
# pages to the ones not yet shown.  docs/CLIENT-2.0.md §24.
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndLabeledButton := preload("res://ui/elem/KWndLabeledButton.gd")
const KUiDialogMath := preload("res://ui/KUiDialogMath.gd")

const SCHEME := "hop-thoai-mot-nut"

signal confirmed      # the last page of a Talk with m_nParam 1 was confirmed: the 0x5f answer 0 goes to the zone
signal closed         # the last page went away (with or without a notify)

var _info := KWndText.new()
var _ok := KWndLabeledButton.new()
var _pages: Array = []      # the pages still to show, the current one first
var _notify := false        # bNeedConfirmNotify of the last page


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiInformation2"
	add_child(_info)
	_info.init_from(ini, "Info")
	add_child(_ok)
	_ok.init_from(ini, "OK")
	_ok.full_text = true
	_ok.clicked.connect(_on_ok)
	return true


# KUiInformation2::SpeakWords: the pages after the ones still waiting; the first shows at once when the window is closed
func speak_words(pages: Array, notify: bool) -> void:
	if pages.is_empty():
		return
	for p in pages:
		_pages.append(str(p))
	_notify = notify
	if not visible:
		_show_page()


func _show_page() -> void:
	if _pages.is_empty():
		return
	var count := _pages.size()
	_info.set_text(str(_pages[0]))
	var strings = load("res://ui/KUiItemView.gd")   # loaded here: it needs the Assets autoload the headless tests run without
	_ok.set_label(KUiDialogMath.page_label(0, count, strings.core_string("G_PLAYER_14"), strings.core_string("G_PLAYER_15")))
	show_window()


func pages_left() -> int:
	return _pages.size()


func close_pages() -> void:
	_pages.clear()
	if visible:
		hide_window()
		closed.emit()


func _on_ok() -> void:
	if not visible:
		return
	if not _pages.is_empty():
		_pages.pop_front()
	if _pages.is_empty():
		hide_window()
		if _notify:
			_notify = false
			confirmed.emit()
		closed.emit()
	else:
		_show_page()
