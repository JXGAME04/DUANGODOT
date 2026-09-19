# UiTaskNote - the journal "Ký Sự" (KUiTaskNote of the 2004 client, Ui/UiCase/UiTaskNote.cpp + UiTaskDataFile.cpp; the 2.0
# client's classes KUiTaskNote / KUiTaskNote_System / KUiTaskNote_Personal / KUiTaskNote_Item, 0x004D21B2 loads
# "%s\任务记事.ini" and the four page files).  Lua AddNote (the 0x63 packet with the ui id 3: OnScriptAction 0x00601058 ->
# ui message 0x24 GDCNI_MISSION_RECORD -> KUiTaskNote::WakeUp 0x004D2550) adds a SYSTEM record {time, value, text} to the
# page "Ghi chú nhiệm vụ" (KTaskDataFile::InsertSystemRecord), saved with the character (MissionMemory.dat of the user's
# private folder; user://journal_<player id>.json here) whether the window is open or not (0x004D25A9: SaveData when it
# is closed).  The frame 任务记事.ini: [Main] 420x287 at (294,200) 记事界面底图.spr, [Title] "Ký Sự", four check-box page
# buttons 分页1.spr at y = 29: [BulletinBtn] "Cập nhật sự kiện" (7), [SystemBtn] "Ghi chú nhiệm vụ" (109), [PersonalBtn]
# "Nhật ký" (211), [ItemBindBtn] "Mở khóa vật phẩm" (313), [CloseBtn] (392,3).  The pages sit at (4,49) 412x228 on
# 个人游戏记事页面.spr: the system page 任务记事-系统任务分页.ini [List_List] (2,2) 393x200 font 12 colour 218,255,165 selectable
# (SelColor 255,253,122, SelBgColor 150,142,105) + [DeleteBtn] "Xóa" (161,201) (KUiTaskNote_System::OnDelete removes the
# picked record); the personal page 任务记事-个人记事分页.ini [Editor] (0,0) 396x200 MaxLen 2000 multi-line + [SaveBtn] "Lưu"
# (161,201) (KUiTaskNote_Personal::OnSave keeps the text); the bulletin page 任务记事-游戏更新记录.ini [TitleList] + [MessageList]
# and the item-bind page 任务记事-装备绑定.ini [List_List] show nothing here (their client-side files are not exported).
# The window is Switch([[tasknote]]) of the 2.0 client (the command "Player_Recorder", 0x00449B20).  docs/CLIENT-2.0.md §28.
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KWndLabeledButton := preload("res://ui/elem/KWndLabeledButton.gd")
const KWndList := preload("res://ui/elem/KWndList.gd")
const KWndEdit := preload("res://ui/elem/KWndEdit.gd")
const KWndImage := preload("res://ui/elem/KWndImage.gd")
const KUiDialogMath := preload("res://ui/KUiDialogMath.gd")

const SCHEME := "nhat-ky"
const PAGE_SCHEMES := ["nhat-ky-cap-nhat", "nhat-ky-he-thong", "nhat-ky-ca-nhan", "nhat-ky-rang-buoc"]
const PAGE_BUTTONS := ["BulletinBtn", "SystemBtn", "PersonalBtn", "ItemBindBtn"]
const PAGE_BULLETIN := 0
const PAGE_SYSTEM := 1
const PAGE_PERSONAL := 2
const PAGE_ITEMBIND := 3
const PERSONAL_MAX := 2000   # [Editor] MaxLen

signal closed

