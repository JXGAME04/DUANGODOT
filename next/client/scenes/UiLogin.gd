# Login screen: server, account, password -> Game.login()
extends Control

var _server: LineEdit
var _account: LineEdit
var _password: LineEdit
var _button: Button
var _status: Label


func _ready() -> void:
	_build_ui()
	Game.login_result.connect(_on_login_result)
	var args := OS.get_cmdline_user_args()
	for a in args:
		if a.begins_with("--server="):
			_server.text = a.substr(9)
		elif a.begins_with("--account="):
			_account.text = a.substr(10)
		elif a.begins_with("--password="):
			_password.text = a.substr(11)
	if OS.has_environment("JX_SERVER"):
		_server.text = OS.get_environment("JX_SERVER")
	if Game.last_notice != "":   # kicked / lost the connection: say why (KUiConnectInfo of the old client)
		_status.text = Game.last_notice
		Game.last_notice = ""
	Log.info("ui", "login screen")
	if "--auto" in args:
		# automated end-to-end run (tests): login -> create/pick character -> enter -> move once
		Log.info("auto", "auto login", {"server": _server.text, "account": _account.text})
		call_deferred("_on_login_pressed")


func _build_ui() -> void:
	var center := CenterContainer.new()
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(center)
	var box := VBoxContainer.new()
	box.custom_minimum_size = Vector2(360, 0)
	box.add_theme_constant_override("separation", 10)
	center.add_child(box)

	var title := Label.new()
	title.text = "JX NEXT"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 36)
	box.add_child(title)

	_server = _field(box, "Máy chủ (host:port)", "127.0.0.1:17100")
	_account = _field(box, "Tài khoản", "test1")
	_password = _field(box, "Mật khẩu", "test")
	_password.secret = true

	_button = Button.new()
	_button.text = "Đăng nhập"
	_button.pressed.connect(_on_login_pressed)
	box.add_child(_button)

	_status = Label.new()
	_status.text = "Máy chủ dev: tài khoản mới tự tạo khi đăng nhập lần đầu (mật khẩu phải giống lần sau)."
	_status.autowrap_mode = TextServer.AUTOWRAP_WORD
	_status.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	box.add_child(_status)
	_account.grab_focus()


func _field(parent: Control, label: String, value: String) -> LineEdit:
	var l := Label.new()
	l.text = label
	parent.add_child(l)
	var e := LineEdit.new()
	e.text = value
	e.text_submitted.connect(func(_t): _on_login_pressed())
	parent.add_child(e)
	return e


func _on_login_pressed() -> void:
	if _account.text.strip_edges() == "":
		_status.text = "Nhập tài khoản."
		return
	_button.disabled = true
	_status.text = "Đang kết nối %s ..." % _server.text
	Log.info("ui", "login pressed", {"server": _server.text, "account": _account.text})
	Game.login(_server.text.strip_edges(), _account.text.strip_edges(), _password.text)


func _on_login_result(ok: bool, text: String) -> void:
	_button.disabled = false
	if ok:
		_status.text = "Đăng nhập thành công."
		get_tree().change_scene_to_file("res://scenes/UiSelPlayer.tscn")
	else:
		_status.text = text
		Game.last_notice = ""
		if "--auto" in OS.get_cmdline_user_args():
			# automated run: never sit on the login screen waiting for a human
			print("AUTO_LOGIN_FAILED %s" % text)
			Log.error("auto", "login failed", {"text": text})
			if DisplayServer.get_name() != "headless":
				await RenderingServer.frame_post_draw
				await RenderingServer.frame_post_draw
				get_viewport().get_texture().get_image().save_png("user://logs/auto_login_failed.png")
			get_tree().quit(2)
