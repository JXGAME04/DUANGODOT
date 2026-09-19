# UiLoginPlain - the login flow with plain Godot controls, for a checkout that has no exported
# game data (assets/ui is made from the old client by `python tools/dev.py assets` and is never
# committed).  CI and a fresh clone still have to log in, make a character and enter the world, so
# UiShell falls back to this: one panel, three steps - log in, pick or make a character, enter.
# With `-- --auto` it walks the steps by itself, exactly like UiShell does with the real windows.
extends Control

const KLogin := preload("res://net/KLogin.gd")
const SERIES_NAMES := ["Kim", "Mộc", "Thủy", "Hỏa", "Thổ"]
const SEX_NAMES := ["Nam", "Nữ"]

var notice := "":
	set(value):
		notice = value
		if _status != null:
			_status.text = value

var _args := {}
var _server: LineEdit
var _account: LineEdit
var _password: LineEdit
var _login_btn: Button
var _list: ItemList
var _new_name: LineEdit
var _series: OptionButton
var _sex: OptionButton
var _status: Label
var _login_box: VBoxContainer
var _lobby_box: VBoxContainer


func _ready() -> void:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("--"):
			var eq := a.find("=")
			_args[a.substr(2, eq - 2) if eq > 0 else a.substr(2)] = a.substr(eq + 1) if eq > 0 else ""
	set_anchors_preset(Control.PRESET_FULL_RECT)
	_build()
	Game.login_result.connect(_on_login_result)
	Game.char_list.connect(_on_char_list)
	Game.char_created.connect(_on_char_created)
	Game.entered_world.connect(func(_i): get_tree().change_scene_to_file("res://scenes/UiGame.tscn"))
	Game.enter_failed.connect(func(result): _say("Không vào được game: " + KLogin.result_text(result)))
	Game.kicked.connect(func(reason, text): _say("Bị ngắt: " + KLogin.result_text(reason, text)); _to_login())
	Game.connection_lost.connect(func(_r): _to_login())
	if _args.has("server"):
		_server.text = str(_args["server"])
	elif OS.has_environment("JX_SERVER"):
		_server.text = OS.get_environment("JX_SERVER")
	if _args.has("account"):
		_account.text = str(_args["account"])
	if _args.has("password"):
		_password.text = str(_args["password"])
	if Game.last_notice != "":
		_say(Game.last_notice)
		Game.last_notice = ""
	if Game.state == "lobby":
		_to_lobby()
		Game.request_char_list()
	elif _args.has("auto"):
		call_deferred("_on_login_pressed")


func _build() -> void:
	var center := CenterContainer.new()
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(center)
	var box := VBoxContainer.new()
	box.custom_minimum_size = Vector2(420, 0)
	box.add_theme_constant_override("separation", 8)
	center.add_child(box)
	var title := Label.new()
	title.text = "JX NEXT"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 34)
	box.add_child(title)

	_login_box = VBoxContainer.new()
	box.add_child(_login_box)
	_server = _field(_login_box, "Máy chủ (host:port, tls://, ws://)", "127.0.0.1:19100")
	_account = _field(_login_box, "Tài khoản", "")
	_password = _field(_login_box, "Mật khẩu", "")
	_password.secret = true
	_password.text_submitted.connect(func(_t): _on_login_pressed())
	_login_btn = Button.new()
	_login_btn.text = "Đăng nhập"
	_login_btn.pressed.connect(_on_login_pressed)
	_login_box.add_child(_login_btn)

	_lobby_box = VBoxContainer.new()
	_lobby_box.visible = false
	box.add_child(_lobby_box)
	_list = ItemList.new()
	_list.custom_minimum_size = Vector2(0, 120)
	_list.item_activated.connect(func(_i): _on_enter_pressed())
	_lobby_box.add_child(_list)
	var enter := Button.new()
	enter.text = "Vào game"
	enter.pressed.connect(_on_enter_pressed)
	_lobby_box.add_child(enter)
	_new_name = _field(_lobby_box, "Tên nhân vật mới (2-16 ký tự, không có khoảng trắng)", "")
	_new_name.max_length = 16
	var row := HBoxContainer.new()
	_lobby_box.add_child(row)
	_series = OptionButton.new()
	for s in SERIES_NAMES:
		_series.add_item(s)
	row.add_child(_series)
	_sex = OptionButton.new()
	for s in SEX_NAMES:
		_sex.add_item(s)
	row.add_child(_sex)
	var create := Button.new()
	create.text = "Tạo nhân vật"
	create.pressed.connect(_on_create_pressed)
	row.add_child(create)
	var back := Button.new()
	back.text = "Thoát"
	back.pressed.connect(func(): Game.logout("back to login"); _to_login())
	_lobby_box.add_child(back)

	_status = Label.new()
	_status.text = notice
	_status.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	_status.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	box.add_child(_status)


