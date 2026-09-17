# KUiSelPlayer - "Chọn nhân vật": the characters of the account stand side by side
# (Ui\UiCase\UiSelPlayer.cpp; layout <theme>\UiNewLogin\选游戏存档人物.ini; LoadScheme of gamecl.exe
# 2.0 at 0x49A040, UpdateData at 0x49B810 - the same arithmetic as the JX1 source).
#
# Up to three characters are shown.  [Player], [Name], [Level] and [PlayerInfoBg] describe ONE
# character; the others are copies moved sideways:
#     1 character    where the layout says
#     2 characters   x + Player2Pos_0  and  x + Player2Pos_1
#     3 characters   the SECOND one where the layout says (and Player3Pos_1 lower), the first at
#                    x + Player3Pos_0, the third at x + Player3Pos_2
# A figure is a sprite built by name (KUiSelPlayer::GetRoleImageName): "<prefix>_<series>_<sex>_<n>",
# n = 2 standing at the back (not picked), 1 stepping forward (played once when picked), 0 standing
# in front (picked).  The level reads "LV:%d" (0x49B1A9).
# Left / Right pick a character, Enter enters the game, Escape goes back to the server list.
#
# With more than three characters [Pre] / [Next] slide the row by one; with fewer than three the
# 2.0 window hides both.  Our gateway allows three (gateway.max_chars), so they stay hidden.
# The 2.0 layout has no [Del] section, so there is no delete button; [Transfer] (moving a character
# to another server, a service of the old publisher) is not offered.
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndImageBase := preload("res://ui/elem/KWndImage.gd")

const SCHEME := "chon-nhan-vat"
const MAX_SHOWN := 3

signal enter_game(index: int)
signal new_role
signal cancelled

var login_bg := "Login2"
var roles: Array = []                  # [{pid, name, level, series, sex}]
var selected := -1

var _ini: KUiScheme = null
var _ok := KWndButton.new()
var _cancel := KWndButton.new()
var _new := KWndButton.new()
var _pre := KWndButton.new()
var _next := KWndButton.new()
var _transfer := KWndButton.new()
var _life_time := KWndText.new()
var _players: Array = []               # of KWndButton: a figure is clicked like a button
var _names: Array = []
var _levels: Array = []
var _info_bgs: Array = []
var _child_pos := [0, 0, 0, 0]         # m_ChildPos: x of Player, Name, Level, PlayerInfoBg
var _x_offset := [0, 0, 0, 0]          # m_ChildWndXOffset
var _y_offset := 0                     # m_ChildWndYOffset
var _page_start := 0
var _just_clicked := false             # m_bJustClicked: the step-forward animation is playing


func load_scheme(screen: Vector2i) -> bool:
	_ini = KUiScheme.open(SCHEME)
	if _ini == null or not init_window(_ini, "SelRole", screen):
		return false
	name = "UiSelPlayer"
	login_bg = _ini.get_string("SelRole", "LoginBg", "Login2")
	for i in MAX_SHOWN:
		var bg := KWndImageBase.new()
		add_child(bg)
		bg.init_from(_ini, "PlayerInfoBg")
		_info_bgs.append(bg)
	for i in MAX_SHOWN:
		var p := KWndButton.new()
		add_child(p)
		p.init_from(_ini, "Player")
		p.exclude_trans = true         # a figure is clicked on its pixels, not on its 800x528 box
		p.clicked.connect(_on_figure.bind(i, false))
		p.double_clicked.connect(_on_figure.bind(i, true))
		_players.append(p)
	for i in MAX_SHOWN:
		var n := KWndText.new()
		add_child(n)
		n.init_from(_ini, "Name")
		_names.append(n)
		var l := KWndText.new()
		add_child(l)
		l.init_from(_ini, "Level")
		_levels.append(l)
	for pair in [[_ok, "Ok"], [_cancel, "Cancel"], [_new, "New"], [_pre, "Pre"], [_next, "Next"], [_transfer, "Transfer"], [_life_time, "LifeTime"]]:
		add_child(pair[0])
		pair[0].init_from(_ini, pair[1])
	_transfer.visible = false
	_x_offset[0] = _ini.get_integer("SelRole", "Player2Pos_0", 0)
	_x_offset[1] = _ini.get_integer("SelRole", "Player2Pos_1", 0)
	_x_offset[2] = _ini.get_integer("SelRole", "Player3Pos_0", 0)
	_y_offset = _ini.get_integer("SelRole", "Player3Pos_1", 0)
	_x_offset[3] = _ini.get_integer("SelRole", "Player3Pos_2", 0)
	_child_pos = [int(_players[0].position.x), int(_names[0].position.x), int(_levels[0].position.x), int(_info_bgs[0].position.x)]
	_ok.clicked.connect(_on_enter)
	_cancel.clicked.connect(_on_cancel)
	_new.clicked.connect(func(): new_role.emit())
	_pre.clicked.connect(_slide.bind(-1))
	_next.clicked.connect(_slide.bind(1))
	mouse_filter = Control.MOUSE_FILTER_PASS
	focus_mode = Control.FOCUS_ALL
	return true


