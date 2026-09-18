# UiShell - everything the player sees before the game world (S3Client\Ui\UiShell.cpp drove the
# same windows: UiStart, the login flow, UiStartGame).
#
# The windows are the ones of the VLTK 2.0 client, opened in its order (docs/VLTK20-CLIENT.md):
#
#   KUiInit            Bắt Đầu Trò Chơi / Tùy Chọn Hệ Thống / Xem Ghi Hình / Thoát Khỏi Trò Chơi
#   KUiSelServer       Chọn Máy Chủ: regions and their servers
#   KUiLogin           Đăng nhập: account, password
#   KUiConnectInfo     "Hiện đang kết nối với máy chủ" ... and every failure, with Quay lại
#   KUiSelPlayer       Chọn nhân vật
#   KUiSelNativePlace  Chọn tân thủ thôn          (Tạo nhân vật goes through here first)
#   KUiNewPlayer       Tạo nhân vật
#
# with KUiLoginBackGround behind all of them.  Who opens whom is the old client's: Cancel in the
# server window goes to the first menu, Cancel in the login window to the servers, a failed login
# back to the login window, a lost connection back to the servers, a created character straight
# into the game.
#
# The windows are laid out for the screen their theme was drawn for - 1024x768 for ui3_1024 - so
# while this scene is up the window's content scale is that size: one layout pixel is one screen
# pixel at 1024x768 and the same picture, scaled, on anything else.  A wider window gets black bars
# left and right, like the old game on a wide monitor.
#
# Arguments after `--`:
#   --shot=<window>       save user://logs/ui_<window>.png and quit.  <window> is the folder name of
#                         the layout: bat-dau, chon-may-chu, dang-nhap, thong-bao-ket-noi,
#                         chon-nhan-vat, chon-tan-thu-thon, tao-nhan-vat (the last three show
#                         sample characters, no server needed)
#   --auto --account=A --password=P [--server=ADDR]
#                         log in, make or pick a character and enter the world without a person
#   --server=ADDR         adds that gateway as a region of its own ("Dòng lệnh")
#   --serverlist=FILE     a server list instead of the player's / the shipped one
extends Control

const KUiScheme := preload("res://ui/KUiScheme.gd")
const KWndShowAnimate := preload("res://ui/elem/KWndShowAnimate.gd")
const UiLoginBg := preload("res://ui/uicase/UiLoginBg.gd")
const UiInit := preload("res://ui/uicase/UiInit.gd")
const UiSelServer := preload("res://ui/uicase/UiSelServer.gd")
const UiLogin := preload("res://ui/uicase/UiLogin.gd")
const UiConnectInfo := preload("res://ui/uicase/UiConnectInfo.gd")
const UiSelPlayer := preload("res://ui/uicase/UiSelPlayer.gd")
const UiSelNativePlace := preload("res://ui/uicase/UiSelNativePlace.gd")
const UiNewPlayer := preload("res://ui/uicase/UiNewPlayer.gd")
const KLoginServer := preload("res://net/KLoginServer.gd")
const KLogin := preload("res://net/KLogin.gd")
const Proto := preload("res://proto/jx_pb.gd")

const GAME_SCENE := "res://scenes/UiGame.tscn"

var screen := Vector2i(1024, 768)
var canvas: Control = null
var bg: UiLoginBg = null
var server: Dictionary = {}         # what KUiSelServer chose: {title, address}

var _args := {}
var _old_scale_size := Vector2i.ZERO
var _init: UiInit = null
var _sel_server: UiSelServer = null
var _login: UiLogin = null
var _info: UiConnectInfo = null
var _sel_player: UiSelPlayer = null
var _native_place: UiSelNativePlace = null
var _new_player: UiNewPlayer = null
var _servers: KLoginServer = null
var _links := {}
var _creating := false               # a character was asked for: the list that follows enters the game
var _pending_pid := 0


