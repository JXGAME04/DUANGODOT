# Checks the login flow rebuilt from the old client's layouts: that every window builds, that the
# numbers the .ini gave come out where the old arithmetic puts them, and that the parts behave the
# way the old code did (a button clicks on release, a check box on press, Kim is for men...).
# A scene, not a SceneTree script, because the windows need the Assets, Log and Game autoloads:
#
#   godot --headless --path client tests/UiCheck.tscn
#
# Exit code 0 when every check passes, 1 otherwise.  Without exported game data (a fresh clone,
# CI) only the checks that need none run - the scripts must compile and the pure parts must work.
extends Node

const KFont := preload("res://ui/KFont.gd")
const KText := preload("res://ui/KText.gd")
const KUiScheme := preload("res://ui/KUiScheme.gd")
const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KWndList := preload("res://ui/elem/KWndList.gd")
const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndShowAnimate := preload("res://ui/elem/KWndShowAnimate.gd")
const KLoginServer := preload("res://net/KLoginServer.gd")
const UiSelServer := preload("res://ui/uicase/UiSelServer.gd")
const UiLogin := preload("res://ui/uicase/UiLogin.gd")
const UiInit := preload("res://ui/uicase/UiInit.gd")
const UiConnectInfo := preload("res://ui/uicase/UiConnectInfo.gd")
const UiSelPlayer := preload("res://ui/uicase/UiSelPlayer.gd")
const UiSelNativePlace := preload("res://ui/uicase/UiSelNativePlace.gd")
const UiNewPlayer := preload("res://ui/uicase/UiNewPlayer.gd")

const SCREEN := Vector2i(1024, 768)

var _failed := 0
var _passed := 0
var _host: Control


func check(cond: bool, what: String) -> void:
	if cond:
		_passed += 1
	else:
		_failed += 1
		printerr("FAIL: " + what)


func _ready() -> void:
	for path in ["res://scenes/UiShell.gd", "res://scenes/UiLoginPlain.gd", "res://scenes/UiGame.gd"]:
		check(ResourceLoader.load(path, "GDScript", ResourceLoader.CACHE_MODE_IGNORE) != null, "%s compiles" % path)
	_check_text_tags()
	_check_server_list()
	_check_button()
	_check_plain_list()
	if not FileAccess.file_exists(Assets.assets_root() + "/ui/chon-may-chu/bo-cuc.json"):
		print("ui layouts not exported (python tools/dev.py assets): only the checks without game data ran")
		_finish()
		return
	KWndShowAnimate.animate = false
	_host = Control.new()
	_host.size = Vector2(SCREEN)
	add_child(_host)
	_check_scheme()
	_check_font()
	_check_init()
	_check_sel_server()
	_check_login()
	_check_connect_info()
	_check_sel_player()
	_check_native_place()
	_check_new_player()
	_finish()


func _finish() -> void:
	print("ui checks: %d passed, %d failed" % [_passed, _failed])
	get_tree().quit(0 if _failed == 0 else 1)


# ---------------------------------------------------------------- without game data

func _check_text_tags() -> void:
	var runs := KText.parse("<color=Gold>Kim:<color> Nam<enter>dòng hai <bclr=0xFF0000>viền đỏ<bclr> hết")
	check(runs.size() == 5, "five runs: %d" % runs.size())
	if runs.size() == 5:
		check(runs[0]["text"] == "Kim:" and runs[0]["color"] == Color8(243, 194, 90), "Gold is the 2.0 engine's 243,194,90: %s" % str(runs[0]))
		check(runs[1]["text"] == " Nam" and runs[1]["color"] == null and runs[1]["br"], "<color> alone gives the window's colour back, <enter> ends the line")
		check(runs[3]["border"] == Color8(255, 0, 0) and runs[4]["border"] == null, "<bclr=0xRRGGBB> and back")
	check(KText.plain("a<color=red>b<color>c") == "abc", "plain() drops the tags")
	check(KText.parse("1 < 2 and <b>").size() == 1, "a '<' that is no tag of the game stays text")
	check(KText.color_of("255,253,122", Color.BLACK) == Color8(255, 253, 122), "a colour key is r,g,b")
	check(KText.color_of("12,15", Color.RED) == Color.RED, "a malformed colour falls back")
	check(KUiScheme.leading_int("5000 ;ms") == 5000 and KUiScheme.leading_int("", 7) == 7 and KUiScheme.leading_int("-38") == -38,
		"GetInteger reads the leading number")


