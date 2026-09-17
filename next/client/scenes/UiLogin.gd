# Login screen: the old client's window, laid out from assets/ui/login.json.
#
# The picture, the two boxes and the four buttons are the ones \Ui\Ui3\登陆.ini names, at the
# coordinates it gives, with the artwork of the 2.0 client.  The only thing added is the server
# box at the bottom, which the old client did not need (it read ServerList.ini) and we do while
# there is no server list yet.
extends Control

const KUiScheme := preload("res://scenes/KUiScheme.gd")

var _scheme: KUiScheme = null
var _server: LineEdit
var _account: LineEdit
var _password: LineEdit
var _button: BaseButton
var _status: Label


func _ready() -> void:
	if not _build_scheme():
		_build_plain_ui()
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
	KUiScheme.shot_if_asked(self, "login_screen")
	if "--auto" in args:
		# automated end-to-end run (tests): login -> create/pick character -> enter -> move once
		Log.info("auto", "auto login", {"server": _server.text, "account": _account.text})
		call_deferred("_on_login_pressed")


# ------------------------------------------------------------ the old window

func _build_scheme() -> bool:
	var s := KUiScheme.new()
	if not s.build(self, "login"):
		return false
	_scheme = s
	_account = s.widget("account") as LineEdit
	_password = s.widget("password") as LineEdit
	_button = s.widget("login") as BaseButton
	var cancel := s.widget("cancel") as BaseButton
	if _account == null or _password == null or _button == null:
		Log.warn("ui", "login layout has no account/password/login section")
		s.root.queue_free()
		return false
	_account.text = "test1"
	_password.text = "test"
	_account.text_submitted.connect(func(_t): _password.grab_focus())
	_password.text_submitted.connect(func(_t): _on_login_pressed())
	_button.pressed.connect(_on_login_pressed)
	if cancel != null:
		cancel.pressed.connect(func(): get_tree().quit())
	_status = _add_status(s.root)
	_server = _add_server_box(s.root)
	_account.grab_focus()
	return true


# One line under the window for messages, where the old client put its notices.
func _add_status(root: Control) -> Label:
	var l := Label.new()
	l.name = "Status"
	l.position = Vector2(100, 370)
	l.size = Vector2(600, 40)
	l.custom_minimum_size = l.size
	l.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	l.add_theme_font_size_override("font_size", 13)
	l.add_theme_color_override("font_color", Color8(255, 236, 170))
	l.add_theme_color_override("font_outline_color", Color8(20, 12, 6))
	l.add_theme_constant_override("outline_size", 3)
	l.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.add_child(l)
	return l


# The server box: ours, not the old client's, so it sits out of the way at the bottom.
func _add_server_box(root: Control) -> LineEdit:
	var row := HBoxContainer.new()
	row.name = "ServerRow"
	row.position = Vector2(240, 560)
	row.size = Vector2(320, 24)
	row.custom_minimum_size = row.size
	root.add_child(row)
	var l := Label.new()
	l.text = "Máy chủ"
	l.add_theme_font_size_override("font_size", 13)
	l.add_theme_color_override("font_color", Color8(255, 236, 170))
	l.add_theme_color_override("font_outline_color", Color8(20, 12, 6))
	l.add_theme_constant_override("outline_size", 3)
	row.add_child(l)
	var e := LineEdit.new()
	e.text = "127.0.0.1:17100"
	e.custom_minimum_size = Vector2(240, 22)
	e.add_theme_font_size_override("font_size", 13)
	e.text_submitted.connect(func(_t): _on_login_pressed())
	row.add_child(e)
	return e


# ------------------------------------------------------------ fallback

# Used when assets/ui/login.json is not there (no exported bundle): plain Godot controls, so the
# client is still usable for a test run.
func _build_plain_ui() -> void:
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

	_server = _field(box, "Máy chủ (host:port, tls://, ws://)", "127.0.0.1:17100")
	_account = _field(box, "Tài khoản", "test1")
	_password = _field(box, "Mật khẩu", "test")
	_password.secret = true

	var b := Button.new()
	b.text = "Đăng nhập"
	b.pressed.connect(_on_login_pressed)
	box.add_child(b)
	_button = b

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


# ------------------------------------------------------------ behaviour

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