func _ready() -> void:
	_parse_args()
	var probe = KUiScheme.open("chon-may-chu")
	if probe != null:
		screen = probe.screen_size()
	_old_scale_size = get_window().content_scale_size
	get_window().content_scale_size = screen
	set_anchors_preset(Control.PRESET_FULL_RECT)
	var black := ColorRect.new()
	black.color = Color.BLACK
	black.set_anchors_preset(Control.PRESET_FULL_RECT)
	black.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(black)
	canvas = Control.new()
	canvas.name = "Canvas"
	canvas.size = Vector2(screen)
	canvas.clip_contents = true
	canvas.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(canvas)
	resized.connect(_fit)
	_fit()
	if _args.has("shot") or _args.has("auto"):
		KWndShowAnimate.animate = false
	bg = UiLoginBg.new()
	canvas.add_child(bg)
	# [VersionText] stays empty, as on the real client's screens (it sits under the logo)
	if not _build_windows():
		_fallback("Chưa có giao diện: hãy chạy `python tools/dev.py assets` (jxassets export-ui).")
		return
	Game.login_result.connect(_on_login_result)
	Game.char_list.connect(_on_char_list)
	Game.char_created.connect(_on_char_created)
	Game.entered_world.connect(_on_entered_world)
	Game.enter_failed.connect(_on_enter_failed)
	Game.kicked.connect(_on_kicked)
	Game.connection_lost.connect(_on_connection_lost)
	_servers = KLoginServer.load_list(str(_args.get("server", "")), str(_args.get("serverlist", "")))
	_links = _servers.links
	Log.info("ui", "login flow ready", {"theme_screen": "%dx%d" % [screen.x, screen.y], "regions": _servers.regions.size(),
		"server_list": _servers.source, "fonts": preload("res://ui/KFont.gd").of(14) != null})
	_start()


func _exit_tree() -> void:
	if _old_scale_size != Vector2i.ZERO:
		get_window().content_scale_size = _old_scale_size


func _parse_args() -> void:
	for a in OS.get_cmdline_user_args():
		if a.begins_with("--"):
			var eq := a.find("=")
			if eq > 0:
				_args[a.substr(2, eq - 2)] = a.substr(eq + 1)
			else:
				_args[a.substr(2)] = ""
	if OS.has_environment("JX_SERVER") and not _args.has("server"):
		_args["server"] = OS.get_environment("JX_SERVER")


# The canvas sits in the middle of whatever the window is.
func _fit() -> void:
	if canvas != null:
		canvas.position = ((size - Vector2(screen)) * 0.5).floor()


func _build_windows() -> bool:
	_init = UiInit.new()
	_sel_server = UiSelServer.new()
	_login = UiLogin.new()
	_sel_player = UiSelPlayer.new()
	_native_place = UiSelNativePlace.new()
	_new_player = UiNewPlayer.new()
	_info = UiConnectInfo.new()
	for w in [_init, _sel_server, _login, _sel_player, _native_place, _new_player, _info]:
		canvas.add_child(w)
		if not w.load_scheme(screen):
			Log.error("ui", "layout missing", {"window": w.SCHEME})
			return false
	_init.enter_game.connect(open_sel_server)
	_init.exit_game.connect(func(): get_tree().quit())
	_init.game_config.connect(func(): _say_later("Tùy Chọn Hệ Thống"))
	_init.open_rep.connect(func(): _say_later("Xem Ghi Hình"))
	_sel_server.server_chosen.connect(_on_server_chosen)
	_sel_server.cancelled.connect(open_init)
	_login.login_requested.connect(_on_login_requested)
	_login.input_missing.connect(func(): _open_info(UiConnectInfo.CI_MI_ERROR_LOGIN_INPUT, UiConnectInfo.CI_NS_LOGIN_WND))
	_login.cancelled.connect(_back_to_servers)
	_login.change_server.connect(_back_to_servers)
	_info.confirmed.connect(_on_info_confirmed)
	_sel_player.enter_game.connect(_on_enter_game)
	_sel_player.new_role.connect(_on_new_role)
	_sel_player.cancelled.connect(_back_to_servers)
	_native_place.place_chosen.connect(func(id): _show(_new_player); _new_player.open(id))
	_native_place.cancelled.connect(func(): _show(_sel_player); _sel_player.open(Game.chars, Game.max_chars, _sel_player.selected))
	_new_player.create_requested.connect(_on_create_requested)
	_new_player.name_invalid.connect(func(sentence): _open_info(sentence, UiConnectInfo.CI_NS_NEW_ROLE_WND))
	_new_player.cancelled.connect(func(id): _show(_native_place); _native_place.open(id))
	return true