func open(list: Array, max_roles: int, pick: int = 0) -> void:
	roles = list
	_page_start = 0
	selected = clampi(pick, 0, roles.size() - 1) if not roles.is_empty() else -1
	_new.enable(roles.size() < max_roles)
	update_data()
	show_window()
	grab_focus()


# KUiSelPlayer::UpdateData
func update_data() -> void:
	var shown := mini(roles.size() - _page_start, MAX_SHOWN)
	var parts := [_players, _names, _levels, _info_bgs]
	for k in parts.size():
		var base: int = _child_pos[k]
		var row: Array = parts[k]
		var y: float = row[0].position.y
		match shown:
			1:
				row[0].position = Vector2(base, y)
			2:
				row[0].position = Vector2(base + _x_offset[0], y)
				row[1].position = Vector2(base + _x_offset[1], y)
			3:
				row[1].position = Vector2(base, y + _y_offset)
				row[0].position = Vector2(base + _x_offset[2], y)
				row[2].position = Vector2(base + _x_offset[3], y)
	for i in MAX_SHOWN:
		var on := i < shown
		for row in parts:
			row[i].visible = on
		if not on:
			continue
		var role: Dictionary = roles[_page_start + i]
		_names[i].text = str(role.get("name", ""))
		_levels[i].text = "LV:%d" % int(role.get("level", 1))
		_players[i].set_image(_ini.portrait(int(role.get("series", 0)), int(role.get("sex", 0)), 2))
	var more := roles.size() > MAX_SHOWN
	_pre.visible = more
	_next.visible = more
	_ok.enable(shown > 0 and selected >= 0)
	if shown > 0:
		_select(clampi(selected - _page_start, 0, shown - 1))


# KUiSelPlayer::OnSelectPlayer: the one picked steps forward, the one picked before steps back.
func _select(slot: int) -> void:
	var index := _page_start + slot
	if selected != index and selected >= _page_start and selected < _page_start + MAX_SHOWN:
		var was: Dictionary = roles[selected]
		_players[selected - _page_start].set_image(_ini.portrait(int(was.get("series", 0)), int(was.get("sex", 0)), 2))
	selected = index
	var role: Dictionary = roles[index]
	_players[slot].set_image(_ini.portrait(int(role.get("series", 0)), int(role.get("sex", 0)), 1))
	_just_clicked = true
	_ok.enable(true)


func _process(delta: float) -> void:
	super._process(delta)
	for i in MAX_SHOWN:
		if not _players[i].visible:
			continue
		var looped: bool = _players[i].next_frame()
		if _just_clicked and looped and _page_start + i == selected:
			var role: Dictionary = roles[selected]
			_players[i].set_image(_ini.portrait(int(role.get("series", 0)), int(role.get("sex", 0)), 0))
			_just_clicked = false


func _on_figure(slot: int, double_click: bool) -> void:
	if _page_start + slot >= roles.size():
		return
	_select(slot)
	if double_click:
		_on_enter()


func _slide(by: int) -> void:
	var start := clampi(_page_start + by, 0, maxi(roles.size() - MAX_SHOWN, 0))
	if start == _page_start:
		return
	_page_start = start
	selected = clampi(selected, _page_start, _page_start + MAX_SHOWN - 1)
	update_data()


func _on_enter() -> void:
	if selected >= 0 and selected < roles.size():
		hide_window()
		enter_game.emit(selected)


func _on_cancel() -> void:
	hide_window()
	cancelled.emit()


func _gui_input(event: InputEvent) -> void:
	if not (event is InputEventKey) or not event.pressed:
		return
	match event.keycode:
		KEY_ENTER, KEY_KP_ENTER:
			_on_enter()
		KEY_ESCAPE:
			_on_cancel()
		KEY_LEFT:
			if selected > _page_start:
				_select(selected - 1 - _page_start)
		KEY_RIGHT, KEY_SPACE:
			if selected < mini(_page_start + MAX_SHOWN, roles.size()) - 1:
				_select(selected + 1 - _page_start)
		_:
			return
	accept_event()
