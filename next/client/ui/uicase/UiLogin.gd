# KUiLogin - "Đăng nhập": account, password and what goes with them
# (Ui\UiCase\UiLogin.cpp; layout <theme>\UiNewLogin\登陆.ini, read by KUiLogin::LoadScheme of
# gamecl.exe 2.0 at 0x4853F0).
#
# From the JX1 source: Account / Password / Login / Cancel / Remember, the account remembered from
# last time with the keyboard going to the password box when there is one, Tab and Enter walking
# Account -> Password -> Login, Escape and Cancel going back to the server list, and a login with
# an empty box answered by sentence 15 of the connect box.
# Added by the 2.0 layout: the texts beside the three check boxes (RememberTxt, InvisibleTxt,
# VirtualKeyboardTxt), the chosen server and "Đổi Server" (SelServerInfo, ServerName, SelServer),
# the agreement line (Agree, UserNotify, PrivacyNotify) with its four reminders (HaveNot*), and the
# list of recent accounts (BtnOpenAccountList, AccountList).
#
# What this client does NOT do with them, on purpose: a password is never stored (the old
# "remember all" switch is gone), "Đăng nhập ẩn" is remembered but our gateway has no hidden state
# yet, and the two agreement links open the addresses of config/serverlist.json ("user_notify_url",
# "privacy_notify_url") - never the old publisher's pages the layout names.
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KWndLabeledButton := preload("res://ui/elem/KWndLabeledButton.gd")
const KWndEdit := preload("res://ui/elem/KWndEdit.gd")
const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndList := preload("res://ui/elem/KWndList.gd")
const KLoginServer := preload("res://net/KLoginServer.gd")

const SCHEME := "dang-nhap"
const MAX_RECENT_ACCOUNTS := 6

signal login_requested(account: String, password: String)
signal input_missing                 # a box was empty: sentence 15
signal cancelled                     # back to the server list
signal change_server

var login_bg := "Login"
var user_notify_url := ""
var privacy_notify_url := ""

var _account := KWndEdit.new()
var _password := KWndEdit.new()
var _login := KWndButton.new()
var _cancel := KWndButton.new()
var _remember := KWndButton.new()
var _remember_txt := KWndText.new()
var _invisible := KWndButton.new()
var _invisible_txt := KWndText.new()
var _keyboard := KWndButton.new()
var _keyboard_txt := KWndText.new()
var _server_info := KWndText.new()
var _server_name := KWndText.new()
var _sel_server := KWndLabeledButton.new()
var _agree := KWndButton.new()
var _user_notify := KWndLabeledButton.new()
var _privacy_notify := KWndLabeledButton.new()
var _reminders := {}                 # "HaveNotAgree" ... -> KWndText
var _open_accounts := KWndButton.new()
var _account_list := KWndList.new()
var _recent: Array = []


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiLogin"
	login_bg = ini.get_string("Main", "LoginBg", "Login")
	for pair in [[_account, "Account"], [_password, "Password"], [_login, "Login"], [_cancel, "Cancel"],
			[_remember, "Remember"], [_remember_txt, "RememberTxt"], [_invisible, "Invisible"], [_invisible_txt, "InvisibleTxt"],
			[_keyboard, "VirtualKeyboard"], [_keyboard_txt, "VirtualKeyboardTxt"],
			[_server_info, "SelServerInfo"], [_server_name, "ServerName"], [_sel_server, "SelServer"],
			[_agree, "Agree"], [_user_notify, "UserNotify"], [_privacy_notify, "PrivacyNotify"],
			[_open_accounts, "BtnOpenAccountList"], [_account_list, "AccountList"]]:
		add_child(pair[0])
		pair[0].init_from(ini, pair[1])
	for section in ["HaveNotReadUN", "HaveNotReadPN", "HaveNotReadAll", "HaveNotAgree"]:
		var t := KWndText.new()
		add_child(t)
		if t.init_from(ini, section):
			t.text = ini.get_string(section, "RichText", "")
			t.visible = false
			_reminders[section] = t
	# the plain list of the old client: the colours of this one are named Msg* in the layout
	_account_list.item_color = ini.get_color("AccountList", "MsgColor", Color.WHITE)
	_account_list.visible = false
	_account_list.item_selected.connect(_on_recent_picked)

	_login.clicked.connect(_on_login)
	_cancel.clicked.connect(_on_cancel)
	_sel_server.clicked.connect(_on_cancel_to_servers)
	_open_accounts.clicked.connect(_toggle_account_list)
	_agree.toggled.connect(func(_on): _hide_reminders())
	_user_notify.clicked.connect(func(): _open_url(user_notify_url))
	_privacy_notify.clicked.connect(func(): _open_url(privacy_notify_url))
	_account.submitted.connect(func(_t): _password.take_focus())
	_password.submitted.connect(func(_t): _on_login())
	_account.tab_pressed.connect(func(back): if not back: _password.take_focus())
	_password.tab_pressed.connect(func(back): if back: _account.take_focus())
	_account.escape_pressed.connect(_on_cancel)
	_password.escape_pressed.connect(_on_cancel)
	focus_mode = Control.FOCUS_ALL
	return true