# Sets the backdrop a window asks for; every other window is already hidden by its own close.
func _show(window) -> void:
	bg.set_config(window.login_bg)


func _start() -> void:
	if _args.has("shot"):
		await _show_for_shot(str(_args["shot"]))
		return
	if _args.has("auto"):
		_auto_login()
		return
	if Game.last_notice != "":
		# thrown out of the world (kick, lost connection): say why, then the server list
		var why := Game.last_notice
		Game.last_notice = ""
		_open_info(UiConnectInfo.CI_MI_CONNECT_FAILED, UiConnectInfo.CI_NS_SEL_SERVER, why)
		return
	if Game.state == "lobby":
		# back from the world with the session still open: the characters
		_open_info(UiConnectInfo.CI_MI_GETTING_ROLE_DATA, UiConnectInfo.CI_NS_NONE)
		Game.request_char_list()
		return
	open_init()


# ---------------------------------------------------------------- the windows, in order

func open_init() -> void:
	_show(_init)
	_init.open()


func open_sel_server() -> void:
	_show(_sel_server)
	var choice := KLoginServer.load_choice()
	if _args.has("region"):
		choice["region"] = int(_args["region"])
	_sel_server.open(_servers, choice)


func _back_to_servers() -> void:
	Game.logout("back to login")
	open_sel_server()


func _on_server_chosen(chosen: Dictionary) -> void:
	server = chosen
	open_login()


func open_login() -> void:
	_show(_login)
	_login.open(server, KLoginServer.load_choice(), _links)


func _on_login_requested(account: String, password: String) -> void:
	var choice: Dictionary = _login.choice()
	choice["recent"] = account
	KLoginServer.save_choice(choice)
	_open_info(UiConnectInfo.CI_MI_CONNECTING, UiConnectInfo.CI_NS_NONE)
	Log.info("ui", "login", {"server": server.get("title", ""), "address": server.get("address", ""), "account": account})
	Game.login(str(server.get("address", "")), account, password)


func _open_info(index: int, then: int, text: String = "") -> void:
	for w in [_init, _sel_server, _login, _sel_player, _native_place, _new_player]:
		if w.visible:
			w.hide_window()
	_info.open(index, then, text)


func _on_info_confirmed(next_step: int) -> void:
	match next_step:
		UiConnectInfo.CI_NS_INIT_WND:
			Game.logout("back to login")
			open_init()
		UiConnectInfo.CI_NS_LOGIN_WND:
			open_login()
		UiConnectInfo.CI_NS_SEL_ROLE_WND:
			_show(_sel_player)
			_sel_player.open(Game.chars, Game.max_chars, _sel_player.selected)
		UiConnectInfo.CI_NS_NEW_ROLE_WND:
			_show(_new_player)
			_new_player.open(_new_player.place_id, true)
		UiConnectInfo.CI_NS_EXIT_PROGRAM:
			get_tree().quit()
		_:
			_back_to_servers()   # CI_NS_SEL_SERVER, and giving up while the flow was still waiting


# ---------------------------------------------------------------- what the gateway answers