func _check_server_list() -> void:
	var list = KLoginServer.new()
	var servers := [{"title": "Tung Sơn", "address": "a:1"}, {"title": "", "address": "b:2"}, {"title": "Hoa Sơn", "address": ""}, {"title": "Vu Sơn", "address": "c:3"}]
	var count: int = list.take({"advice_region": 9, "regions": [{"title": "Cụm Sơn 1", "servers": servers}, {"title": "", "servers": servers}],
		"user_notify_url": "https://example.org/dieu-khoan", "privacy_notify_url": "javascript:alert(1)"})
	check(count == 2 and list.regions.size() == 1, "a region without a name and servers without a title or an address are skipped: %d servers, %d regions" % [count, list.regions.size()])
	check(list.advice_region == 0, "AdviceRegion outside the list falls back to the first")
	check(list.server_titles(0) == ["Tung Sơn", "Vu Sơn"] and list.find_server(0, "Vu Sơn") == 1 and list.find_server(0, "?") == -1, "titles and lookup")
	check(list.server(0, 1).get("address", "") == "c:3" and list.server(3, 0).is_empty(), "server() by region and index")
	check(list.links.has("user_notify_url") and not list.links.has("privacy_notify_url"), "only http(s) links are kept")


# A button the way KWndButton::WndProc drove it, without a picture.
func _check_button() -> void:
	var b := KWndButton.new()
	add_child(b)
	b.size = Vector2(100, 30)
	b.animate_over = true
	b.over_start_frame = 2
	var clicks := [0]
	b.clicked.connect(func(): clicks[0] += 1)
	b._mouse_over()
	check(b.over and b.frame == 2, "under the mouse it shows OverFrame")
	b._on_left_down()
	check(b.down and b.frame == 1 and clicks[0] == 0, "a press shows Down and does not click yet")
	b._on_left_up()
	check(not b.down and clicks[0] == 1, "the release clicks")
	b._on_left_down()
	b._mouse_leave()
	b._on_left_up()
	check(clicks[0] == 1 and b.frame == 0, "a press that leaves the button does not click")
	var box := KWndButton.new()
	add_child(box)
	box.is_checkbox = true
	var states: Array = []
	box.toggled.connect(func(on): states.append(on))
	box._on_left_down()
	box._on_left_up()
	box._on_left_down()
	check(states == [true, false], "a check box flips on every PRESS: %s" % str(states))
	b.enable(false)
	b._gui_input(InputEventMouseButton.new())
	check(clicks[0] == 1, "a disabled button takes no input")
	b.queue_free()
	box.queue_free()


func _check_plain_list() -> void:
	var l := KWndList.new()
	add_child(l)
	l.size = Vector2(120, 60)
	l.font_size = 14
	l.set_items(["a", "b", "c", "d", "e"])
	check(l.row_pitch() == 15 and l.visible_rows() == 4, "a plain list has rows of Font + 1")
	check(l.item_at(Vector2(5, 16)) == 1 and l.item_at(Vector2(5, 59)) == 3 and l.item_at(Vector2(500, 5)) == -1, "hit test by row")
	l.set_cur_sel(9)
	check(l.selected == -1, "SetCurSel outside the list picks nothing")
	l.queue_free()


# ---------------------------------------------------------------- with the exported layouts

