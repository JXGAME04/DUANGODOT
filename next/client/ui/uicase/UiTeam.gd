# UiTeam - the team window of the VLTK 2.0 client (KUiTeamManage; OpenWindow 0x004ADAF0 loads "%s\队伍管理.ini", the
# widgets at 0x004AD930, WndProc 0x004AE0D0, the refresh 0x004AE300; docs/CLIENT-2.0.md §21).
#   - [Main] 404 x 253 at (230,170) on 组队界面底板.spr; [txtTitle] "Tổ Đội", [txtOurTeam] "Đội mình" over [MemberList],
#     [txtAroundPlayer] "Lân cận" over [NearbyList] (+ [NearbyScroll]), [txtLeaderAbility] "Tài lãnh đạo" + [LeaderAbility]
#     "%d" (0x00468BD0: the leadership level GetGameData 0x3fc hands back - core+0x11aac, the level the client works out
#     of the lead exp with its own level_lead_exp.txt), [txtSearch] + [InputEdit] (an edit the 2.0 WndProc never reads).
#   - buttons (0x004AE162..): [Invite] 0x004ADE00 - out of a team OperationRequest(5) creates one first, then (7, the
#     picked nearby player); [Kick] 0x004ADDD0 (8, the picked member); [Appoint] 0x004ADDA0 (6, the picked member);
#     [Refresh] 0x004ADE50 rebuilds the nearby list (GetGameData 0x3fa: the players around that are not in my team,
#     0x0066B180 - a player of another camp is left out while my camp is 0); [Leave] / [Dismiss] share one place and
#     both send OperationRequest(9) - the refresh shows Dismiss to a captain, Leave to anyone else (enabled in a team);
#     [CloseTeam] a check box = the team is open (0x004AE39C), a click sends (0xa, checked); [Cancel] closes.
#   - a member picked (0x004ADBD0): Kick / Appoint work on anyone but oneself; a nearby player picked (0x004ADC70):
#     Invite works when I lead a team or have none.
# The zone speaks the 0x53 sub-commands directly (Game.team_request); the window shows Game.team.
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KWndLabeledButton := preload("res://ui/elem/KWndLabeledButton.gd")
const KWndList := preload("res://ui/elem/KWndList.gd")
const KWndEdit := preload("res://ui/elem/KWndEdit.gd")
const KWndImage := preload("res://ui/elem/KWndImage.gd")
const Proto := preload("res://proto/jx_pb.gd")

const SCHEME := "to-doi"
const TEXTS := ["txtTitle", "txtLeaderAbility", "txtSearch", "txtOurTeam", "txtAroundPlayer"]
const LABELED := ["Invite", "Kick", "Appoint", "Refresh", "Leave", "Dismiss"]

signal closed

var _ability := KWndText.new()
var _edit := KWndEdit.new()
var _members := KWndList.new()
var _nearby := KWndList.new()
var _scroll := KWndImage.new()
var _close_team := KWndLabeledButton.new()
var _cancel := KWndButton.new()
var _buttons := {}
var _texts: Array = []
var _member_ids: Array = []    # entity id per row of the member list
var _nearby_ids: Array = []    # entity id per row of the nearby list


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiTeam"
	for section in TEXTS:
		var t := KWndText.new()
		add_child(t)
		t.init_from(ini, section)
		t.text = ini.get_string(section, "Text", "")
		_texts.append(t)
	add_child(_ability)
	_ability.init_from(ini, "LeaderAbility")
	add_child(_edit)
	_edit.init_from(ini, "InputEdit")
	add_child(_members)
	_members.init_from(ini, "MemberList")
	_members.item_selected.connect(func(_i): _refresh_buttons())
	add_child(_nearby)
	_nearby.init_from(ini, "NearbyList")
	_nearby.item_selected.connect(func(_i): _refresh_buttons())
	add_child(_scroll)
	_scroll.init_from(ini, "NearbyScroll_Btn")
	_scroll.position = Vector2(ini.get_integer("NearbyScroll", "Left", 385), ini.get_integer("NearbyScroll", "Top", 56))
	_scroll.mouse_filter = Control.MOUSE_FILTER_IGNORE
	for section in LABELED:
		var b := KWndLabeledButton.new()
		add_child(b)
		b.init_from(ini, section)
		_buttons[section] = b
	_buttons["Invite"].clicked.connect(_on_invite)
	_buttons["Kick"].clicked.connect(_on_kick)
	_buttons["Appoint"].clicked.connect(_on_appoint)
	_buttons["Refresh"].clicked.connect(refresh_nearby)
	_buttons["Leave"].clicked.connect(_on_leave)
	_buttons["Dismiss"].clicked.connect(_on_leave)
	add_child(_close_team)
	_close_team.init_from(ini, "CloseTeam")
	_close_team.toggled.connect(_on_close_team)
	add_child(_cancel)
	_cancel.init_from(ini, "Cancel")
	_cancel.clicked.connect(close_window)
	Game.team_changed.connect(refresh)
	Game.entity_despawn.connect(func(_ids): if visible: refresh_nearby())
	Game.entity_spawn.connect(func(_list): if visible: refresh_nearby())
	refresh()
	return true


