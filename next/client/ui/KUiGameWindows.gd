# KUiGameWindows - the windows of the game screen (the part of Ui\UiBase.cpp that opens and
# closes the item / status windows and keeps the item on the cursor).
#
# A CanvasLayer over the world: the bag (UiItem), the character window (UiStatus), the tooltip
# (UiMouseHover) and the item on the cursor (KUiDraggedObject).  The windows are laid out for the
# theme's screen (1024x768) and drawn at that size on whatever the viewport is; they open at the
# Left/Top of their layout.  Keys: I the bag, C the character window (Alt+ the same), Escape puts
# a lifted item back and closes the window on top.
extends CanvasLayer

const UiItem := preload("res://ui/uicase/UiItem.gd")
const UiStatus := preload("res://ui/uicase/UiStatus.gd")
const UiSkills := preload("res://ui/uicase/UiSkills.gd")
const UiMouseHover := preload("res://ui/uicase/UiMouseHover.gd")
const UiControlBar := preload("res://ui/uicase/UiControlBar.gd")
const UiPlayerBar := preload("res://ui/uicase/UiPlayerBar.gd")
const UiSkillTree := preload("res://ui/uicase/UiSkillTree.gd")
const UiSkillState := preload("res://ui/uicase/UiSkillState.gd")
const UiTeam := preload("res://ui/uicase/UiTeam.gd")
const UiInformation := preload("res://ui/uicase/UiInformation.gd")
const UiTrade := preload("res://ui/uicase/UiTrade.gd")
const UiMsgCentrePad := preload("res://ui/uicase/UiMsgCentrePad.gd")
const UiMsgSel := preload("res://ui/uicase/UiMsgSel.gd")
const UiInformation2 := preload("res://ui/uicase/UiInformation2.gd")
const UiNpcDescribe := preload("res://ui/uicase/UiNpcDescribe.gd")
const UiSysMsg := preload("res://ui/uicase/UiSysMsg.gd")
const UiGiveItem := preload("res://ui/uicase/UiGiveItem.gd")
const KWndPopupMenu := preload("res://ui/elem/KWndPopupMenu.gd")
const KUiShortcut := preload("res://ui/KUiShortcut.gd")
const KUiShortcutItem := preload("res://ui/KUiShortcutItem.gd")
const KUiSkillDesc := preload("res://ui/KUiSkillDesc.gd")
const KUiDraggedObject := preload("res://ui/KUiDraggedObject.gd")
const KUiItemView := preload("res://ui/KUiItemView.gd")
const KUiScheme := preload("res://ui/KUiScheme.gd")
const Proto := preload("res://proto/jx_pb.gd")

var item_window: UiItem = null
var status_window: UiStatus = null
var skills_window: UiSkills = null
# the bars of the 2.0 screen (docs/CLIENT-2.0.md §7): the top bar (life / mana / stamina / exp / level), the tool bar
# (the buttons that open the windows) and the bottom bar (quick items, the two mouse skills, the chat line)
var top_bar: UiControlBar = null
var tool_bar: UiControlBar = null
var player_bar: UiPlayerBar = null
var skill_tree: UiSkillTree = null   # the mouse-skill tree (Open([[leftskill]]) / Open([[rightskill]]))
var state_window: UiSkillState = null   # the skill state list under the top bar (技能状态列表.ini)
var team_window: UiTeam = null          # the team window (队伍管理.ini; the tool bar's "team", docs/CLIENT-2.0.md §21)
var info_box: UiInformation = null      # the two-button message box (提示.ini): the invitations and applications ask through it
var trade_window: UiTrade = null        # the trade window (玩家间交易.ini), open while Game.trade.state == 2
var player_menu: KWndPopupMenu = null   # Ctrl+right click on a player (autoexec.lua Mouse_Menu): G_UIGAME_* entries by the target's sign
var msg_pad: UiMsgCentrePad = null      # the chat channels (消息集合面板_左.ini): colours, short names, the current channel
var channel_menu: KWndPopupMenu = null  # the ChannelBtn's menu (0x00472620)
var msg_sel: UiMsgSel = null            # a npc script's Say: the sentence and the answers (滚动选择界面.ini)
var info2: UiInformation2 = null        # a npc script's Talk: the pages (提示2.ini)
var describe: UiNpcDescribe = null      # a npc script's Describe: the description and the answers (npc描述界面.ini)
var sys_msg_pane: UiSysMsg = null       # the system message pane (系统消息.ini): a script's TaskTip lands there
var give_window: UiGiveItem = null      # a npc script's GiveItemUI: the 6 x 4 box the player puts pieces into (给予界面.ini)
var _channel_entries: Array = []
var _menu_target := 0                   # the entity the player menu is about
var _menu_actions: Array = []           # the G_UIGAME_* index of each entry shown
signal system_line(text: String)        # a sentence for the chat log (the 0x69 / 0x86 team messages the 2.0 client prints)
var shortcuts := KUiShortcut.new()      # the nine shortcut skills (Q W E A S D Z X C), kept per character
var quick := KUiShortcutItem.new()      # the nine quick slots of the bottom bar (keys 1..9), kept per character
var _tip_skill := 0                     # the skill whose tip the mouse hover shows (0 = none); the zone's numbers may arrive later
signal quick_skill(skill_id: int)       # ShortcutUseItem on a cell holding a skill: cast it at the cursor (the scene knows where)
var hover: UiMouseHover = null
var hand: KUiDraggedObject = null
var ready_ok := false
var screen := Vector2i(1024, 768)
var _canvas: Control = null