func _check_scheme() -> void:
	var ini: KUiScheme = KUiScheme.open("chon-may-chu")
	check(ini != null and ini.screen_size() == SCREEN, "the theme was drawn for 1024x768")
	check(ini.get_integer("LeftList", "Font") == 14 and ini.get_integer("leftlist", "nokey", -5) == -5, "sections and keys ignore case; a missing key gives the default")
	check(ini.get_integer2("Main", "StartPos") == Vector2i(800, 0), "GetInteger2 reads a,b")
	check(ini.get_color("LeftList", "SelColor", Color.BLACK) == Color8(255, 253, 122), "colours")
	var row = ini.image("LeftList", "SprImg")
	check(row != null and row.frame_count() == 2 and row.box == Vector2i(150, 20), "the row picture has its two frames: %s" % str(row.box if row != null else null))
	check(KUiScheme.open("khong-co-man-nay") == null, "a layout that was not exported is null")


func _check_font() -> void:
	var f14 = KFont.of(14)
	check(f14 != null and f14.advance == 7 and f14.cell == Vector2i(14, 14), "Font=14 is the game's 14x14 font at 7 pixels a letter")
	check(f14 != null and f14.width_of("Tung Sơn") == 56, "text is measured as letters * size / 2")
	var f13 = KFont.of(13)
	check(f13 != null and f13.size == 12, "Font=13 draws with the glyphs of 12 (fontsetting.ini: 2_File=#12)")
	check(f14 != null and f14.letters.has_char("ơ".unicode_at(0)) and f14.outline.has_char("ơ".unicode_at(0)), "the Vietnamese letters are in both layers")


func _check_init() -> void:
	var w := UiInit.new()
	_host.add_child(w)
	check(w.load_scheme(SCREEN), "the first menu builds")
	var enter := w.get_node_or_null("EnterGame")
	check(enter != null and enter.position == Vector2(418, 350) and enter.image != null and enter.image.frame_count() >= 3, "Bắt Đầu Trò Chơi at 418,350 with its frames")
	var fired := [false]
	w.enter_game.connect(func(): fired[0] = true)
	w.open()
	var key := InputEventKey.new()
	key.keycode = KEY_ENTER
	key.pressed = true
	w._gui_input(key)
	check(fired[0], "Enter starts the game")
	w.queue_free()


func _check_sel_server() -> void:
	var w := UiSelServer.new()
	_host.add_child(w)
	check(w.load_scheme(SCREEN), "the server window builds")
	check(w.position == Vector2(225, 186), "PositionType=1 centres 574x396 on 1024x768 at 225,186: %s" % w.position)
	check(w.login_bg == "Login", "it asks for the Login backdrop")
	var list = KLoginServer.new()
	var regions: Array = []
	for i in 17:
		regions.append({"title": "Cụm %d" % i, "servers": [{"title": "Máy %d-1" % i, "address": "h:%d" % (100 + i)}, {"title": "Máy %d-2 (Đầy)" % i, "address": "h:%d" % (200 + i)}]})
	list.take({"advice_region": 2, "regions": regions})
	w.open(list, {"region": -1, "server": ""})
	check(w._left.count() == 14 and w._right.count() == 3, "fourteen regions on the left, the rest on the right: %d + %d" % [w._left.count(), w._right.count()])
	check(w._left.selected == 2 and w._right.selected == -1 and w.region == 2, "AdviceRegion is picked first")
	check(w._ip.items == ["Máy 2-1", "Máy 2-2 (Đầy)"] and w._ip.selected == 0 and w._name.text == "Cụm 2", "the servers and the name of that region")
	check(not w._left.is_gray() and w._ip.is_gray(), "the region lists are the ones in use at first")
	check(w._ip.mark_red == "(Đầy)", "a full server is marked with the game's own word: '%s'" % w._ip.mark_red)
	check(w._left.position == Vector2(11, 41) and w._ip.position == Vector2(337, 70) and w._left.row_pitch() == 18, "lists where the layout puts them, rows 18 pixels apart")
	w.open(list, {"region": 15, "server": "Máy 15-2 (Đầy)"})
	check(w._left.selected == -1 and w._right.selected == 1 and w._ip.selected == 1, "the choice of last time comes back, in the right-hand list")
	w._left.selected = 3
	w._left.item_selected.emit(3)
	check(w._right.selected == -1 and w.region == 3 and w._name.text == "Cụm 3", "picking on the left clears the right")
	var chosen := [{}]
	w.server_chosen.connect(func(s): chosen[0] = s)
	w._ip.set_cur_sel(1)
	w._on_login()
	check(chosen[0].get("address", "") == "h:203", "OK hands over the server: %s" % str(chosen[0]))
	w.queue_free()