var page := PAGE_SYSTEM
var records: Array = []      # [{time (unix s), value, text}] - the system records, newest first (InsertSystemRecord)
var personal := ""           # the personal page's text
var _page_buttons: Array = []
var _pages: Array = []       # KWndImage per page (the page background), the widgets under it
var _list := KWndList.new()
var _delete := KWndLabeledButton.new()
var _editor := KWndEdit.new()
var _save := KWndLabeledButton.new()
var _player_id := 0


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiTaskNote"
	var title := KWndText.new()
	add_child(title)
	title.init_from(ini, "Title")
	for i in PAGE_BUTTONS.size():
		var b := KWndLabeledButton.new()
		add_child(b)
		b.init_from(ini, PAGE_BUTTONS[i])
		b.toggled.connect(_on_page_button.bind(i))
		b.clicked.connect(func(): _on_page_button(true, i))
		_page_buttons.append(b)
	var close := KWndButton.new()
	add_child(close)
	close.init_from(ini, "CloseBtn")
	close.clicked.connect(close_window)
	for i in PAGE_SCHEMES.size():
		var pini: KUiScheme = KUiScheme.open(PAGE_SCHEMES[i])
		var bg := KWndImage.new()
		add_child(bg)
		if pini != null:
			bg.init_from(pini, "Main")
		bg.mouse_filter = Control.MOUSE_FILTER_IGNORE
		_pages.append(bg)
		if pini == null:
			continue
		if i == PAGE_SYSTEM:
			bg.add_child(_list)
			_list.init_from(pini, "List_List")
			_list.item_color = pini.get_color("List_List", "MsgColor", _list.item_color)
			_list.item_border_color = pini.get_color("List_List", "MsgBorderColor", Color.BLACK)
			if pini.get_string("List_List", "SelBgColor", "") != "":
				_list.sel_bg_color = pini.get_color("List_List", "SelBgColor", Color.BLACK)
			bg.add_child(_delete)
			_delete.init_from(pini, "DeleteBtn")
			_delete.clicked.connect(_on_delete)
		elif i == PAGE_PERSONAL:
			bg.add_child(_editor)
			_editor.init_from(pini, "Editor")
			bg.add_child(_save)
			_save.init_from(pini, "SaveBtn")
			_save.clicked.connect(_on_save)
	_show_page(PAGE_SYSTEM)
	return true


func open_window() -> void:
	_refresh()
	show_window()


func close_window() -> void:
	if visible:
		hide_window()
		closed.emit()


func toggle_window() -> void:
	if visible:
		close_window()
	else:
		open_window()


# KUiTaskNote::WakeUp: a system record in front of the others, kept with the character at once
func add_system_record(text: String, value: int, now_s: int = -1) -> void:
	if text == "":
		return
	if now_s < 0:
		now_s = int(Time.get_unix_time_from_system())
	records = KUiDialogMath.journal_insert(records, {"time": now_s, "value": value, "text": text})
	save_data()
	if visible:
		_refresh()


func record_count() -> int:
	return records.size()


# ---- the file of the character (KTaskDataFile: MissionMemory.dat -> user://journal_<player id>.json) ----

func bind_player(player_id: int) -> void:
	_player_id = player_id
	load_data()


func _path() -> String:
	return "user://journal_%d.json" % _player_id


func load_data() -> void:
	records = []
	personal = ""
	var path := _path()
	if _player_id == 0 or not FileAccess.file_exists(path):
		return
	var f := FileAccess.open(path, FileAccess.READ)
	if f == null:
		return
	var parsed = JSON.parse_string(f.get_as_text())
	f.close()
	if parsed is Dictionary:
		personal = str(parsed.get("personal", ""))
		for r in parsed.get("system", []):
			if r is Dictionary:
				records.append({"time": int(r.get("time", 0)), "value": int(r.get("value", 0)), "text": str(r.get("text", ""))})


func save_data() -> void:
	if _player_id == 0:
		return
	var f := FileAccess.open(_path(), FileAccess.WRITE)
	if f == null:
		return
	f.store_string(JSON.stringify({"personal": personal, "system": records}))
	f.close()


# ---- the pages ----

func _on_page_button(_checked: bool, index: int) -> void:
	_show_page(index)


func _show_page(index: int) -> void:
	page = index
	for i in _page_buttons.size():
		_page_buttons[i].check(i == index)
	for i in _pages.size():
		_pages[i].visible = i == index
	_refresh()


func _refresh() -> void:
	_list.set_items(KUiDialogMath.journal_lines(records))
	if _editor.get_text() != personal:
		_editor.set_text(personal)


# KUiTaskNote_System::OnDelete: the picked record goes; the pick moves up when it was the last
func _on_delete() -> void:
	var index: int = _list.selected
	if index < 0 or index >= records.size():
		return
	records.remove_at(index)
	save_data()
	_refresh()
	if index >= records.size():
		index -= 1
	_list.set_cur_sel(index)


# KUiTaskNote_Personal::OnSave: the editor's text (at most MaxLen) kept
func _on_save() -> void:
	personal = _editor.get_text().substr(0, PERSONAL_MAX)
	save_data()
