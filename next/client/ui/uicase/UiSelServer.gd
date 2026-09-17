# KUiSelServer - "Chọn Máy Chủ" (Ui\UiCase\UiSelServer.cpp; layout <theme>\UiNewLogin\选服务器.ini).
#
# Ported from gamecl.exe of VLTK 2.0, where the window differs from the JX1 source:
#   GetList (0x488510)        LeftList takes the first 14 REGIONS, RightList the ones after that;
#                             IpList takes the SERVERS of the region that is picked, NameBigger its
#                             name.  The region picked first is the one chosen last time.
#   GetServerList (0x489570)  region = LeftList's pick, or 14 + RightList's pick when LeftList has none
#   WndProc (0x489A90)        picking in one region list clears the other; a click in a region list
#                             makes IpList the "gray" one and the other way round (see KWndList);
#                             a double click on a server is the same as the OK button
#   OnLogin (0x4892C0)        with a server picked: remember it, save the choice, open the login
#                             window.  Without one: nothing.
#   OnCancel (0x4896B0)       back to the start menu
#   OnKeyDown (0x489740)      Left / Right move between the region lists and the server list, Up /
#                             Down move the pick and wrap from one region list into the other,
#                             Enter is OK, Escape is cancel
# A server whose title carries G_STR_SERVERLIST_STATUS1 "(Đầy)" is written in red, STATUS2
# "(Đề cử)" in green (PaintWindow of the list, 0x481BC7).
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndList := preload("res://ui/elem/KWndList.gd")
const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KLoginServer := preload("res://net/KLoginServer.gd")

const SCHEME := "chon-may-chu"
const REGIONS_PER_LIST := 14

signal server_chosen(server: Dictionary)   # {title, address}
signal cancelled

var login_bg := "Login"
var servers = null                          # KLoginServer
var region := 0                             # m_nRegion

var _left := KWndList.new()
var _right := KWndList.new()
var _ip := KWndList.new()
var _name := KWndText.new()
var _ok := KWndButton.new()
var _cancel := KWndButton.new()


# Builds the window; false when the layout has not been exported.
func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiSelServer"
	login_bg = ini.get_string("Main", "LoginBg", "Login")
	for pair in [[_left, "LeftList"], [_right, "RightList"], [_ip, "IpList"], [_name, "NameBigger"], [_ok, "Login"], [_cancel, "Cancel"]]:
		add_child(pair[0])
		pair[0].init_from(ini, pair[1])
	var strings = Assets.ui_data("chuoi-client")
	if strings != null:
		_ip.mark_red = str(strings.get("strings", {}).get("G_STR_SERVERLIST_STATUS1", ""))
		_ip.mark_green = str(strings.get("strings", {}).get("G_STR_SERVERLIST_STATUS2", ""))
	_left.item_selected.connect(_on_region_picked.bind(_left, _right))
	_right.item_selected.connect(_on_region_picked.bind(_right, _left))
	_ip.item_selected.connect(func(_i): _focus_lists(false))
	_ip.item_activated.connect(func(_i): _on_login())
	_ok.clicked.connect(_on_login)
	_cancel.clicked.connect(_on_cancel)
	focus_mode = Control.FOCUS_ALL
	return true


# KUiSelServer::OpenWindow + GetList
func open(list, choice: Dictionary) -> void:
	servers = list
	var titles: Array = servers.region_titles()
	_left.set_items(titles.slice(0, REGIONS_PER_LIST))
	_right.set_items(titles.slice(REGIONS_PER_LIST))
	region = int(choice.get("region", -1))
	if region < 0 or region >= titles.size():
		region = servers.advice_region
	if region >= REGIONS_PER_LIST:
		_right.set_cur_sel(region - REGIONS_PER_LIST)
		_left.set_cur_sel(-1)
	else:
		_left.set_cur_sel(region)
		_right.set_cur_sel(-1)
	_focus_lists(true)
	_fill_servers(str(choice.get("server", "")))
	show_window()
	grab_focus()


# GetServerList: the servers of the picked region, the one used last time picked again.
func _fill_servers(last_title: String = "") -> void:
	if _left.count() == 0 and _right.count() == 0:
		return
	region = _left.selected if _left.selected >= 0 else _right.selected + REGIONS_PER_LIST
	_ip.set_items(servers.server_titles(region))
	var pick: int = servers.find_server(region, last_title)
	_ip.set_cur_sel(maxi(pick, 0) if _ip.count() > 0 else -1)
	_name.text = _left.item_text(_left.selected) if _left.selected >= 0 else _right.item_text(_right.selected)


# true: the region lists are the ones in use and the server list is gray; false: the other way.
func _focus_lists(regions_active: bool) -> void:
	_left.set_gray(not regions_active)
	_right.set_gray(not regions_active)
	_ip.set_gray(regions_active)


func _on_region_picked(_index: int, picked: KWndList, other: KWndList) -> void:
	other.set_cur_sel(-1)
	_focus_lists(true)
	if picked.selected >= 0:
		_fill_servers()


func _on_login() -> void:
	if servers == null or _ip.selected < 0:
		return
	var chosen: Dictionary = servers.server(region, _ip.selected)
	if chosen.is_empty():
		return
	Log.info("ui", "server chosen", {"region": _name.text, "server": chosen.get("title", ""), "address": chosen.get("address", "")})
	KLoginServer.save_choice({"region": region, "server": chosen.get("title", "")})
	hide_window()
	server_chosen.emit(chosen)


func _on_cancel() -> void:
	hide_window()
	cancelled.emit()


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed:
		grab_focus()
	if not (event is InputEventKey) or not event.pressed:
		return
	var regions_active := _ip.is_gray()
	match event.keycode:
		KEY_ENTER, KEY_KP_ENTER:
			_on_login()
		KEY_ESCAPE:
			_on_cancel()
		KEY_LEFT:
			if not regions_active:
				_focus_lists(true)
		KEY_RIGHT:
			if regions_active:
				_focus_lists(false)
		KEY_UP:
			_step(-1, regions_active)
		KEY_DOWN:
			_step(1, regions_active)
		_:
			return
	accept_event()


# Up / Down: through the servers, or through the regions as ONE list that happens to be shown in
# two columns (from the last of LeftList into the first of RightList and round again).
func _step(by: int, regions_active: bool) -> void:
	if not regions_active:
		if _ip.count() > 0:
			_ip.set_cur_sel(posmod(_ip.selected + by, _ip.count()))
		return
	var total := _left.count() + _right.count()
	if total == 0:
		return
	var at := posmod(region + by, total)
	if at >= REGIONS_PER_LIST:
		_left.set_cur_sel(-1)
		_right.set_cur_sel(at - REGIONS_PER_LIST)
	else:
		_right.set_cur_sel(-1)
		_left.set_cur_sel(at)
	_fill_servers()