func _ready() -> void:
	layer = 10
	_canvas = Control.new()
	_canvas.name = "Canvas"
	_canvas.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_canvas.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(_canvas)
	screen = Vector2i(get_viewport().get_visible_rect().size)
	get_viewport().size_changed.connect(func(): screen = Vector2i(get_viewport().get_visible_rect().size))   # rule 14: the windows re-anchor
	var probe = KUiScheme.open(UiItem.SCHEME)
	if probe == null:
		Log.warn("ui", "item windows missing", {"hint": "python tools/dev.py assets (jxassets export-ui)"})
		return
	item_window = UiItem.new()
	status_window = UiStatus.new()
	skills_window = UiSkills.new()
	hover = UiMouseHover.new()
	hand = KUiDraggedObject.new()
	_build_bars()
	for w in [item_window, status_window, skills_window]:
		_canvas.add_child(w)
		if not w.load_scheme(screen):
			Log.error("ui", "layout missing", {"window": w.SCHEME})
			return
	team_window = UiTeam.new()
	_canvas.add_child(team_window)
	if not team_window.load_scheme(screen):
		Log.warn("ui", "layout missing", {"window": UiTeam.SCHEME})
		team_window.queue_free()
		team_window = null
	trade_window = UiTrade.new()
	_canvas.add_child(trade_window)
	if not trade_window.load_scheme(screen):
		Log.warn("ui", "layout missing", {"window": UiTrade.SCHEME})
		trade_window.queue_free()
		trade_window = null
	else:
		trade_window.item_hovered.connect(_on_item_hovered)
	skill_tree = UiSkillTree.new()
	_canvas.add_child(skill_tree)
	if not skill_tree.load_scheme(screen):
		Log.warn("ui", "layout missing", {"window": UiSkillTree.SCHEME})
		skill_tree.queue_free()
		skill_tree = null
	else:
		skill_tree.picked.connect(_on_skill_clicked)
		skill_tree.hovered.connect(_on_tree_hovered)
		skill_tree.shortcuts = shortcuts
	_load_shortcuts()
	_canvas.add_child(hover)
	hover.load_scheme(screen)
	info_box = UiInformation.new()
	_canvas.add_child(info_box)
	if not info_box.load_scheme(screen):
		Log.warn("ui", "layout missing", {"window": UiInformation.SCHEME})
		info_box.queue_free()
		info_box = null
	else:
		info_box.answered.connect(_on_info_answered)
	Game.team_event.connect(_on_team_event)
	Game.trade_apply.connect(_on_trade_apply)
	Game.trade_end.connect(_on_trade_end)
	Game.sys_msg.connect(_on_sys_msg)
	player_menu = KWndPopupMenu.new()
	_canvas.add_child(player_menu)
	player_menu.picked.connect(_on_player_menu_picked)
	msg_pad = UiMsgCentrePad.new()
	if not msg_pad.load_scheme():
		Log.warn("ui", "layout missing", {"window": UiMsgCentrePad.SCHEME})
	channel_menu = KWndPopupMenu.new()
	_canvas.add_child(channel_menu)
	channel_menu.picked.connect(_on_channel_picked)
	_show_channel()
	msg_sel = UiMsgSel.new()
	_canvas.add_child(msg_sel)
	if not msg_sel.load_scheme(screen):
		Log.warn("ui", "layout missing", {"window": UiMsgSel.SCHEME})
		msg_sel.queue_free()
		msg_sel = null
	else:
		msg_sel.chosen.connect(func(index: int): Game.dialog_answer(index, 0))
	info2 = UiInformation2.new()
	_canvas.add_child(info2)
	if not info2.load_scheme(screen):
		Log.warn("ui", "layout missing", {"window": UiInformation2.SCHEME})
		info2.queue_free()
		info2 = null
	else:
		info2.confirmed.connect(func(): Game.dialog_answer(0, 0))
	describe = UiNpcDescribe.new()
	_canvas.add_child(describe)
	if not describe.load_scheme(screen):
		Log.warn("ui", "layout missing", {"window": UiNpcDescribe.SCHEME})
		describe.queue_free()
		describe = null
	else:
		describe.chosen.connect(func(index: int): Game.dialog_answer(index, 0))
	sys_msg_pane = UiSysMsg.new()
	_canvas.add_child(sys_msg_pane)
	if not sys_msg_pane.load_scheme(screen):
		Log.warn("ui", "layout missing", {"window": UiSysMsg.SCHEME})
		sys_msg_pane.queue_free()
		sys_msg_pane = null
	else:
		Game.task_tip.connect(func(text: String): sys_msg_pane.add_message(text, 1, true, 3))
	give_window = UiGiveItem.new()
	_canvas.add_child(give_window)
	if not give_window.load_scheme(screen):
		Log.warn("ui", "layout missing", {"window": UiGiveItem.SCHEME})
		give_window.queue_free()
		give_window = null
	else:
		give_window.cell_clicked.connect(_put_in_give_box)
		give_window.confirmed.connect(func(entries: Array): Game.give_items(1, entries))
		give_window.changed.connect(func(entries: Array): Game.give_items(0, entries))
		give_window.cancelled.connect(func(): Game.dialog_answer(1, 0))
		give_window.item_hovered.connect(_on_item_hovered)
		Game.give_item_msg.connect(func(kind: int, text: String): give_window.set_message(kind, text))
	Game.script_action.connect(_on_script_action)
	_canvas.add_child(hand)
	item_window.open_status.connect(func(): status_window.open_window())
	status_window.open_item.connect(func(): item_window.open_window())
	item_window.item_hovered.connect(_on_item_hovered)
	status_window.item_hovered.connect(_on_item_hovered)
	item_window.item_lift.connect(_lift)
	status_window.item_lift.connect(_lift)
	item_window.item_put.connect(_put_in_bag)
	status_window.item_put_on.connect(_put_on)
	# the skill book: a left click picks the left mouse skill (GOI_SET_IMMDIA_SKILL 0 of the old client), a right
	# click the right one; the tooltip shows the name and level
	skills_window.skill_clicked.connect(_on_skill_clicked.bind(false))
	skills_window.skill_right_clicked.connect(_on_skill_clicked.bind(true))
	skills_window.skill_hovered.connect(_on_skill_hovered)
	Game.player_attrib_changed.connect(func(_a): _refresh_bars())
	Game.skills_changed.connect(_refresh_mouse_skills)
	Game.mouse_skill_changed.connect(_refresh_mouse_skills)
	Game.skill_desc_received.connect(func(id: int): if id == _tip_skill: _show_skill_tip(id))
	Game.skill_changed.connect(func(_id): _refresh_mouse_skills())
	_refresh_bars()
	_refresh_mouse_skills()
	Game.item_changed.connect(_on_item_changed)
	Game.item_removed.connect(_on_item_removed)
	Game.item_result.connect(_on_item_result)
	ready_ok = true
	Log.info("ui", "item windows ready", {"screen": "%dx%d" % [screen.x, screen.y]})


