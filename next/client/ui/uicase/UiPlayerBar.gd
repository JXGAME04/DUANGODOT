# UiPlayerBar - the bottom bar of the VLTK 2.0 client (KUiPlayerBar, 玩家信息主界面.ini; gamecl.exe LoadScheme
# 0x00472360 from 0x004750E0, docs/CLIENT-2.0.md §7), read line by line:
#   - [Main]: the full-screen frame 玩家信息下版1024.spr (PositionType 2 = centred at the bottom), DummyWnd=1 (it
#     never takes the mouse); [Main] ToolBoxSchema names the tool bar (工具控制条.ini) loaded next (0x004751A0 ->
#     0x00474B20) - UiControlBar draws that one;
#   - Face (the emoticon button), Market, DateTime (text; GameLogo / ping words in the section), AdultPermit;
#   - Item_%d for 0..8 (0x00472410: KWndObjectBox at (158 + 38 i, 728) 36 px; +0x28b8 cleared) - the quick item
#     slots; ImediaLeftSkill / ImediaRightSkill (791 / 828, 724) - the two mouse skills (EnableClickEmpty=1);
#   - InputEdit (the chat line, Type=2, MaxLen 80, Color 254,255,160; FocusBKColor / FocusNoCanBKColor + alpha
#     read at 0x004724BA..: alpha 0..255 -> (255 - a) << 21 in the colour's high byte), SendBtn, ChannelBtn,
#     OpenChannelBtn, SwitchSizeBtn (the small bar 玩家信息主界面最小化.ini when +0x8d90), InputBack, StatusBack.
# The chat line is a Godot LineEdit placed on the [InputEdit] rect: the game scene listens to `chat_submitted`.
extends "res://ui/elem/KWndWindow.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndImage := preload("res://ui/elem/KWndImage.gd")
const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KWndObjContainer := preload("res://ui/elem/KWndObjContainer.gd")
const KWndLabeledButton := preload("res://ui/elem/KWndLabeledButton.gd")

const SCHEME := "thanh-nhan-vat"
const QUICK_SLOTS := 9        # 0x00472457: Item_0 .. Item_8 (0x2c70 / 0x4f0)

signal chat_submitted(text: String)
signal channel_clicked(at: Vector2)       # the ChannelBtn (0x004764A8): the channel menu opens at the mouse

const HISTORY := 8                        # KUiPlayerBar+0x7c46: 8 lines of 0x200, the index at +0x7c45 (0x00475AB0..)
var channel_btn: KWndLabeledButton = null
var _history: Array = []                  # the last lines sent, oldest first
var _history_pos := -1                    # -1 = the line being typed
signal mouse_skill_clicked(right: bool)   # a click on ImediaLeftSkill / ImediaRightSkill
signal quick_clicked(slot: int)           # a left click on a filled quick box, nothing on the cursor (msg 0x513 -> ShortcutUseItem)
signal quick_put(slot: int)               # a left click on a quick box with something on the cursor (msg 0x511 -> 0x00472E70)
signal quick_right_clicked(slot: int)     # a right click on a filled quick box (the new client clears it)
signal quick_hovered(slot: int, obj)      # the mouse over a quick box: its object dict or null (the box's object tip)
signal mouse_skill_hovered(right: bool, obj)   # the mouse over ImediaLeftSkill / ImediaRightSkill

var chat_input: LineEdit = null
var _quick: Array = []        # the nine KWndObjContainer
var _left_box: KWndObjContainer = null
var _right_box: KWndObjContainer = null
var _datetime: KWndText = null


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_from(ini, "Main"):
		return false
	place_on_screen(ini, "Main", screen)
	name = "UiPlayerBar"
	mouse_filter = Control.MOUSE_FILTER_IGNORE   # DummyWnd=1
	var back := KWndImage.new()
	add_child(back)
	if back.init_from(ini, "Main"):
		back.position = Vector2.ZERO
	else:
		back.queue_free()
	for section in ["InputBack", "StatusBack", "AdultPermit"]:
		var img := KWndImage.new()
		add_child(img)
		if not img.init_from(ini, section):
			img.queue_free()
	if ini.has_section("DateTime"):
		_datetime = KWndText.new()
		add_child(_datetime)
		_datetime.init_from(ini, "DateTime")
	if ini.has_section("Face"):
		var face := KWndButton.new()
		add_child(face)
		face.init_from(ini, "Face")
	# 0x00472410: the nine quick slots
	for i in QUICK_SLOTS:
		var box := KWndObjContainer.new()
		add_child(box)
		if box.init_from(ini, "Item_%d" % i):
			box.accept_free = true   # a KWndObjectBox takes whatever is dropped on it (one cell)
			var slot_index := i
			box.object_clicked.connect(func(_o): quick_clicked.emit(slot_index))
			box.put_requested.connect(func(_x, _y): quick_put.emit(slot_index))
			box.object_right_clicked.connect(func(_o): quick_right_clicked.emit(slot_index))
			box.hovered.connect(func(o): quick_hovered.emit(slot_index, o))
			_quick.append(box)
		else:
			box.queue_free()
	_left_box = _skill_box(ini, "ImediaLeftSkill", false)
	_right_box = _skill_box(ini, "ImediaRightSkill", true)
	# the chat line on the [InputEdit] rect
	if ini.has_section("InputEdit"):
		chat_input = LineEdit.new()
		chat_input.name = "InputEdit"
		chat_input.position = Vector2(ini.get_integer("InputEdit", "Left", 0), ini.get_integer("InputEdit", "Top", 0))
		chat_input.size = Vector2(ini.get_integer("InputEdit", "Width", 200), maxi(ini.get_integer("InputEdit", "Height", 15), 15))
		chat_input.max_length = ini.get_integer("InputEdit", "MaxLen", 80)
		chat_input.placeholder_text = "Chat (Enter)"
		chat_input.add_theme_font_size_override("font_size", ini.get_integer("InputEdit", "Font", 12))
		chat_input.add_theme_color_override("font_color", ini.get_color("InputEdit", "Color", Color(254 / 255.0, 1.0, 160 / 255.0)))
		var focus := StyleBoxFlat.new()
		var bk := ini.get_color("InputEdit", "FocusBKColor", Color(25 / 255.0, 31 / 255.0, 11 / 255.0))
		bk.a = ini.get_integer("InputEdit", "FocusBKColorAlpha", 180) / 255.0
		focus.bg_color = bk
		var plain := StyleBoxEmpty.new()
		chat_input.add_theme_stylebox_override("normal", plain)
		chat_input.add_theme_stylebox_override("focus", focus)
		chat_input.text_submitted.connect(_on_submitted)
		chat_input.gui_input.connect(_on_input_key)
		add_child(chat_input)
	# the channel button left of the line (0x004730D0 paints it in the channel's colour; the short name is written on
	# it here so the channel can be read): a click opens the channel menu (0x004764A8 -> 0x00472620)
	if ini.has_section("ChannelBtn"):
		channel_btn = KWndLabeledButton.new()
		add_child(channel_btn)
		channel_btn.init_from(ini, "ChannelBtn")
		channel_btn.font_size = maxi(ini.get_integer("ChannelBtn", "Font", 12), 12)
		channel_btn.full_text = true
		channel_btn.clicked.connect(func(): channel_clicked.emit(channel_btn.global_position + Vector2(0, -4)))
	if ini.has_section("SendBtn"):
		var send := KWndButton.new()
		add_child(send)
		send.init_from(ini, "SendBtn")
		send.clicked.connect(func(): if chat_input != null: _on_submitted(chat_input.text))
	Log.info("ui", "player bar", {"quick": _quick.size(), "chat": chat_input != null})
	return true