func _on_login_result(ok: bool, text: String) -> void:
	if ok:
		_info.set_info_msg(UiConnectInfo.CI_MI_GETTING_ROLE_DATA)
		Game.request_char_list()
		return
	Game.last_notice = ""
	var index := UiConnectInfo.CI_MI_CONNECT_FAILED
	var then := UiConnectInfo.CI_NS_SEL_SERVER
	var detail := ""
	match Game.last_login_result:
		Proto.Result.UNAUTHORIZED:
			index = UiConnectInfo.CI_MI_ACCOUNT_PWD_ERROR
			then = UiConnectInfo.CI_NS_LOGIN_WND
		Proto.Result.ACCOUNT_IN_USE:
			index = UiConnectInfo.CI_MI_ACCOUNT_LOCKED
			then = UiConnectInfo.CI_NS_LOGIN_WND
		Proto.Result.ACCOUNT_FROZEN:
			index = UiConnectInfo.CI_MI_ACCOUNT_FREEZE
			then = UiConnectInfo.CI_NS_LOGIN_WND
		Proto.Result.NO_GAME_TIME:
			index = UiConnectInfo.CI_MI_NOT_ENOUGH_ACCOUNT_POINT
			then = UiConnectInfo.CI_NS_LOGIN_WND
		Proto.Result.SERVER_BUSY:
			index = UiConnectInfo.CI_MI_CONNECT_SERV_BUSY
		Proto.Result.TIMEOUT:
			index = UiConnectInfo.CI_MI_CONNECT_TIMEOUT
		Proto.Result.VERSION_MISMATCH:
			index = UiConnectInfo.CI_MI_INVALID_PROTOCOLVERSION
			then = UiConnectInfo.CI_NS_INIT_WND
		Proto.Result.SERVER_SHUTDOWN, Proto.Result.ZONE_UNAVAILABLE:
			index = UiConnectInfo.CI_MI_SVRDOWN
		Game.LOGIN_NO_CONNECTION:
			index = UiConnectInfo.CI_MI_CONNECT_FAILED
		_:
			detail = text                # a result the old game has no sentence for: ours
	# a sentence the exported table lacks falls back to our own words as well
	if detail == "" and _info.info_string(index) == "":
		detail = text
	_open_info(index, then, detail)
	if _args.has("auto"):
		print("AUTO_LOGIN_FAILED %s" % text)
		Log.error("auto", "login failed", {"text": text})
		get_tree().quit(2)


func _on_char_list(chars: Array) -> void:
	if _pending_pid != 0:
		return
	_info.close()
	_show(_sel_player)
	_sel_player.open(chars, Game.max_chars)
	if _args.has("auto"):
		if chars.is_empty():
			Log.info("auto", "auto create")
			# --place=<map id>: the starting village (the Id of NativePlaceList.ini; 0 = the default map)
			_on_create_requested("Auto%d" % (randi() % 100000), 0, 0, int(str(_args.get("place", "0"))))
		else:
			Log.info("auto", "auto enter", {"pid": chars[0].pid, "name": chars[0].name})
			_on_enter_game(0)


func _on_enter_game(index: int) -> void:
	if index < 0 or index >= Game.chars.size():
		return
	_open_info(UiConnectInfo.CI_MI_ENTERING_GAME, UiConnectInfo.CI_NS_NONE)
	Game.enter_world(int(Game.chars[index].pid))


func _on_new_role() -> void:
	if Game.chars.size() >= Game.max_chars:
		return
	_sel_player.hide_window()
	_show(_native_place)
	_native_place.open()


func _on_create_requested(role_name: String, series: int, sex: int, place_id: int) -> void:
	_creating = true
	_open_info(UiConnectInfo.CI_MI_CREATING_ROLE, UiConnectInfo.CI_NS_NONE)
	Log.info("ui", "create character", {"name": role_name, "series": series, "sex": sex, "native_place": place_id})
	Game.create_char(role_name, series, sex, place_id)


func _on_char_created(ok: bool, result: int, summary: Dictionary) -> void:
	_creating = false
	if ok:
		# the old flow went from a new character straight into the game (LL_S_IN_GAME)
		_pending_pid = int(summary.get("pid", 0))
		_info.set_info_msg(UiConnectInfo.CI_MI_ENTERING_GAME)
		Game.enter_world(_pending_pid)
		return
	var reasons := {7: "Tên nhân vật không hợp lệ.", 5: "Tên nhân vật đã có người dùng.", 6: "Tài khoản đã đủ số nhân vật."}
	var text: String = reasons.get(result, "")
	_open_info(UiConnectInfo.CI_MI_ERROR_ROLE_NAME, UiConnectInfo.CI_NS_NEW_ROLE_WND, text)
	if _args.has("auto"):
		print("AUTO_CREATE_FAILED %d" % result)
		get_tree().quit(2)


func _on_entered_world(_info_dict: Dictionary) -> void:
	_pending_pid = 0
	get_tree().change_scene_to_file(GAME_SCENE)