func _unhandled_input(event: InputEvent) -> void:
	if not ready_ok or not (event is InputEventKey) or not event.pressed or event.echo:
		return
	# the keys of autoexec.lua of the 2.0 client: F3 status, F4 items, F5 skills, M horse; Q W E A S D Z X C the shortcut
	# skills (ShortcutSkill(0..8)); I and K stay as the plain client had them
	var k := KUiShortcut.slot_of_key(event.keycode)
	if k >= 0:
		_shortcut_key(k)
		get_viewport().set_input_as_handled()
		return
	var q := KUiShortcutItem.slot_of_key(event.keycode)
	if q >= 0:
		_quick_key(q)
		get_viewport().set_input_as_handled()
		return
	match event.keycode:
		KEY_I, KEY_F4:
			item_window.toggle_window()
			get_viewport().set_input_as_handled()
		KEY_F3:
			status_window.toggle_window()
			get_viewport().set_input_as_handled()
		KEY_K, KEY_F5:
			skills_window.toggle_window()
			get_viewport().set_input_as_handled()
		KEY_M:
			Game.ride(not bool(Game.entities.get(Game.entity_id, {}).get("riding", false)))
			get_viewport().set_input_as_handled()
		KEY_T, KEY_O:
			# autoexec.lua: AddCommand("T" / "O", "", "Switch([[trade]])") -> 0x005C3236: the sign off when it is up (the 0x6a
			# packet), else up with the sentence (TradeApplyOpen; the 2.0 bar passes none)
			toggle_trade_sign()
			get_viewport().set_input_as_handled()
		KEY_P:
			# AddCommand("P", "", "Open([[team]])")
			if team_window != null:
				team_window.toggle_window()
			get_viewport().set_input_as_handled()
		KEY_ESCAPE:
			if hand.holding():
				_drop_hand()
				get_viewport().set_input_as_handled()
			elif player_menu != null and player_menu.visible:
				player_menu.hide_menu()
				get_viewport().set_input_as_handled()
			elif item_window.visible or status_window.visible or skills_window.visible or (team_window != null and team_window.visible):
				item_window.hide_window()
				status_window.hide_window()
				skills_window.hide_window()
				if team_window != null:
					team_window.hide_window()
				get_viewport().set_input_as_handled()


func any_open() -> bool:
	return ready_ok and (item_window.visible or status_window.visible or skills_window.visible or (skill_tree != null and skill_tree.visible)
		or (team_window != null and team_window.visible) or (info_box != null and info_box.visible)
		or (trade_window != null and trade_window.visible) or (player_menu != null and player_menu.visible))


# Switch([[trade]]) 0x005C3236: my sign 2 -> TradeApplyClose, else TradeApplyOpen(sentence)
func toggle_trade_sign(sentence: String = "") -> void:
	if int(Game.trade.state) == 2:
		return
	if int(Game.trade.state) == 1:
		Game.trade_request(Proto.TradeCmd.TRADE_APPLY_CLOSE)
	else:
		Game.trade_request(Proto.TradeCmd.TRADE_APPLY_OPEN, 0, 0, sentence)


# the player menu (gamecl.exe 0x004C2450; Ctrl+right click = Mouse_Menu of autoexec.lua): the entries the 2.0 client shows
# for a player - "Giao Dịch" (G_UIGAME_2) when the target's sign is 2 and I am in no trade, "Nhập đội" (G_UIGAME_3) when it
# is 1 and I am in no team, "Tổ đội" (G_UIGAME_4: invite) when I lead a team or have none; the other entries (chat, friend,
# follow, info, guild, ...) wait for their systems
# ---- the chat channels (docs/CLIENT-2.0.md §23) ----

# the ChannelBtn's menu (0x00472620): the channels the bar can send on, each line in its colour (0x004B6530)
func _open_channel_menu(at: Vector2) -> void:
	if channel_menu == null or msg_pad == null:
		return
	_channel_entries = msg_pad.menu_entries()
	if _channel_entries.is_empty():
		return
	channel_menu.open_at(_channel_entries, at - Vector2(0, _channel_entries.size() * 17 + 8), screen)


# 0x00475900 -> 0x004730D0: the picked channel becomes the current one (+0x8c48), the button takes its colour
func _on_channel_picked(index: int) -> void:
	if index < 0 or index >= _channel_entries.size():
		return
	set_channel(int(_channel_entries[index].index))


func set_channel(channel: int) -> void:
	if msg_pad == null:
		return
	msg_pad.current = channel
	_show_channel()


func _show_channel() -> void:
	if player_bar != null and msg_pad != null:
		player_bar.set_channel(msg_pad.short_name(msg_pad.current), msg_pad.color_of(msg_pad.current))