func open_window() -> void:
	refresh()
	refresh_nearby()
	show_window()


func close_window() -> void:
	hide_window()
	closed.emit()


func toggle_window() -> void:
	if visible:
		close_window()
	else:
		open_window()


func member_count() -> int:
	return _member_ids.size()


func nearby_count() -> int:
	return _nearby_ids.size()


# KUiTeamManage::UpdateTeamInfo 0x004AE300: the state, the leadership, the members (the captain first), the buttons
func refresh() -> void:
	var t: Dictionary = Game.team
	_ability.text = str(int(t.get("lead_level", 1)))
	var names: Array = []
	_member_ids = []
	if bool(t.get("in_team", false)):
		var leader: Dictionary = t.get("leader", {})
		if not leader.is_empty():
			names.append(str(leader.name))
			_member_ids.append(int(leader.id))
		for m in t.get("members", []):
			names.append(str(m.name))
			_member_ids.append(int(m.id))
	_members.set_items(names)
	_members.set_cur_sel(-1)
	_close_team.check(int(t.get("state", 0)) != 0)
	_refresh_buttons()


# GetGameData 0x3fa / 0x0066B180: the players around that are not in my team; a player of another camp is left out while
# my own camp is 0 (the rule of KTeam::AddMember)
func refresh_nearby() -> void:
	var own = Game.entities.get(Game.entity_id, {})
	var my_camp := int(own.get("camp", 0)) if own != null else 0
	var names: Array = []
	_nearby_ids = []
	for id in Game.entities:
		if int(id) == Game.entity_id:
			continue
		var e: Dictionary = Game.entities[id]
		if int(e.get("type", 0)) != Proto.EntityType.ENTITY_PLAYER:
			continue
		if int(e.get("camp", 0)) != 0 and my_camp == 0:
			continue
		if Game.is_team_mate(int(id)):
			continue
		names.append(str(e.get("name", "")))
		_nearby_ids.append(int(id))
	_nearby.set_items(names)
	_nearby.set_cur_sel(-1)
	_refresh_buttons()


# 0x004AE397..0x004AE419 and the selection handlers 0x004ADBD0 / 0x004ADC70
func _refresh_buttons() -> void:
	var t: Dictionary = Game.team
	var in_team := bool(t.get("in_team", false))
	var captain := in_team and bool(t.get("captain", false))
	_buttons["Dismiss"].visible = captain
	_buttons["Leave"].visible = not captain
	_buttons["Leave"].enable(in_team)
	_close_team.enable(captain)
	var member_sel := _members.selected
	var picked_other: bool = member_sel >= 0 and member_sel < _member_ids.size() and int(_member_ids[member_sel]) != Game.entity_id
	_buttons["Kick"].enable(captain and picked_other)
	_buttons["Appoint"].enable(captain and picked_other)
	var nearby_sel := _nearby.selected
	_buttons["Invite"].enable(nearby_sel >= 0 and nearby_sel < _nearby_ids.size() and (captain or not in_team))


# 0x004ADE00: no team yet -> OperationRequest(5) creates one (the 0x53 sub 2), then (7, the picked player) invites (sub 10)
func _on_invite() -> void:
	var sel := _nearby.selected
	if sel < 0 or sel >= _nearby_ids.size():
		return
	if not bool(Game.team.get("in_team", false)):
		Game.team_request(Proto.TeamCmd.TEAM_CREATE)
	Game.team_request(Proto.TeamCmd.TEAM_INVITE, _nearby_ids[sel])


func _on_kick() -> void:
	var sel := _members.selected
	if sel < 0 or sel >= _member_ids.size():
		return
	Game.team_request(Proto.TeamCmd.TEAM_KICK, _member_ids[sel])


func _on_appoint() -> void:
	var sel := _members.selected
	if sel < 0 or sel >= _member_ids.size():
		return
	Game.team_request(Proto.TeamCmd.TEAM_CHANGE_CAPTAIN, _member_ids[sel])


# OperationRequest(9) 0x005F7070: the 0x53 sub 6 - the zone dismisses when a captain leaves (KTeam::DeleteMember)
func _on_leave() -> void:
	if bool(Game.team.get("in_team", false)):
		Game.team_request(Proto.TeamCmd.TEAM_LEAVE)


# OperationRequest(0xa, 0, checked) 0x005FA7C0: the 0x53 sub 3 with the flag
func _on_close_team(checked: bool) -> void:
	Game.team_request(Proto.TeamCmd.TEAM_OPEN_CLOSE, 0, 1 if checked else 0)


func _gui_input(event: InputEvent) -> void:
	# the wheel over the nearby list scrolls it (the 2.0 list has a slider; this one has the wheel)
	if event is InputEventMouseButton and event.pressed and _nearby.get_rect().has_point(event.position):
		if event.button_index == MOUSE_BUTTON_WHEEL_DOWN and _nearby.top_index + _nearby.visible_rows() < _nearby.count():
			_nearby.top_index += 1
			_nearby.queue_redraw()
			accept_event()
		elif event.button_index == MOUSE_BUTTON_WHEEL_UP and _nearby.top_index > 0:
			_nearby.top_index -= 1
			_nearby.queue_redraw()
			accept_event()
