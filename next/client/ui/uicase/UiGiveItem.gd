# UiGiveItem - the box a npc script asks the player to put pieces into (Lua GiveItemUI): KUiGiveItem of the 2.0 client
# (singleton 0x8c0354, 0xb7cc bytes; OpenWindow 0x00519C80, ctor 0x00519A50 vtable 0x799134, Init 0x00519710, 0x00519020
# loads "%s\给予界面.ini"), reached from KPlayer::OnScriptAction case 11 (0x0060194C: the content at +0x11 to its NUL, the
# title 0x20 bytes after it, +9 the parameter, +8 the "report changes" flag) through the ui message 0x3e (0x00429EAD).
# Layout: [Main] 223x325 at (294,116) 给予底图.spr PositionType 1; [LabelTitle] "Giao Nộp" (5,7) 195x16 font 16 white centred;
# [Title] (16,36) 190x17 font 12 colour 201,215,162 centred; [ContentList] (11,59) 188x96 font 12 colour 255,217,78 (40 chars
# a line) + [ContentScroll]; [Items] (27,160) 170x112 a 6 x 4 box (HUnits 6, VUnits 4, UnitBorder 2); [Assemble] "Đồng ý"
# (20,292) 74x26 大按钮二字.spr; [Close] "Hủy bỏ" (128,292); [CloseBtn] (198,2) 关闭.spr.
# The pieces stay in the bag: the box only remembers them (the core keeps the list, GetGameData 0x7da at 0x00519390) and
# "Đồng ý" reports every piece's place and cell in the 0x89 packet ({room, x, y, cell x, cell y}, 0x080ABA20 on the
# server); "Hủy bỏ" / the close button end the window (OperationRequest 0x5d with the flag, 0x00518FE6) and answer the
# dialog's second function (the cancel one).  SetUiGiveItemMoreConfirmMsg (the 0xdf packet -> ui message 0xa8) asks once
# more before the pieces go: here the sentence takes the content's place and "Đồng ý" is pressed again.
# docs/CLIENT-2.0.md §27.
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KWndLabeledButton := preload("res://ui/elem/KWndLabeledButton.gd")
const KWndObjContainer := preload("res://ui/elem/KWndObjContainer.gd")
const KUiItemView := preload("res://ui/KUiItemView.gd")
const KUiDialogMath := preload("res://ui/KUiDialogMath.gd")

const SCHEME := "dua-vat-pham"
const BOX_W := 6
const BOX_H := 4
const ROOM_BAG := 0

signal confirmed(entries: Array)      # "Đồng ý": [{room, x, y, cell_x, cell_y}] of the pieces in the box
signal changed(entries: Array)        # a piece put in or taken out while the script asked to be told (notify_changes)
signal cancelled                      # "Hủy bỏ" / the close button: the second answer of the dialog (the cancel function)
signal cell_clicked(x: int, y: int)   # a click on the box with something on the cursor: the top-left cell it would take
signal item_hovered(item)

var _label := KWndText.new()
var _title := KWndText.new()
var _content := KWndText.new()
var _box := KWndObjContainer.new()
var _ok := KWndLabeledButton.new()
var _cancel := KWndLabeledButton.new()
var _close := KWndButton.new()
var _placed: Array = []      # [{id, x, y}]: the piece and the cell it was dropped on
var _notify := false
var _param := 0
var _more_confirm := ""      # SetUiGiveItemMoreConfirmMsg: asked before the pieces go
var _asked := false


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiGiveItem"
	add_child(_label)
	_label.init_from(ini, "LabelTitle")
	add_child(_title)
	_title.init_from(ini, "Title")
	add_child(_content)
	_content.init_from(ini, "ContentList")   # a KWndMessageListBox: MsgColor, MsgLineCount chars a line
	_content.multi_line = true
	_content.text_color = ini.get_color("ContentList", "MsgColor", _content.text_color)
	add_child(_box)
	_box.init_from(ini, "Items")
	_box.object_clicked.connect(_on_box_clicked)
	_box.put_requested.connect(func(x: int, y: int): cell_clicked.emit(x, y))
	_box.hovered.connect(func(o): item_hovered.emit(o))
	for pair in [[_ok, "Assemble"], [_cancel, "Close"]]:
		add_child(pair[0])
		pair[0].init_from(ini, pair[1])
	_ok.clicked.connect(_on_ok)
	_cancel.clicked.connect(_on_cancel)
	add_child(_close)
	_close.init_from(ini, "CloseBtn")
	_close.clicked.connect(_on_cancel)
	return true


# KPlayer::OnScriptAction case 11: the content and the title of the packet, whether changes are reported
func open_box(content: String, title: String, notify: bool, param: int = 0) -> void:
	_placed.clear()
	_notify = notify
	_param = param
	_more_confirm = ""
	_asked = false
	_content.set_text(content)
	_title.set_text(title)
	_refresh()
	show_window()


func close_box() -> void:
	if visible:
		hide_window()


func set_hand(cells: Vector2i) -> void:
	_box.hand_size = cells


# SetUiGiveItemMsg (kind 0): the hint takes the content line; SetUiGiveItemMoreConfirmMsg (kind 1): asked before the pieces go
func set_message(kind: int, text: String) -> void:
	if kind == 0:
		_content.set_text(text)
	else:
		_more_confirm = text
		_asked = false


# a bag piece dropped on a cell: taken when the cell is inside, the piece not in the box yet and its cells free
func put_item(item: Dictionary, cell: Vector2i) -> bool:
	if not visible or int(item.get("room", -1)) != ROOM_BAG:
		return false
	var obj: Dictionary = KUiItemView.object_of(item)
	if KUiDialogMath.give_box_place(_placed_objects(), cell, int(obj.w), int(obj.h), BOX_W, BOX_H) == null:
		return false
	for p in _placed:
		if int(p.id) == int(obj.id):
			return false
	_placed.append({"id": int(obj.id), "x": cell.x, "y": cell.y})
	_refresh()
	if _notify:
		changed.emit(entries())
	return true


func placed_count() -> int:
	return _placed.size()


# the 0x89 list: where each piece lies in the bag and the cell it sits on in the box
func entries() -> Array:
	return KUiDialogMath.give_entries(_placed, Game.items)


func _placed_objects() -> Array:
	var out: Array = []
	for p in _placed:
		var it = Game.items.get(int(p.id))
		if it == null:
			continue
		var obj: Dictionary = KUiItemView.object_of(it)
		obj.x = int(p.x)
		obj.y = int(p.y)
		out.append(obj)
	return out


func _refresh() -> void:
	_box.set_objects(_placed_objects())


# a click on a piece in the box takes it out again
func _on_box_clicked(o: Dictionary) -> void:
	for i in _placed.size():
		if int(_placed[i].id) == int(o.id):
			_placed.remove_at(i)
			break
	_refresh()
	if _notify:
		changed.emit(entries())


func _on_ok() -> void:
	if not visible:
		return
	if _more_confirm != "" and not _asked:   # the sentence of SetUiGiveItemMoreConfirmMsg, then "Đồng ý" once more
		_asked = true
		_content.set_text(_more_confirm)
		return
	var e := entries()
	hide_window()
	confirmed.emit(e)


func _on_cancel() -> void:
	if not visible:
		return
	hide_window()
	cancelled.emit()