func _check_login() -> void:
	var w := UiLogin.new()
	_host.add_child(w)
	check(w.load_scheme(SCREEN), "the login window builds")
	check(w.position == Vector2(292, 251), "440x267 centred: %s" % w.position)
	check(w._account.position == Vector2(228, 88) and w._account.max_len == 80 and w._account.font_size == 14, "the account box")
	check(w._password.password and w._password.ascii_only and not w._account.ascii_only, "the password box hides and takes plain ASCII only (Type=1)")
	check(w._remember_txt.text == "Nhớ tài khoản" and w._sel_server.label == "Đổi Server", "the 2.0 texts came through from TCVN3")
	w.open({"title": "Tung Sơn", "address": "h:1"}, {"remember": true, "account": "kiemkhach", "agree": false})
	check(w._account.get_text() == "kiemkhach" and w._server_name.text == "Tung Sơn", "the remembered account and the chosen server")
	var asked: Array = []
	var missing := [0]
	w.login_requested.connect(func(a, p): asked.append([a, p]))
	w.input_missing.connect(func(): missing[0] += 1)
	w._password.set_text("matkhau")
	w._on_login()
	check(asked.is_empty() and w._reminders["HaveNotAgree"].visible, "no login before the agreement box is ticked")
	w._agree.check(true)
	w._on_login()
	check(asked == [["kiemkhach", "matkhau"]] and w._password.get_text() == "", "login hands over both and the password does not outlive the window")
	w.open({"title": "x", "address": "h:1"}, {"agree": true})
	w._on_login()
	check(missing[0] == 1, "an empty box is answered by sentence 15")
	w._password.input.text = "paợss"
	w._password._on_input_changed(w._password.input.text)
	check(w._password.get_text() == "pass", "letters outside ASCII do not enter the password: '%s'" % w._password.get_text())
	w.queue_free()


func _check_connect_info() -> void:
	var w := UiConnectInfo.new()
	_host.add_child(w)
	check(w.load_scheme(SCREEN), "the connect box builds")
	check(w.info_string(1) == "Hiện đang kết nối với máy chủ", "sentence 1 is the game's own: '%s'" % w.info_string(1))
	var went := [-1]
	w.confirmed.connect(func(step): went[0] = step)
	w.open(UiConnectInfo.CI_MI_ENTERING_GAME, UiConnectInfo.CI_NS_NONE)
	check(not w._confirm.visible, "no button while entering the game")
	w.open(UiConnectInfo.CI_MI_ACCOUNT_PWD_ERROR, UiConnectInfo.CI_NS_LOGIN_WND)
	check(w._confirm.visible, "a failure has the back button")
	w._on_confirm()
	check(went[0] == UiConnectInfo.CI_NS_LOGIN_WND and not w.visible, "and it goes where it was told")
	w.open(UiConnectInfo.CI_MI_CONNECT_FAILED, UiConnectInfo.CI_NS_SEL_SERVER, "lời của gateway")
	check(w.message == "lời của gateway", "our own words replace the table's when given")
	w.queue_free()