func _skill_box(ini: KUiScheme, section: String, right: bool) -> KWndObjContainer:
	if not ini.has_section(section):
		return null
	var box := KWndObjContainer.new()
	add_child(box)
	box.init_from(ini, section)
	box.object_clicked.connect(func(_o): mouse_skill_clicked.emit(right))
	box.put_requested.connect(func(_x, _y): mouse_skill_clicked.emit(right))
	box.hovered.connect(func(o): mouse_skill_hovered.emit(right, o))
	return box


func _on_submitted(text: String) -> void:
	if chat_input != null:
		chat_input.text = ""
		chat_input.release_focus()
	if text.strip_edges() != "":   # 0x00475AA7..0x00475B02: the line into the ring of eight
		_history.append(text)
		while _history.size() > HISTORY:
			_history.pop_front()
	_history_pos = -1
	chat_submitted.emit(text)


# Up / Down in the line walk the ring (KUiPlayerBar::WndProc: the edit's key message)
func _on_input_key(event: InputEvent) -> void:
	if chat_input == null or not (event is InputEventKey) or not event.pressed or _history.is_empty():
		return
	if event.keycode == KEY_UP:
		_history_pos = (_history.size() - 1) if _history_pos < 0 else maxi(_history_pos - 1, 0)
	elif event.keycode == KEY_DOWN:
		if _history_pos < 0:
			return
		_history_pos += 1
		if _history_pos >= _history.size():
			_history_pos = -1
			chat_input.text = ""
			chat_input.accept_event()
			return
	else:
		return
	chat_input.text = str(_history[_history_pos])
	chat_input.caret_column = chat_input.text.length()
	chat_input.accept_event()


# the current channel on the button: its short name in its colour (0x004730D0: the 3-byte colour code as the caption)
func set_channel(short: String, color: Color) -> void:
	if channel_btn == null:
		return
	channel_btn.font_color = color
	channel_btn.over_color = color.lightened(0.3)
	channel_btn.select_color = color
	channel_btn.set_label(short)


func history() -> Array:
	return _history.duplicate()


# the two mouse skills (KPlayer::m_nLeftSkillID / m_nRightSkillID): {id, name, icon} or empty
# what a quick box shows: an object dict of KWndObjContainer ({id, image, name, count, ...}) or an empty one
func set_quick(slot: int, obj: Dictionary) -> void:
	if slot < 0 or slot >= _quick.size():
		return
	var box: KWndObjContainer = _quick[slot]
	if obj.is_empty():
		box.set_objects([])
	else:
		var o := obj.duplicate()
		o["x"] = 0
		o["y"] = 0
		o["w"] = 1
		o["h"] = 1
		box.set_objects([o])


# the size of the item on the cursor: a click on a quick box then asks to put it there
func set_hand(cells: Vector2i) -> void:
	for box in _quick:
		box.hand_size = cells
		box.queue_redraw()


func set_mouse_skills(left: Dictionary, right: Dictionary) -> void:
	for pair in [[_left_box, left], [_right_box, right]]:
		var box: KWndObjContainer = pair[0]
		var info: Dictionary = pair[1]
		if box == null:
			continue
		if info.is_empty():
			box.set_objects([])
		else:
			box.set_objects([{"id": int(info.get("id", 0)), "x": 0, "y": 0, "w": 1, "h": 1, "image": Assets.item_image(str(info.get("icon", ""))),
				"ex_type": 0, "usable": false, "name": str(info.get("name", "")), "count": 0}])


func _process(_delta: float) -> void:
	if _datetime != null:
		var t := Time.get_time_dict_from_system()
		var s := "%02d:%02d" % [int(t.hour), int(t.minute)]
		if _datetime.text != s:
			_datetime.text = s