# KUiLogin::Show
func open(server: Dictionary, choice: Dictionary, links: Dictionary = {}) -> void:
	user_notify_url = str(links.get("user_notify_url", ""))
	privacy_notify_url = str(links.get("privacy_notify_url", ""))
	_server_name.text = str(server.get("title", ""))
	_remember.check(bool(choice.get("remember", false)))
	_invisible.check(bool(choice.get("invisible", false)))
	_agree.check(bool(choice.get("agree", false)))
	_recent = choice.get("recent_accounts", [])
	_open_accounts.visible = not _recent.is_empty()
	_account_list.visible = false
	_hide_reminders()
	var last := str(choice.get("account", "")) if bool(choice.get("remember", false)) else ""
	_account.set_text(last)
	_password.clear_text()
	show_window()
	if last != "":
		_password.take_focus()
	else:
		_account.take_focus()


# KUiLogin::CloseWindow(false): the password never outlives the window
func close() -> void:
	_password.clear_text()
	hide_window()


func set_account(account: String, password: String = "") -> void:
	_account.set_text(account)
	_password.set_text(password)


func choice() -> Dictionary:
	return {"remember": _remember.is_checked(), "invisible": _invisible.is_checked(), "agree": _agree.is_checked(),
		"account": _account.get_text().strip_edges() if _remember.is_checked() else ""}


func _hide_reminders() -> void:
	for t in _reminders.values():
		t.visible = false


func _on_login() -> void:
	_hide_reminders()
	if _agree.visible and not _agree.is_checked() and _reminders.has("HaveNotAgree"):
		_reminders["HaveNotAgree"].visible = true
		return
	var account := _account.get_text().strip_edges()
	var password := _password.get_text()
	if account == "" or password == "":
		close()
		input_missing.emit()
		return
	close()
	login_requested.emit(account, password)


func _on_cancel() -> void:
	close()
	cancelled.emit()


func _on_cancel_to_servers() -> void:
	close()
	change_server.emit()


func _toggle_account_list() -> void:
	if _account_list.visible:
		_account_list.visible = false
		return
	_account_list.set_items(_recent.slice(0, MAX_RECENT_ACCOUNTS))
	_account_list.set_cur_sel(-1)
	_account_list.visible = true
	_account_list.move_to_front()


func _on_recent_picked(index: int) -> void:
	_account.set_text(_account_list.item_text(index))
	_account_list.visible = false
	_password.take_focus()


func _open_url(url: String) -> void:
	if url != "":
		OS.shell_open(url)


func _gui_input(event: InputEvent) -> void:
	if not (event is InputEventKey) or not event.pressed:
		return
	match event.keycode:
		KEY_ENTER, KEY_KP_ENTER:
			_on_login()
		KEY_ESCAPE:
			_on_cancel()
		_:
			return
	accept_event()