# KUiPlayerBar::SendChat 0x00475A10 on a line of the input: "/name text" whispers (Lua Say), "&short text" picks the
# channel by its short name (Lua Chat), anything else goes on the current channel; a line of 0x200 or more is refused
# with G_STR_MSG_VOERFLOW (0x00475F15); SendMsgNum / SendMsgInterval of the channel hold the line back with
# G_PLAYERBAR_3 "%d giây".  The GM channel ('&' on a channel with flag 4, "[gm]" lines) and the word filters
# (\settings\chatsent.flt, 0x0058DF90 / 0x00617B90 -> G_PLAYERBAR_2) are not here.
func send_chat(text: String) -> void:
	if text.strip_edges() == "":
		return
	if text.length() >= 0x200:
		system_line.emit(KUiItemView.client_string("G_STR_MSG_VOERFLOW"))
		return
	var req: Dictionary = msg_pad.parse_input(text) if msg_pad != null else {"channel": 0, "target": "", "text": text}
	if str(req.text).strip_edges() == "":
		return
	if msg_pad != null:
		var wait := msg_pad.throttle(int(req.channel), Time.get_ticks_msec())
		if wait > 0:
			system_line.emit(KUiItemView.client_string("G_PLAYERBAR_3") % wait)
			return
	Game.chat(str(req.text), int(req.channel), str(req.target))


# a received line as the chat log shows it: {"texture", "bbcode"} (KUiMsgCentrePad::ChannelMessageArrival)
func chat_line(msg: Dictionary) -> Dictionary:
	if msg_pad == null:
		return {"texture": null, "bbcode": "[b]%s:[/b] %s" % [str(msg.get("name", "")), str(msg.get("text", "")).replace("[", "[lb]")]}
	var own = Game.entities.get(Game.entity_id, {})
	return msg_pad.line(msg, str(own.get("name", "")))


# the 0x63 packet (KPlayer::OnScriptAction 0x006004C0): UI_SELECTDIALOG -> the question window, UI_TALKDIALOG -> the pages;
# the other ui ids of the 2003 UIInfo (trade, note, msg, news, music, tong) are not sent by the zone
func _on_script_action(a: Dictionary) -> void:
	if int(a.get("operate", 0)) != 0:
		return
	var text := str(a.get("text", ""))
	if int(a.get("text_id", 0)) != 0 and text == "":
		text = "[%d]" % int(a.get("text_id", 0))   # g_GetStringRes(id): no string resource table in the new client yet
	match int(a.get("ui", 0)):
		0:
			if msg_sel != null:
				if info2 != null and info2.visible:
					info2.close_pages()
				if describe != null and describe.visible:
					describe.close_dialog()
				msg_sel.open_dialog(text, a.get("options", []))
		2:
			if info2 != null:
				if msg_sel != null and msg_sel.visible:
					msg_sel.close_dialog()
				info2.speak_words(a.get("options", []), int(a.get("param", 0)) == 1)
		11:
			# 0x0060194C -> the ui message 0x3e -> the give-item box: the content, the title (the packet's second string), the flags
			if give_window != null:
				var opts: Array = a.get("options", [])
				give_window.open_box(text, str(opts[0]) if opts.size() > 0 else "", bool(a.get("notify", false)), int(a.get("param", 0)))
		12:
			# 0x006007FD -> the ui message 0x40 -> the npc description window; the name under the portrait is the npc talked to
			if describe != null:
				if msg_sel != null and msg_sel.visible:
					msg_sel.close_dialog()
				describe.open_dialog(text, a.get("options", []), str(Game.entities.get(Game.dialog_npc, {}).get("name", "")))
		_:
			Log.warn("ui", "script action ui not shown", {"ui": int(a.get("ui", 0))})


func open_player_menu(entity_id: int, at: Vector2) -> void:
	if player_menu == null:
		return
	var e = Game.entities.get(entity_id)
	if e == null or entity_id == Game.entity_id:
		return
	var entries: Array = []
	_menu_actions = []
	var sign := int(e.get("menu_state", 0))
	if sign == 2 and int(Game.trade.state) != 2:
		entries.append(KUiItemView.client_string("G_UIGAME_2"))
		_menu_actions.append(2)
	if sign == 1 and not bool(Game.team.in_team):
		entries.append(KUiItemView.client_string("G_UIGAME_3"))
		_menu_actions.append(3)
	if not bool(Game.team.in_team) or bool(Game.team.captain):
		entries.append(KUiItemView.client_string("G_UIGAME_4"))
		_menu_actions.append(4)
	if entries.is_empty():
		return
	_menu_target = entity_id
	player_menu.open_at(entries, at, screen)


func _on_player_menu_picked(index: int) -> void:
	if index < 0 or index >= _menu_actions.size():
		return
	match int(_menu_actions[index]):
		2:
			# ProcessPeople ACTION_TRADE -> TradeApplyStart (the 0x6b packet)
			Game.trade_request(Proto.TradeCmd.TRADE_APPLY_START, _menu_target)
			system_line.emit(KUiItemView.core_string("MSG_TRADE_SEND_APPLY") % str(Game.entities.get(_menu_target, {}).get("name", "")))
		3:
			# ACTION_JOINTEAM -> ApplyAddTeam (the 0x53 sub 4)
			Game.team_request(Proto.TeamCmd.TEAM_APPLY_ADD, _menu_target)
		4:
			# the invitation: a team is made first when there is none (KUiTeamManage 0x004ADE00 does the same)
			if not bool(Game.team.in_team):
				Game.team_request(Proto.TeamCmd.TEAM_CREATE)
			Game.team_request(Proto.TeamCmd.TEAM_INVITE, _menu_target)


# the 0x8b packet: UiSysMsgCentre SMCT_UI_TRADE_APPLY - "%s mong muốn giao dịch với bạn" (G_SysMsgCentre_3), agree / refuse
func _on_trade_apply(ev: Dictionary) -> void:
	system_line.emit(KUiItemView.core_string("MSG_TRADE_GET_APPLY") % str(ev.name))
	if info_box != null:
		info_box.show_box(KUiItemView.client_string("G_SysMsgCentre_3") % str(ev.name), KUiItemView.client_string("G_ACCEPT_WORD"),
			KUiItemView.client_string("G_REFUSE_WORD"), {"kind": "trade", "id": int(ev.id)})