func _field(parent: Control, label: String, value: String) -> LineEdit:
	var l := Label.new()
	l.text = label
	parent.add_child(l)
	var e := LineEdit.new()
	e.text = value
	parent.add_child(e)
	return e


func _say(text: String) -> void:
	_status.text = text


func _to_login() -> void:
	_login_box.visible = true
	_lobby_box.visible = false
	_login_btn.disabled = false


func _to_lobby() -> void:
	_login_box.visible = false
	_lobby_box.visible = true


func _on_login_pressed() -> void:
	if _account.text.strip_edges() == "" or _password.text == "":
		_say("Xin nhập vào tài khoản và mật khẩu.")
		return
	_login_btn.disabled = true
	_say("Đang kết nối %s ..." % _server.text)
	Log.info("ui", "login (plain)", {"server": _server.text, "account": _account.text})
	Game.login(_server.text.strip_edges(), _account.text.strip_edges(), _password.text)


func _on_login_result(ok: bool, text: String) -> void:
	_login_btn.disabled = false
	if ok:
		_to_lobby()
		Game.request_char_list()
		return
	_say(text)
	Game.last_notice = ""
	if _args.has("auto"):
		print("AUTO_LOGIN_FAILED %s" % text)
		get_tree().quit(2)


func _on_char_list(chars: Array) -> void:
	_list.clear()
	for c in chars:
		_list.add_item("%s  (LV:%d, %s, %s)" % [c.name, c.level, SERIES_NAMES[clampi(c.series, 0, 4)], SEX_NAMES[clampi(c.sex, 0, 1)]])
	if not chars.is_empty():
		_list.select(0)
	_say("%d nhân vật." % chars.size() if not chars.is_empty() else "Chưa có nhân vật - hãy tạo một nhân vật.")
	if _args.has("auto"):
		if chars.is_empty():
			# --place=<map id> picks the starting village (the Id of NativePlaceList.ini); 0 = the default map
			Game.create_char("Auto%d" % (randi() % 100000), 0, 0, int(str(_args.get("place", "0"))))
		else:
			Game.enter_world(int(chars[0].pid))


func _on_enter_pressed() -> void:
	var picked := _list.get_selected_items()
	if picked.is_empty() or picked[0] >= Game.chars.size():
		_say("Chọn một nhân vật.")
		return
	_say("Đang vào game...")
	Game.enter_world(int(Game.chars[picked[0]].pid))


func _on_create_pressed() -> void:
	var who := _new_name.text
	if who.length() < 2 or who.contains(" "):
		_say("Tên nhân vật không hợp lệ.")
		return
	var series := _series.selected
	var sex := _sex.selected
	if series == 0:
		sex = 0          # Kim: nam
	elif series == 2:
		sex = 1          # Thủy: nữ
	Game.create_char(who, series, sex)


func _on_char_created(ok: bool, result: int, summary: Dictionary) -> void:
	if ok:
		_say("Đã tạo %s." % summary.get("name", ""))
		Game.enter_world(int(summary.get("pid", 0)))
		return
	var reasons := {7: "Tên nhân vật không hợp lệ.", 5: "Tên nhân vật đã có người dùng.", 6: "Tài khoản đã đủ số nhân vật."}
	_say(reasons.get(result, "Tạo nhân vật thất bại (%d)." % result))
	if _args.has("auto"):
		print("AUTO_CREATE_FAILED %d" % result)
		get_tree().quit(2)