func _on_enter_failed(result: int) -> void:
	_pending_pid = 0
	_open_info(UiConnectInfo.CI_MI_SVRDOWN, UiConnectInfo.CI_NS_SEL_ROLE_WND,
		"" if result == Proto.Result.ZONE_UNAVAILABLE else KLogin.result_text(result))


func _on_kicked(reason: int, text: String) -> void:
	_pending_pid = 0
	Game.last_notice = ""
	if KLogin.session_ends(reason):
		_open_info(UiConnectInfo.CI_MI_CONNECT_FAILED, UiConnectInfo.CI_NS_SEL_SERVER, KLogin.result_text(reason, text))
	else:
		_open_info(UiConnectInfo.CI_MI_SVRDOWN, UiConnectInfo.CI_NS_SEL_ROLE_WND, KLogin.result_text(reason, text))


func _on_connection_lost(reason: String) -> void:
	if reason == "logout" or reason == "back to login" or _info.visible and _info.next_step != UiConnectInfo.CI_NS_NONE:
		return
	if Game.last_login_result != Proto.Result.OK:
		return                           # the login answer already said why
	Game.last_notice = ""
	_pending_pid = 0
	Log.warn("ui", "connection lost in the login flow", {"reason": reason})
	_open_info(UiConnectInfo.CI_MI_CONNECT_FAILED, UiConnectInfo.CI_NS_SEL_SERVER)


func _say_later(what: String) -> void:
	Log.info("ui", "not built yet", {"window": what})


# ---------------------------------------------------------------- without a person

func _auto_login() -> void:
	var address := str(_args.get("server", "127.0.0.1:17100"))
	server = {"title": address, "address": address}
	Log.info("auto", "auto login", {"server": address, "account": _args.get("account", "")})
	_open_info(UiConnectInfo.CI_MI_CONNECTING, UiConnectInfo.CI_NS_NONE)
	Game.login(address, str(_args.get("account", "auto1")), str(_args.get("password", "auto")))


const SAMPLE_ROLES := [
	{"pid": 1, "name": "KiếmKhách", "level": 35, "series": 0, "sex": 0},
	{"pid": 2, "name": "TiểuLongNữ", "level": 72, "series": 2, "sex": 1},
	{"pid": 3, "name": "ĐườngMôn", "level": 8, "series": 1, "sex": 0},
]


func _show_for_shot(window_name: String) -> void:
	match window_name:
		"bat-dau":
			open_init()
		"dang-nhap":
			server = _servers.server(0, 0) if not _servers.regions.is_empty() else {"title": "Máy chủ", "address": "127.0.0.1:17100"}
			open_login()
		"thong-bao-ket-noi":
			bg.set_config("Login")
			_info.open(UiConnectInfo.CI_MI_CONNECT_FAILED, UiConnectInfo.CI_NS_SEL_SERVER)
		"chon-nhan-vat":
			_show(_sel_player)
			_sel_player.open(SAMPLE_ROLES.slice(0, int(_args.get("roles", "3"))), 3, 1)
		"chon-tan-thu-thon":
			_show(_native_place)
			_native_place.open()
		"tao-nhan-vat":
			_show(_new_player)
			_new_player.series = int(_args.get("series", "0"))
			_new_player.open(53)
			_new_player.set_role_name("KiếmKhách")
		_:
			open_sel_server()
			window_name = "chon-may-chu"
	if DisplayServer.get_name() == "headless":
		get_tree().quit(0)
		return
	for i in 6:
		await RenderingServer.frame_post_draw
	DirAccess.make_dir_recursive_absolute("user://logs")
	var path := "user://logs/ui_%s.png" % window_name
	var err := get_viewport().get_texture().get_image().save_png(path)
	print("shot %s -> %s%s" % [window_name, ProjectSettings.globalize_path(path), "" if err == OK else " FAILED"])
	get_tree().quit(0 if err == OK else 1)


# No layouts exported: plain controls, so the client can still log in for a test run.
func _fallback(why: String) -> void:
	Log.warn("ui", "no exported layouts, plain login", {"why": why})
	get_window().content_scale_size = _old_scale_size
	var plain: Node = load("res://scenes/UiLoginPlain.tscn").instantiate()
	add_child(plain)
	plain.set("notice", why)