# the 0x78 packet: MSG_TRADE_SUCCESS / MSG_TRADE_FAIL with the partner's name
func _on_trade_end(ok: bool) -> void:
	system_line.emit(KUiItemView.core_string("MSG_TRADE_SUCCESS" if ok else "MSG_TRADE_FAIL") % str(Game.trade.partner_name))


# the 0x86 packet: the sentence by id (the client's 0x00657AD0 table, docs/CLIENT-2.0.md §21)
func _on_sys_msg(id: int, _entity_id: int, name: String) -> void:
	var line := sys_msg_text(id, name)
	if line != "":
		system_line.emit(line)


static func sys_msg_text(id: int, name: String) -> String:
	var keys := {
		2: "MSG_TEAM_DISMISS_CAPTAIN", 3: "MSG_TEAM_LEAVE_SELF_MSG", 5: "MSG_TEAM_SELF_ADD", 8: "MSG_OBJ_CANNOT_PICKUP", 9: "MSG_OBJ_TOO_FAR",
		0xa: "MSG_DEC_MONEY", 0xb: "MSG_TRADE_SELF_ROOM_FULL", 0xc: "MSG_TRADE_DEST_ROOM_FULL", 0xd: "MSG_TRADE_REFUSE_APPLY",
		0xe: "MSG_TRADE_TASK_ITEM", 0x10: "MSG_ITEM_DAMAGED", 0x11: "MSG_MONEY_CANNOT_PICKUP", 0x12: "MSG_TEAM_TARGET_CANNOT_ADD_TEAM",
		0x13: "MSG_TEAM_TARGET_CANNOT_ADD_TEAM", 0x24: "MSG_TEAM_ERROR01", 0x25: "MSG_TEAM_ERROR02", 0x26: "MSG_TEAM_ERROR03",
		0x27: "MSG_TEAM_ERROR04", 0x28: "MSG_TEAM_ERROR05", 0x2a: "MSG_ITEM_USINGTIMES_END",
	}
	var client_keys := {0x2c: "G_ProtocolProcess_20", 0x29: "G_ProtocolProcess_4"}
	var text := ""
	if keys.has(id):
		text = KUiItemView.core_string(keys[id])
	elif client_keys.has(id):
		text = KUiItemView.client_string(client_keys[id])
	if text.find("%s") >= 0:
		text = text % name
	elif text.find("%d") >= 0:
		text = text.replace("%d", "")
	return text


func _on_item_hovered(item) -> void:
	if item == null or hand.holding():
		hover.hide_lines()
	else:
		# KItem::GetDesc, then the "\n" the CoreShell wrapper (0x006b66a0) adds after it
		hover.show_text(KUiItemView.describe_text(item, KUiItemView.equip_enhance(item)) + "\n", _canvas.get_local_mouse_position())


# KUiPlayerBar::LoadScheme 0x004750E0 (the bottom bar, then [Main] ToolBoxSchema = the tool bar 0x00474B20) and the
# top bar 顶部控制条.ini: the bars sit under the windows; a missing layout leaves that bar out
func _build_bars() -> void:
	player_bar = UiPlayerBar.new()
	_canvas.add_child(player_bar)
	if not player_bar.load_scheme(screen):
		Log.warn("ui", "layout missing", {"window": UiPlayerBar.SCHEME})
		player_bar.queue_free()
		player_bar = null
	else:
		player_bar.mouse_skill_clicked.connect(func(right): if skill_tree != null: skill_tree.toggle_for(right))
		player_bar.quick_clicked.connect(_quick_key)
		player_bar.quick_put.connect(_quick_put)
		player_bar.quick_right_clicked.connect(_quick_clear)
		player_bar.channel_clicked.connect(_open_channel_menu)
		player_bar.quick_hovered.connect(_on_quick_hovered)
		player_bar.mouse_skill_hovered.connect(func(_right: bool, obj): _show_skill_tip(int(obj.id) if obj != null else 0))
		Game.items_changed.connect(_refresh_quick)
		Game.item_changed.connect(func(_item): _refresh_quick())
		Game.item_removed.connect(func(_id): _refresh_quick())
		_refresh_quick()
	top_bar = UiControlBar.new()
	_canvas.add_child(top_bar)
	if not top_bar.load_scheme("thanh-dieu-khien-tren", screen):
		Log.warn("ui", "layout missing", {"window": "thanh-dieu-khien-tren"})
		top_bar.queue_free()
		top_bar = null
	tool_bar = UiControlBar.new()
	_canvas.add_child(tool_bar)
	if not tool_bar.load_scheme("thanh-cong-cu", screen, true):
		Log.warn("ui", "layout missing", {"window": "thanh-cong-cu"})
		tool_bar.queue_free()
		tool_bar = null
	else:
		tool_bar.command.connect(_on_bar_command)
	state_window = UiSkillState.new()
	_canvas.add_child(state_window)
	if not state_window.load_scheme(screen):
		Log.warn("ui", "layout missing", {"window": UiSkillState.SCHEME})
		state_window.queue_free()
		state_window = null
	else:
		Game.state_changed.connect(func(_id): state_window.refresh())
		state_window.state_hovered.connect(_on_state_hovered)


# the Lua Open([[x]]) / Switch([[x]]) a tool bar button runs (0x0044B250..): the windows this client has
func _on_bar_command(cmd: String) -> void:
	match cmd:
		"status":
			status_window.toggle_window()
		"items":
			item_window.toggle_window()
		"skills":
			skills_window.toggle_window()
		"sit":
			# Switch([[sit]]) 0x0044B470 of the 2.0 tool bar: the 0x71 packet - down when standing, up when sitting
			var own = Game.entities.get(Game.entity_id)
			var sitting: bool = own != null and int(own.get("doing", 0)) == 7   # jx.pb.Action ACTION_SIT
			Game.sit(not sitting)
		"team":
			# Open([[team]]) of the 2.0 tool bar -> KUiTeamManage::OpenWindow 0x004AE880
			if team_window != null:
				team_window.toggle_window()
		_:
			Log.info("ui", "window not built yet", {"command": cmd})


