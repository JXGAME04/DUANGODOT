# Character lobby: list / create / enter world.
extends Control

const KLogin := preload("res://net/KLogin.gd")
const SERIES_NAMES := ["Kim", "Mộc", "Thủy", "Hỏa", "Thổ"]
const SEX_NAMES := ["Nam", "Nữ"]

var _list: ItemList
var _name: LineEdit
var _series: OptionButton
var _sex: OptionButton
var _create: Button
var _enter: Button
var _status: Label
var _chars: Array = []


func _ready() -> void:
	_build_ui()
	Game.char_list.connect(_on_char_list)
	Game.char_created.connect(_on_char_created)
	Game.entered_world.connect(_on_entered_world)
	Game.enter_failed.connect(_on_enter_failed)
	Game.connection_lost.connect(_on_connection_lost)
	Game.kicked.connect(_on_kicked)
	Log.info("ui", "char select screen")
	Game.request_char_list()


func _build_ui() -> void:
	var center := CenterContainer.new()
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(center)
	var box := VBoxContainer.new()
	box.custom_minimum_size = Vector2(480, 0)
	box.add_theme_constant_override("separation", 8)
	center.add_child(box)

	var title := Label.new()
	title.text = "Chọn nhân vật"
	title.add_theme_font_size_override("font_size", 28)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	box.add_child(title)

	_list = ItemList.new()
	_list.custom_minimum_size = Vector2(0, 160)
	_list.item_activated.connect(func(_i): _on_enter_pressed())
	box.add_child(_list)

	_enter = Button.new()
	_enter.text = "Vào game"
	_enter.pressed.connect(_on_enter_pressed)
	box.add_child(_enter)

	var sep := HSeparator.new()
	box.add_child(sep)

	var create_label := Label.new()
	create_label.text = "Tạo nhân vật mới"
	box.add_child(create_label)
	var row := HBoxContainer.new()
	box.add_child(row)
	_name = LineEdit.new()
	_name.placeholder_text = "Tên nhân vật (2-16 ký tự)"
	_name.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_name.text_submitted.connect(func(_t): _on_create_pressed())
	row.add_child(_name)
	_series = OptionButton.new()
	for s in SERIES_NAMES:
		_series.add_item(s)
	row.add_child(_series)
	_sex = OptionButton.new()
	for s in SEX_NAMES:
		_sex.add_item(s)
	row.add_child(_sex)
	_create = Button.new()
	_create.text = "Tạo"
	_create.pressed.connect(_on_create_pressed)
	row.add_child(_create)

	_status = Label.new()
	_status.autowrap_mode = TextServer.AUTOWRAP_WORD
	box.add_child(_status)

	var back := Button.new()
	back.text = "Thoát"
	back.pressed.connect(func():
		Game.logout("back to login")
		get_tree().change_scene_to_file("res://scenes/UiLogin.tscn"))
	box.add_child(back)


func _on_char_list(chars: Array) -> void:
	_chars = chars
	_list.clear()
	for c in chars:
		_list.add_item("%s  (Lv %d, %s, %s)" % [c.name, c.level, SERIES_NAMES[clampi(c.series, 0, 4)], SEX_NAMES[clampi(c.sex, 0, 1)]])
	if chars.size() > 0:
		_list.select(0)
		_status.text = "%d nhân vật." % chars.size()
	else:
		_status.text = "Chưa có nhân vật - hãy tạo một nhân vật."
	if "--auto" in OS.get_cmdline_user_args():
		if chars.size() > 0:
			if DisplayServer.get_name() != "headless":
				await RenderingServer.frame_post_draw
				await RenderingServer.frame_post_draw
				get_viewport().get_texture().get_image().save_png("user://logs/auto_charselect.png")
			Log.info("auto", "auto enter", {"pid": chars[0].pid, "name": chars[0].name})
			_on_enter_pressed()
		else:
			_name.text = "Auto %d" % (randi() % 100000)
			Log.info("auto", "auto create", {"name": _name.text})
			_on_create_pressed()


func _on_create_pressed() -> void:
	var name := _name.text.strip_edges()
	if name.length() < 2:
		_status.text = "Tên quá ngắn."
		return
	_create.disabled = true
	Game.create_char(name, _series.selected, _sex.selected)


func _on_char_created(ok: bool, result: int, summary: Dictionary) -> void:
	_create.disabled = false
	if ok:
		_status.text = "Đã tạo %s." % summary.name
		_name.text = ""
		Game.request_char_list()
	else:
		var reasons := {7: "Tên không hợp lệ.", 5: "Tên đã tồn tại.", 6: "Đã đủ số nhân vật."}
		_status.text = reasons.get(result, "Tạo nhân vật thất bại (%d)." % result)


func _on_enter_pressed() -> void:
	var sel := _list.get_selected_items()
	if sel.is_empty() or sel[0] >= _chars.size():
		_status.text = "Chọn một nhân vật."
		return
	_enter.disabled = true
	_status.text = "Đang vào game..."
	Game.enter_world(int(_chars[sel[0]].pid))


func _on_entered_world(_info: Dictionary) -> void:
	get_tree().change_scene_to_file("res://scenes/UiGame.tscn")


func _on_enter_failed(result: int) -> void:
	_enter.disabled = false
	_status.text = "Không vào được game (%d)%s" % [result, " - zone chưa sẵn sàng" if result == 9 else ""]


func _on_kicked(reason: int, text: String) -> void:
	_enter.disabled = false
	_status.text = "Bị ngắt: " + KLogin.result_text(reason, text)
	if KLogin.session_ends(reason):
		Game.logout("kicked")
		get_tree().change_scene_to_file("res://scenes/UiLogin.tscn")


func _on_connection_lost(reason: String) -> void:
	Log.warn("ui", "connection lost in lobby", {"reason": reason})
	get_tree().change_scene_to_file("res://scenes/UiLogin.tscn")