func _check_sel_player() -> void:
	var w := UiSelPlayer.new()
	_host.add_child(w)
	check(w.load_scheme(SCREEN), "the character window builds")
	check(w.login_bg == "Login2" and w.position == Vector2(112, 168), "SelRole sits at 112,168 on the Login2 backdrop: %s" % w.position)
	var roles := [{"pid": 1, "name": "Một", "level": 5, "series": 0, "sex": 0}, {"pid": 2, "name": "Hai", "level": 60, "series": 2, "sex": 1}, {"pid": 3, "name": "Ba", "level": 9, "series": 4, "sex": 1}]
	w.open(roles.slice(0, 1), 3)
	var base: float = w._players[0].position.x
	check(w._players[0].visible and not w._players[1].visible and w._levels[0].text == "LV:5", "one character where the layout says, level as LV:%d")
	w.open(roles.slice(0, 2), 3)
	check(w._players[0].position.x == base - 160 and w._players[1].position.x == base + 160, "two characters at Player2Pos: %s %s" % [w._players[0].position.x, w._players[1].position.x])
	w.open(roles, 3, 1)
	check(w._players[1].position.x == base and w._players[0].position.x == base - 250 and w._players[2].position.x == base + 250, "three: the second in the middle, the others at Player3Pos")
	check(w.selected == 1 and w._names[1].text == "Hai" and not w._pre.visible and not w._next.visible, "the pick, and no paging for three")
	check(w._players[1].image != null and w._players[1].image == w._ini.portrait(2, 1, 1), "the picked one steps forward (figure 1)")
	check(w._players[0].image == w._ini.portrait(0, 0, 2), "the others stand back (figure 2)")
	check(w._new.disabled, "three characters: no fourth")
	var entered := [-1]
	w.enter_game.connect(func(i): entered[0] = i)
	w._on_figure(2, true)
	check(entered[0] == 2, "a double click enters with that character")
	w.queue_free()


func _check_native_place() -> void:
	var w := UiSelNativePlace.new()
	_host.add_child(w)
	check(w.load_scheme(SCREEN), "the village window builds")
	check(w.places.size() == 8 and int(w.places[0]["id"]) == 53, "eight villages, the first is Ba Lăng Huyện (53): %d" % w.places.size())
	w.open(20)
	check(w._list.selected == 1 and w._desc.text == str(w.places[1]["desc"]) and w._place_img.image != null, "opening with an id picks that village, its picture and its text")
	var got := [0]
	w.place_chosen.connect(func(id): got[0] = id)
	w._on_ok()
	check(got[0] == 20, "OK hands over the village's Id")
	w.queue_free()


func _check_new_player() -> void:
	var w := UiNewPlayer.new()
	_host.add_child(w)
	check(w.load_scheme(SCREEN), "the create window builds")
	var tops := [78, 116, 154, 192, 230]
	for i in 5:
		var b = w._series_btn[i]
		check(b.position == Vector2(0, tops[i]) and b.size == Vector2(103, 38) and b.is_checkbox, "%s is a two-state tab at 0,%d" % [UiNewPlayer.SERIES_SECTIONS[i], tops[i]])
	check(w._male.position.x == -38 and w._female.position.x == 262, "the man at -38, the woman at 262")
	w.open(53)
	check(w.series == 0 and w.sex == 0 and w._female.disabled and not w._male.disabled, "Kim is for men only")
	check(w._property_bg.image != null and w._property_show.text.begins_with("<color=Gold>Kim"), "the element's name picture and its description")
	check(w._male.image == w._ini.portrait(0, 0, 1) and w._female.image == w._ini.portrait(0, 1, 2), "the picked sex steps forward, the other stands back")
	w._on_series(true, 2)
	check(w.sex == 1 and w._male.disabled and not w._female.disabled and w._series_btn[2].is_checked() and not w._series_btn[0].is_checked(), "Thủy is for women only")
	w._on_series(true, 3)
	w._on_sex(0)
	check(w.sex == 0 and not w._male.disabled and not w._female.disabled, "Hỏa takes both")
	var bad: Array = []
	var asked: Array = []
	w.name_invalid.connect(func(n): bad.append(n))
	w.create_requested.connect(func(n, s, x, p): asked.append([n, s, x, p]))
	w.set_role_name("Kiếm Khách")
	w._on_ok()
	w.open(53)
	w.set_role_name("K")
	w._on_ok()
	check(bad == [17, 18], "a blank is sentence 17, a name too short sentence 18: %s" % str(bad))
	w.open(53)
	w._on_series(true, 3)
	w.set_role_name("KiếmKhách")
	w._on_ok()
	check(asked == [["KiếmKhách", 3, 0, 53]], "create hands over name, element, sex and village: %s" % str(asked))
	w.queue_free()