func _refresh_bars() -> void:
	if top_bar != null:
		top_bar.refresh()
	if tool_bar != null:
		tool_bar.refresh()


# the 0x69 sub-commands and the 0x86 team messages as the 2.0 client shows them (docs/CLIENT-2.0.md §21): an invitation (sub
# 0xc, 0x00604630) and an application (sub 7, 0x00603780) open the two-button box of UiSysMsgCentre (G_SysMsgCentre_0 / _1,
# G_ACCEPT_WORD / G_REFUSE_WORD); everything else is a line of the string table in the chat log
func _on_team_event(ev: Dictionary) -> void:
	var kind := int(ev.event)
	var who := str(ev.name)
	match kind:
		Proto.TeamEventKind.TEAM_EV_INVITE:
			system_line.emit(KUiItemView.core_string("MSG_TEAM_GET_INVITE") % who)
			if info_box != null:
				info_box.show_box(KUiItemView.client_string("G_SysMsgCentre_0") % who, KUiItemView.client_string("G_ACCEPT_WORD"),
					KUiItemView.client_string("G_REFUSE_WORD"), {"kind": "invite", "id": int(ev.id)})
		Proto.TeamEventKind.TEAM_EV_APPLY:
			system_line.emit(KUiItemView.core_string("MSG_TEAM_APPLY_ADD") % who)
			if info_box != null:
				info_box.show_box(KUiItemView.client_string("G_SysMsgCentre_1") % who, KUiItemView.client_string("G_ACCEPT_WORD"),
					KUiItemView.client_string("G_REFUSE_WORD"), {"kind": "apply", "id": int(ev.id)})
		_:
			var line := team_event_text(ev)
			if line != "":
				system_line.emit(line)


# the sentence of a team event, "" when the 2.0 client says nothing for it
static func team_event_text(ev: Dictionary) -> String:
	var kind := int(ev.event)
	var who := str(ev.name)
	var arg := int(ev.arg)
	var own := int(ev.id) == Game.entity_id
	match kind:
		Proto.TeamEventKind.TEAM_EV_CREATE_OK:
			return KUiItemView.core_string("MSG_TEAM_CREATE")
		Proto.TeamEventKind.TEAM_EV_CREATE_FAIL:
			return KUiItemView.core_string("MSG_TEAM_CANNOT_CREATE" if arg == 4 else "MSG_TEAM_CREATE_FAIL")
		Proto.TeamEventKind.TEAM_EV_ADD_MEMBER:
			return KUiItemView.core_string("MSG_TEAM_ADD_MEMBER") % who
		Proto.TeamEventKind.TEAM_EV_SELF_ADD:
			return KUiItemView.core_string("MSG_TEAM_SELF_ADD") % who
		Proto.TeamEventKind.TEAM_EV_LEAVE:
			if own:
				var leader: Dictionary = Game.team.get("leader", {})
				return KUiItemView.core_string("MSG_TEAM_LEAVE_SELF_MSG") % str(leader.get("name", ""))
			return KUiItemView.core_string("MSG_TEAM_LEAVE") % who
		Proto.TeamEventKind.TEAM_EV_KICK:
			return KUiItemView.core_string("MSG_TEAM_BE_KICKEN") if own else KUiItemView.core_string("MSG_TEAM_KICK_ONE") % who
		Proto.TeamEventKind.TEAM_EV_CHANGE_CAPTAIN:
			if arg == 1:
				var leader: Dictionary = Game.team.get("leader", {})
				return KUiItemView.core_string("MSG_TEAM_CHANGE_CAPTAIN_SELF") % str(leader.get("name", ""))
			return KUiItemView.core_string("MSG_TEAM_CHANGE_CAPTAIN") % who
		Proto.TeamEventKind.TEAM_EV_OPEN_CLOSE:
			return KUiItemView.core_string("MSG_TEAM_OPEN" if arg != 0 else "MSG_TEAM_CLOSE")
		Proto.TeamEventKind.TEAM_EV_DISMISS:
			if bool(Game.team.get("captain", false)):
				return KUiItemView.core_string("MSG_TEAM_DISMISS_CAPTAIN")
			var leader: Dictionary = Game.team.get("leader", {})
			return KUiItemView.core_string("MSG_TEAM_DISMISS_MEMBER") % str(leader.get("name", ""))
		Proto.TeamEventKind.TEAM_EV_REFUSE:
			return KUiItemView.core_string("MSG_TEAM_REFUSE_INVITE") % who
		Proto.TeamEventKind.TEAM_EV_MSG:
			# the 0x86 handler 0x00657AD0: the id picks the key (the jump table 0x658674)
			match arg:
				6:
					return KUiItemView.core_string("MSG_TEAM_CHANGE_CAPTAIN_FAIL1") + KUiItemView.core_string("MSG_TEAM_CHANGE_CAPTAIN_FAIL2") % who
				7:
					return KUiItemView.core_string("MSG_TEAM_CHANGE_CAPTAIN_FAIL1") + KUiItemView.core_string("MSG_TEAM_CHANGE_CAPTAIN_FAIL3")
				0x12, 0x13:
					return KUiItemView.core_string("MSG_TEAM_TARGET_CANNOT_ADD_TEAM")
				0x24, 0x25, 0x26, 0x27, 0x28:
					return KUiItemView.core_string("MSG_TEAM_ERROR0%d" % (arg - 0x23))
				_:
					return ""
		_:
			return ""


# the answer of the box: the first button agrees (UiSysMsgCentre nSelAction == 0)
func _on_info_answered(index: int, param: Variant) -> void:
	if not (param is Dictionary):
		return
	match str(param.get("kind", "")):
		"invite":
			# TEAM_OI_INVITE_RESPONSE -> the 0x53 sub 11 {captain, flag}
			Game.team_request(Proto.TeamCmd.TEAM_REPLY_INVITE, int(param.id), 1 if index == 0 else 0)
		"apply":
			# TEAM_OI_APPLY_RESPONSE: agreeing is AddTeamMember (the 0x53 sub 5); a refusal sends nothing to jx_linux_y
			if index == 0:
				Game.team_request(Proto.TeamCmd.TEAM_ACCEPT, int(param.id))
		"trade":
			# c2sTradeReplyStart: the applicant and the answer
			Game.trade_request(Proto.TradeCmd.TRADE_REPLY, int(param.id), 1 if index == 0 else 0)


# the two mouse skill boxes of the bottom bar (ImediaLeftSkill / ImediaRightSkill)
func _refresh_mouse_skills() -> void:
	if player_bar == null:
		return
	var left := {}
	var right := {}
	if Game.left_skill > 0 and Game.skills.has(Game.left_skill):
		left = skills_window.skill_info(Game.left_skill)
		left["id"] = Game.left_skill
	if Game.right_skill > 0 and Game.skills.has(Game.right_skill):
		right = skills_window.skill_info(Game.right_skill)
		right["id"] = Game.right_skill
	player_bar.set_mouse_skills(left, right)


func _on_skill_clicked(skill_id: int, right: bool) -> void:
	if right:
		Game.right_skill = skill_id
		# SetRightSkill 0x005FB280 -> KNpc::SetAura 0x005EA870(IsAura ? id : 0): an aura on the right button is switched on
		# at the zone, anything else switches the aura off
		var aura := KUiSkillDesc._cell_int(Game.skill_row(skill_id), "IsAura", 0) != 0
		Game.set_aura(skill_id if aura else 0)
	else:
		Game.left_skill = skill_id
	Log.info("ui", "mouse skill", {"skill": skill_id, "button": "right" if right else "left"})
	_refresh_mouse_skills()


# the tip of a state icon: "name\ndesc\ntime" (0x0041ECE4)
# ShortcutSkill(k) 0x00495F70: with the tree open the hovered entry takes slot k; closed, slot k becomes the mouse skill
# of its side (OperationRequest(0xd, entry, side))
func _shortcut_key(k: int) -> void:
	if skill_tree != null and skill_tree.visible:
		var id := skill_tree.hovered_skill()
		if id > 0:
			assign_shortcut(k, id, skill_tree.right_side)
		return
	var s: Dictionary = shortcuts.slot(k)
	if int(s.id) <= 0 or not Game.skills.has(int(s.id)):
		return
	_on_skill_clicked(int(s.id), bool(s.right))


func assign_shortcut(k: int, skill_id: int, right: bool) -> void:
	if shortcuts.assign(k, skill_id, right):
		Log.info("ui", "shortcut skill", {"slot": k, "key": KUiShortcut.key_of(k), "skill": skill_id, "side": "right" if right else "left"})
		_save_shortcuts()
		if skill_tree != null:
			skill_tree.queue_redraw()


# ShortcutUseItem(k) 0x00472F80: the cell's item is used (UseItem 0x005FD4B0 wants it in the bag: equipment is worn,
# a medicine / portal / script item used), a skill is cast at the cursor (op 0xa genre 4 -> 0x005BC500)
func _quick_key(k: int) -> void:
	var s: Dictionary = quick.slot(k)
	if int(s.genre) == KUiShortcutItem.GENRE_SKILL:
		if Game.skills.has(int(s.id)):
			quick_skill.emit(int(s.id))
		return
	if int(s.genre) != KUiShortcutItem.GENRE_ITEM:
		return
	var it = Game.items.get(int(s.id))
	if it == null or int(it.room) != Game.ROOM_BAG:
		return
	Log.info("ui", "quick item", {"slot": k, "item": int(it.id), "name": str(it.name), "genre": int(it.genre)})
	if int(it.genre) == KUiItemView.GENRE_EQUIP:
		Game.item_equip(int(it.id), -1)
	else:
		Game.item_use(int(it.id))


# a click on a quick box with an item on the cursor (msg 0x511 -> 0x00472E70 -> op 3 kind 7): the cell references the
# bag item, the cursor is emptied (the item stays in the bag); refused when the bar holds that kind already (0x006386C0)
func _quick_put(k: int) -> void:
	if not hand.holding():
		return
	var it = Game.items.get(hand.item_id)
	if it == null:
		return
	if not assign_quick(k, it):
		Log.info("ui", "quick slot refused", {"slot": k, "item": int(it.id), "name": str(it.name)})
		return
	_drop_hand()


func assign_quick(k: int, item: Dictionary) -> bool:
	if not quick.put_item(k, item):
		return false
	Log.info("ui", "quick slot", {"slot": k, "item": int(item.id), "name": str(item.name)})
	_save_shortcuts()
	_refresh_quick()
	return true


func _quick_clear(k: int) -> void:
	if int(quick.slot(k).genre) == 0:
		return
	quick.remove(k)
	_save_shortcuts()
	_refresh_quick()


# the cells follow the bag (op 0xe: a vanished item is replaced by one of its kind, or the cell clears)
func _refresh_quick() -> void:
	if player_bar == null:
		return
	if quick.resolve(Game.items, Game.ROOM_BAG):
		_save_shortcuts()
	for i in KUiShortcutItem.SLOTS:
		var s: Dictionary = quick.slot(i)
		var view := {}
		if int(s.genre) == KUiShortcutItem.GENRE_ITEM and Game.items.has(int(s.id)):
			view = KUiItemView.object_of(Game.items[int(s.id)])
		elif int(s.genre) == KUiShortcutItem.GENRE_SKILL and skills_window != null:
			var info: Dictionary = skills_window.skill_info(int(s.id))
			view = {"id": int(s.id), "image": Assets.item_image(str(info.get("icon", ""))), "ex_type": 0, "usable": false,
				"name": str(info.get("name", "")), "count": 0}
		player_bar.set_quick(i, view)


# [ShortSkill] ShortcutSkill_%d and [Player] Item_%d of the character's settings (0x0052D720, 0x00473B10): here
# user://shortcuts_<player id>.json {"skills": [...], "items": [...]}
func _shortcut_path() -> String:
	return "user://shortcuts_%d.json" % int(Game.player_id)


func _load_shortcuts() -> void:
	var path := _shortcut_path()
	if not FileAccess.file_exists(path):
		return
	var f := FileAccess.open(path, FileAccess.READ)
	if f == null:
		return
	var parsed = JSON.parse_string(f.get_as_text())
	f.close()
	if parsed is Array:   # the first files held the skills only
		shortcuts.from_json(parsed)
	elif parsed is Dictionary:
		shortcuts.from_json(parsed.get("skills", []))
		quick.from_json(parsed.get("items", []))


func _save_shortcuts() -> void:
	var f := FileAccess.open(_shortcut_path(), FileAccess.WRITE)
	if f == null:
		return
	f.store_string(JSON.stringify({"skills": shortcuts.to_json(), "items": quick.to_json()}))
	f.close()


func _on_state_hovered(text: String) -> void:
	if text == "":
		hover.hide_lines()
	else:
		hover.show_text(text + "\n", _canvas.get_local_mouse_position())


func _on_tree_hovered(skill_id: int) -> void:
	_show_skill_tip(skill_id if skill_id > 0 and Game.skills.has(skill_id) else 0)


func _on_skill_hovered(skill) -> void:
	_show_skill_tip(int(skill.id) if skill != null else 0)


# a quick box under the mouse (KWndObjectBox -> ShowObjectTip 0x0044EBC0): an item's tip or a skill's, nothing when empty
func _on_quick_hovered(slot: int, obj) -> void:
	if obj == null:
		hover.hide_lines()
		_tip_skill = 0
		return
	var s: Dictionary = quick.slot(slot)
	if int(s.genre) == KUiShortcutItem.GENRE_SKILL:
		_show_skill_tip(int(s.id))
	else:
		_on_item_hovered(Game.items.get(int(obj.id)))


# the tip of a skill (ShowObjectTip 0x0044EBC0 genre 4 -> KSkill::GetDesc 0x006FBC90): the static part at once, the level
# numbers when the zone answers (G2C_SKILL_DESC), the answer kept per (skill, level)
func _show_skill_tip(skill_id: int) -> void:
	_tip_skill = skill_id
	if skill_id <= 0:
		hover.hide_lines()
		return
	var held: Dictionary = Game.skills.get(skill_id, {})
	var level := int(held.get("level", 0))
	var desc = Game.skill_desc(skill_id, level)
	if desc == null:
		Game.skill_desc_request(skill_id, level)
		desc = {}
	var text := KUiSkillDesc.build(Game.skill_row(skill_id), held, int(Game.player_attrib.get("level", 1)), desc, Game.skill_text(), Game.skill_name)
	hover.show_text(text, _canvas.get_local_mouse_position())


func skill_tip_text(skill_id: int) -> String:
	var held: Dictionary = Game.skills.get(skill_id, {})
	var desc = Game.skill_desc(skill_id, int(held.get("level", 0)))
	return KUiSkillDesc.build(Game.skill_row(skill_id), held, int(Game.player_attrib.get("level", 1)), desc if desc != null else {}, Game.skill_text(), Game.skill_name, Game.skill_row)


# A click on an item with nothing on the cursor lifts it (Wnd_SetDragObj).
func _lift(item: Dictionary) -> void:
	hand.lift(item)
	hover.hide_lines()
	_tell_hand()
	Log.debug("item", "item lifted", {"id": item.id, "name": item.name})


func _drop_hand() -> void:
	hand.drop()
	_tell_hand()


func _tell_hand() -> void:
	item_window.set_hand(hand.cells)
	status_window.set_hand(hand.cells)
	if player_bar != null:
		player_bar.set_hand(hand.cells)
	if give_window != null:
		give_window.set_hand(hand.cells)


# A click in the bag with an item on the cursor: the zone decides (C2G_ITEM_MOVE); the item
# stays on the cursor until G2C_ITEM_MOVE says where it went or G2C_ITEM_RESULT says no.
func _put_in_bag(x: int, y: int) -> void:
	if not hand.holding():
		return
	hand.pending_seq = Game.item_move(hand.item_id, Game.ROOM_BAG, x, y)


# A click on the give-item box with a bag piece on the cursor: the box remembers it (the piece stays in the bag)
func _put_in_give_box(x: int, y: int) -> void:
	if not hand.holding() or give_window == null:
		return
	var it = Game.items.get(hand.item_id)
	if it != null and give_window.put_item(it, Vector2i(x, y)):
		_drop_hand()


func _put_on(part: int) -> void:
	if not hand.holding():
		return
	hand.pending_seq = Game.item_equip(hand.item_id, part)


func _on_item_changed(item: Dictionary) -> void:
	if hand.holding() and int(item.id) == hand.item_id:
		_drop_hand()


func _on_item_removed(id: int) -> void:
	if hand.holding() and id == hand.item_id:
		_drop_hand()


func _on_item_result(seq: int, result: int) -> void:
	if hand.holding() and seq == hand.pending_seq:
		hand.pending_seq = 0   # still on the cursor: the player may try elsewhere
	Log.debug("item", "item request answered", {"seq": seq, "result": result})
