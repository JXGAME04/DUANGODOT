# Game - the client side of the protocol flow (docs/PROTOCOL.md section 3).
# UI scenes call the methods and listen to the signals; protobuf never leaks into scenes.
extends Node

const Proto := preload("res://proto/jx_pb.gd")
const KLogin := preload("res://net/KLogin.gd")
const KPlayerTask := preload("res://scenes/KPlayerTask.gd")
const CLIENT_VERSION := "0.2.0"
const PING_INTERVAL := 5.0
# no Pong for this long while in the world = the server is gone (it drops us after
# heartbeat_s, 30 s by default, so the client notices first)
const PONG_TIMEOUT := 15.0

signal login_result(ok: bool, text: String)
signal char_list(chars: Array)
signal char_created(ok: bool, result: int, summary: Dictionary)
signal entered_world(info: Dictionary)
signal enter_failed(result: int)
signal entity_spawn(entities: Array)
signal entity_despawn(ids: Array)
signal entity_move(mv: Dictionary)
signal entity_action(a: Dictionary)
signal map_changed(info: Dictionary)   # the zone moved us to another map bundle
signal entity_life(l: Dictionary)
signal entity_ride(r: Dictionary)   # G2C_ENTITY_RIDE: a npc mounted or dismounted (drawn from B4 on)
signal entity_camp(c: Dictionary)   # G2C_ENTITY_CAMP: a npc's camp changed (the 0x59 / 0x58 packets)
# G2C_PLAYER_FACTION (the 0x7b / 0x7c packets) or the login sync moved the character's faction record: the skill book
# shows the branch pages of the faction last joined (GDI 0x413 reads PlayerData+0x12080)
signal faction_changed
# G2C_ENTITY_STATE (the 0x87 packet): a skill's timed state came or went on the character - the state list of the 2.0 client
signal state_changed(skill_id: int)
signal chat_msg(msg: Dictionary)
# items (M11): the bag model below changed; `items_changed` after the whole list, `item_changed`
# for one item (added, moved, a stack changed), `item_removed`, `item_result` when a request
# changed nothing (seq of the request, Proto.Result), `money_changed`
signal items_changed()
signal item_changed(item: Dictionary)
signal item_removed(id: int)
signal item_result(seq: int, result: int)
signal money_changed(money: int, bank_money: int)
# the character's own numbers (G2C_PLAYER_ATTRIB: level, exp, points, life / mana / stamina,
# attack rating, defence, damage, resistances - the CURPLAYER_SYNC of the old game); the
# dictionary is `player_attrib`
signal player_attrib_changed(attrib: Dictionary)
signal skills_changed()                 # G2C_SKILL_LIST: the whole book (on entering the world)
signal mouse_skill_changed()            # left_skill / right_skill set by the weapon rule (0x005FE820)
signal skill_changed(skill_id: int)     # G2C_SKILL_LEVEL / G2C_SKILL_FORBID: one skill (level -1 = gone)
signal skill_desc_received(skill_id: int)   # G2C_SKILL_DESC: the numbers of a skill level for its tip arrived
var aura_skill := 0                          # KNpc+0x120 of the 2.0 client: the aura asked for (KNpc::SetAura 0x005EA870)
signal state_icons_changed(entity_id: int)  # G2C_STATE_ICONS: the six icons over an entity changed (entities[id].state_icons)
signal gold_changed(entity_id: int)         # G2C_NPC_GOLD: a monster turned gold (entities[id].gold_type = its kind, the 0x9a packet)
signal entity_res(r: Dictionary)            # G2C_ENTITY_RES: a player's look (the 0xad packet -> KNpc::SetPlayerRes 0x005ED920)
signal pk_changed(state: int, value: int, refused: bool)   # G2C_PK_STATE: one's own PK state (the 0x90 packet) / value (0x93)
signal entity_pk(r: Dictionary)             # G2C_ENTITY_PK: a player's PK state (the flag & 3 of the 0x4b sync -> KNpc+0x16e4)
signal team_changed()                       # G2C_TEAM_SELF: one's own team as it stands (the 0x69 sub 2 / sub 9 packets; `team`)
signal team_event(ev: Dictionary)           # G2C_TEAM_EVENT: {event, id, name, level, arg, leader, members} (the other 0x69 sub-commands, the 0x86 team messages)
signal trade_changed()                      # G2C_TRADE_STATE / G2C_TRADE_SYNC / G2C_TRADE_END: `trade` changed (docs/LINUX-SERVER.md §18)
signal trade_item(ev: Dictionary)           # G2C_TRADE_ITEM: {item, removed} - the partner's trade box changed (trade.other_items holds it)
signal trade_apply(ev: Dictionary)          # G2C_TRADE_APPLY: {id, name} asks to trade with me (the 0x8b packet)
signal trade_end(ok: bool)                  # G2C_TRADE_END: the 0x78 packet
signal sys_msg(id: int, entity_id: int, name: String)   # G2C_SYS_MSG: the 0x86 packet - a stringtable_core.txt sentence by id (CLIENT-2.0.md §21)
signal entity_menu_state(id: int)           # G2C_ENTITY_MENU_STATE: entities[id].menu_state / menu_sentence changed (the sign over the head)
signal script_action(action: Dictionary)    # G2C_SCRIPT_ACTION: {operate, ui, text, text_id, interactive, param, options} of a npc script's Say / Talk
signal task_value_changed(id: int, value: int)   # G2C_TASK_VALUE / G2C_TASK_VALUES: task_values[id] changed (the 0xa7 / 0xb5 packets -> KPlayer::SetTaskValue 0x00601ED0)
signal missle_sync(m: Dictionary)       # G2C_MISSLE: a missile born / flying / gone (the scene draws it)
signal kicked(reason: int, text: String)
signal connection_lost(reason: String)
signal pong(rtt_ms: int, server_ms: int)

var host := "127.0.0.1"
var port := 17100
var state := "offline"   # offline, connecting, hello, auth, lobby, entering, world
var sid := 0
var account_id := 0
var player_id := 0
var entity_id := 0
var zone_id := 0
var zone_name := ""
var tick_hz := 20
var map_id := 0          # asset bundle to draw (0 = grid)
var scene_w := 0         # map size in scene units
var scene_h := 0
var last_rtt_ms := 0
var auth_mode := ""      # "dev" / "strict" from HelloAck
var heartbeat_s := 30    # the gateway's silence limit in the world
var last_notice := ""    # why we are back on the login screen (kick / lost connection)
# The Result of the last login attempt, for the window that turns it into the game's own sentence
# (KUiConnectInfo): Proto.Result.*, or LOGIN_NO_CONNECTION when the gateway was never reached.
const LOGIN_NO_CONNECTION := -1
var last_login_result := 0
var max_chars := 3       # CharListRes.max_chars
var chars: Array = []
# entity_id -> Dictionary; the model of what the zone shows us.  Kept here (not in the scene) so
# packets that arrive before the world scene is loaded are not lost.
var entities := {}
# item id -> Dictionary (see _item_dict): everything the character carries, as the zone last
# said (G2C_ITEM_LIST after entering the world, then ADD / MOVE / REMOVE).  Rooms as the zone
# numbers them: 0 bag, 1 repository, 2 trade, 3 quick slots, 10 worn (x = ITEM_PART).
var items := {}
var money := 0
var bank_money := 0
# what the zone last said about the character itself (KPlayer of the zone): keys as in
# PlayerAttribSync (level, exp, next_level_exp, attribute_point, skill_point, strength.., cur_strength..,
# life, life_max, mana, mana_max, stamina, stamina_max, attack_rating, defend, min_damage, max_damage,
# fire_resist.., walk_speed, run_speed, attack_speed, cast_speed)
var player_attrib := {}
# The skill book (KSkillList of the zone, s2c_synccurplayerskill of the old client): skill_id -> {id, level,
# current_level, exp_percent (0..1024), max_level, req_level, forbidden, cool_down_left, only_inc}
var skills := {}
# KPlayerFaction of the character (PlayerData+0x12078 current, +0x12080 last added, +0x12084 times joined of the 2.0
# client): -1 = none; camp = m_Camp of the player's npc (C_FREE 4 after leaving)
var faction := -1
var task_values := {}    # id -> value: the saved task values the zone mirrors here (SYNC_FLAG rows of settings/task/player_task_def.txt; KPlayer+0xa1a0 of the 2.0 client)
var task_packets := 0    # G2C_TASK_VALUE + G2C_TASK_VALUES received (the login sends every SYNC_FLAG id, zeros included)
var pk_state := 0        # KPlayerPK state of one's own character: 0 exercise, 1 fight, 2 kill (the 0x90 packet)
var pk_value := 0        # the PK value 0..10 (the 0x93 packet)
# the client's KPlayerTeam (core+0xa878+0x7258 of the 2.0 client) + the s2c_teamselfinfo table (0x1f17608..): in_team, team_id,
# state (1 open), captain (am I), leader {id, name, level}, members [{id, name, level}], lead_level, lead_exp, members_max
var team := {"in_team": false, "team_id": -1, "state": 0, "captain": false, "leader": {}, "members": [], "lead_level": 1, "lead_exp": 0, "members_max": 0}
# the client's KTrade (core+0xa878+0x... of the 2.0 client; KPlayerTrade.h): state 0 normal / 1 open for trade / 2 trading,
# the partner, the four flags of the 0x81 sync, the money on both tables, the partner's items (the 0xcc-byte syncs) by id
var trade := {"state": 0, "partner": 0, "partner_name": "", "self_lock": false, "dest_lock": false, "self_ok": false, "dest_ok": false,
	"self_money": 0, "dest_money": 0, "other_items": {}}
var faction_last := -1
var faction_count := 0
var camp := 0
# the character's own skill states (KNpc::m_StateSkillList of the client): skill id -> {level, time (frames), until_ms,
# special_id, states}
var states := {}
var skills_forbidden := false
var skill_descs := {}                   # "id:level" -> the G2C_SKILL_DESC answer as a Dictionary (the tip's numbers)
var _skill_text := {}                   # text/skill_desc.json (G_* strings, [Descript], [SkillAttrib]...), loaded on first use
var _skill_text_loaded := false
var _skill_rows := {}                   # skills.json rows by id (the cells), loaded on first use
var _skill_rows_loaded := false
var _missle_res := {}                   # missles/missle_res.json rows by id (the drawing side of missles.txt), loaded on first use
var _missle_res_loaded := false
var missle_packets := 0                 # G2C_MISSLE packets seen (the --auto flow counts them)
# the two mouse skills of the old client (KPlayer::m_nLeftSkillID / m_nRightSkillID, GOI_SET_IMMDIA_SKILL):
# 0 = the plain attack of the weapon (C2G_ATTACK)
var left_skill := 0
var right_skill := 0
const ITEMPART_WEAPON := 3              # ITEM_PART itempart_weapon (KItem.h of the zone)
const KWeaponSkillTable := preload("res://ui/KWeaponSkillTable.gd")
var _weapon_table := {}                 # weapon_skill.json, loaded on first use
var _weapon_table_loaded := false
const ROOM_BAG := 0
const ROOM_REPOSITORY := 1
const ROOM_TRADE := 2
const ROOM_IMMEDIACY := 3
const ROOM_BODY := 10

var _account := ""
var _password := ""
var _move_seq := 0
var _ping_timer := 0.0
var _ping_sent_ms := 0
var _last_pong_ms := 0


func _ready() -> void:
	Net.connected.connect(_on_connected)
	Net.disconnected.connect(_on_disconnected)
	Net.message.connect(_on_message)


func _set_state(s: String) -> void:
	Log.debug("session", "state", {"from": state, "to": s})
	state = s


# ---- requests ----------------------------------------------------------------------------

# server is what the player typed: "host:port", "tls://host:port" or "ws(s)://host:port/ws".
func login(server: String, account: String, password: String) -> void:
	var a := Net.parse_address(server)
	host = a["host"]
	port = a["port"]
	_account = account
	_password = password
	_set_state("connecting")
	last_login_result = Proto.Result.OK
	if Net.connect_to(server) != OK:
		_set_state("offline")
		last_login_result = LOGIN_NO_CONNECTION
		login_result.emit(false, "Không kết nối được tới %s" % server)


func request_char_list() -> void:
	Net.send_msg(Proto.MsgId.C2G_CHAR_LIST, Proto.CharListReq.new())


func create_char(name: String, series: int, sex: int, native_place: int = 0) -> void:
	var req := Proto.CharCreateReq.new()
	req.set_name(name)
	req.set_series(series)
	req.set_sex(sex)
	if req.has_method("set_native_place"):
		req.set_native_place(native_place)
	Net.send_msg(Proto.MsgId.C2G_CHAR_CREATE, req)


func enter_world(pid: int) -> void:
	var req := Proto.EnterWorldReq.new()
	req.set_player_id(pid)
	player_id = pid
	Log.ctx["pid"] = pid
	_set_state("entering")
	Net.send_msg(Proto.MsgId.C2G_ENTER_WORLD, req)


func leave_world() -> void:
	if state == "world" or state == "entering":
		Net.send_msg(Proto.MsgId.C2G_LEAVE_WORLD, Proto.LeaveWorldReq.new())
		_set_state("lobby")
		entity_id = 0
		entities = {}
		items = {}
		skills = {}
		task_values = {}
		Log.ctx["zone"] = 0


func move_to(x: int, y: int) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.MoveReq.new()
	req.new_target().set_x(x)
	req.get_target().set_y(y)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_MOVE, req)
	Log.trace("world", "move request", {"x": x, "y": y, "seq": _move_seq})
	return _move_seq


# Basic melee attack on an entity (the zone keeps swinging until it dies or we move).
func attack(target_id: int) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.AttackReq.new()
	req.set_target(target_id)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_ATTACK, req)
	Log.trace("world", "attack request", {"target": target_id, "seq": _move_seq})
	return _move_seq


# NpcSkillCommand of the old client (KNpc::SendCommand(do_skill) on the server, C2G_CAST_SKILL): a skill on an
# entity (target_id > 0) or on a spot (x, y in scene units); the zone answers with the actions of the cast
func cast_skill(skill_id: int, target_id: int, x: int = 0, y: int = 0) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.CastSkillReq.new()
	req.set_skill_id(skill_id)
	req.set_target(target_id)
	req.set_x(x)
	req.set_y(y)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_CAST_SKILL, req)
	Log.trace("world", "cast request", {"skill": skill_id, "target": target_id, "x": x, "y": y, "seq": _move_seq})
	return _move_seq


# GOI_TONE_UP_SKILL of the old client: one skill point on a skill (KPlayer::AddSkillPoint of the zone)
func add_skill_point(skill_id: int, points: int = 1) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.AddSkillPointReq.new()
	req.set_skill_id(skill_id)
	req.set_points(points)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_ADD_SKILL_POINT, req)
	Log.trace("world", "skill point request", {"skill": skill_id, "points": points, "seq": _move_seq})
	return _move_seq


# A dead character asks to revive at its revive point (KPlayer::Revive(0) of the old server): the zone
# answers with an EntityAction ACTION_REVIVE, the life and the attributes.
func revive() -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.ReviveReq.new()
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_REVIVE, req)
	Log.trace("world", "revive request", {"seq": _move_seq})
	return _move_seq


# C2G_RIDE: mount (true) or dismount the worn horse - the ride toggle of the old client (0x080AEFA0)
# KNpc::SetAura 0x005EA870 of the 2.0 client: the right-mouse skill that is an aura (IsAura, LRSkill 2) is sent as the
# 0x6f packet {id} - the zone keeps casting its child (KNpc::SetAura 0x08087290 / ProcessState); 0 = no aura
func set_aura(skill_id: int) -> void:
	if state != "world":
		return
	var req := Proto.SetAuraReq.new()
	req.set_skill_id(maxi(skill_id, 0))
	Net.send_msg(Proto.MsgId.C2G_SET_AURA, req)
	aura_skill = skill_id
	Log.debug("world", "aura request", {"skill": skill_id})


# the 0x76 packet {0x76, byte state} of the PK switch (jx_linux_y 0x080DBE00 -> KPlayerPK::SetPKState 0x080C3740); the 2.0
# client's Switch([[pk]]) (F9 / Ctrl+H, OperationRequest 0x14 -> 0x005FB240) sends {0x6d, 2} instead, a packet this server
# reads as a trade - the zone takes the state the server handler expects
func pk_state_request(wanted: int) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.PKStateReq.new()
	req.set_state(wanted)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_PK_STATE, req)
	Log.trace("world", "pk state request", {"state": wanted, "seq": _move_seq})
	return _move_seq


# the 0x53 packet {0x53, word 7, byte sub, dword npc} of the 2.0 client (KPlayer team ops of core+0xa878: create 0x005F6F40,
# leave 0x005F7070, kick 0x005F70B0, change captain 0x005F7100, invite 0x005F75B0, open/close 0x005FA7C0, ...; the same
# sub-commands jx_linux_y reads at 0x080DCC90 - docs/LINUX-SERVER.md §17)
func team_request(cmd: int, target: int = 0, flag: int = 0) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.TeamReq.new()
	req.set_cmd(cmd)
	req.set_target(target)
	req.set_flag(flag)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_TEAM, req)
	Log.trace("world", "team request", {"cmd": cmd, "target": target, "flag": flag, "seq": _move_seq})
	return _move_seq


# the trade packets of the 2.0 client (KPlayer::TradeApplyOpen 0x005FB010 / Close 0x005F7460 {0x6a} / TradeApplyStart 0x005F7480
# {0x6b, npc} / the money 0x6c / the decision 0x6d 0x005FB201; docs/LINUX-SERVER.md §18)
func trade_request(cmd: int, target: int = 0, arg: int = 0, text: String = "") -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.TradeReq.new()
	req.set_cmd(cmd)
	req.set_target(target)
	req.set_arg(arg)
	req.set_text(text)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_TRADE, req)
	Log.trace("world", "trade request", {"cmd": cmd, "target": target, "arg": arg, "seq": _move_seq})
	return _move_seq


func trading() -> bool:
	return int(trade.state) == 2


# the entity ids of one's team mates (the captain and the members but oneself): the life bar of a team mate is
# (230, 190, 0) in PaintLife 0x005EADB8 (0x0066D070 == 8)
func team_mate_ids() -> Dictionary:
	var out := {}
	if not team.in_team:
		return out
	if not team.leader.is_empty() and int(team.leader.id) != entity_id:
		out[int(team.leader.id)] = true
	for m in team.members:
		if int(m.id) != entity_id:
			out[int(m.id)] = true
	return out


func is_team_mate(id: int) -> bool:
	if not team.in_team or id == entity_id:
		return false
	if not team.leader.is_empty() and int(team.leader.id) == id:
		return true
	for m in team.members:
		if int(m.id) == id:
			return true
	return false


# the 0x71 packet {0x71, byte sit} of the 2.0 client (the tool bar's Switch([[sit]]) 0x0044B470): sit down (1) / stand up (0)
func sit(on: bool) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.SitReq.new()
	req.set_sit(on)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_SIT, req)
	Log.trace("world", "sit request", {"sit": on, "seq": _move_seq})
	return _move_seq


func ride(on: bool) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.RideReq.new()
	req.set_on(on)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_RIDE, req)
	Log.trace("world", "ride request", {"on": on, "seq": _move_seq})
	return _move_seq


# a line on a channel (Proto.ChatChannel: CH_NEARBY 0, CH_TEAM 1, CH_WORLD 2, CH_FACTION 3, CH_CITY 5, CH_WHISPER 7
# with the name in `target`); the zone applies the rules of 0x081E3710 / 0x080502A0 (docs/LINUX-SERVER.md §19)
func chat(text: String, channel: int = 0, target: String = "") -> void:
	if state != "world" or text.strip_edges() == "":
		return
	var req := Proto.ChatReq.new()
	req.set_text(text)
	req.set_channel(channel)
	if target != "":
		req.set_target(target)
	Net.send_msg(Proto.MsgId.C2G_CHAT, req)


# ---- the npc dialog (docs/LINUX-SERVER.md §20) ----

# a click on a dialoger npc: the 0x6e packet (KPlayer::DialogNpc) - the zone runs the npc's script main() for us
func npc_dialog(npc: int) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.NpcDialogReq.new()
	req.set_npc(npc)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_NPC_DIALOG, req)
	Log.debug("world", "npc dialog request", {"npc": npc, "seq": _move_seq})
	return _move_seq


# the answer picked in the dialog: the 0x5f packet {0x5f, int index, int kind, 0, 0} (KPlayer::OnSelectFromUI 0x005FC7D0)
func dialog_answer(index: int, kind: int = 0) -> void:
	if state != "world":
		return
	var req := Proto.DialogAnswer.new()
	req.set_index(index)
	req.set_kind(kind)
	Net.send_msg(Proto.MsgId.C2G_DIALOG_ANSWER, req)
	Log.debug("world", "dialog answer", {"index": index, "kind": kind})


# ---- the task values (docs/LINUX-SERVER.md §21) ----

# the saved task value the zone last told us, 0 when it never did (GetTaskValue of the client's KPlayer)
func task_value(id: int) -> int:
	return KPlayerTask.value_of(task_values, id)


# the 0xaa packet {0xaa, int id, int value}: the zone keeps it for an id with CLIENT_FLAG in player_task_def.txt only
func set_task_value(id: int, value: int) -> void:
	if state != "world":
		return
	var req := Proto.TaskValueReq.new()
	req.set_id(id)
	req.set_value(value)
	Net.send_msg(Proto.MsgId.C2G_TASK_VALUE, req)
	Log.debug("world", "task value request", {"id": id, "value": value})


func _set_task_value(id: int, value: int) -> void:
	if KPlayerTask.set_value(task_values, id, value):
		task_value_changed.emit(id, value)


# ---- items: the requests share the move sequence so a G2C_ITEM_RESULT can be matched ----

# Put an item at (room, x, y).  An item lying exactly under the target trades places with it
# (the zone answers G2C_ITEM_MOVE for both), otherwise G2C_ITEM_RESULT says why not.
func item_move(id: int, room: int, x: int, y: int) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.ItemMove.new()
	req.set_id(id)
	req.set_room(room)
	req.set_x(x)
	req.set_y(y)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_ITEM_MOVE, req)
	Log.trace("item", "move request", {"id": id, "room": room, "x": x, "y": y, "seq": _move_seq})
	return _move_seq


# Wear an item; part -1 = the part its kind goes to (a ring: the first ring slot).
func item_equip(id: int, part: int = -1) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.ItemEquipReq.new()
	req.set_id(id)
	req.set_part(part)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_ITEM_EQUIP, req)
	Log.trace("item", "equip request", {"id": id, "part": part, "seq": _move_seq})
	return _move_seq


func item_unequip(part: int) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.ItemUnequipReq.new()
	req.set_part(part)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_ITEM_UNEQUIP, req)
	Log.trace("item", "unequip request", {"part": part, "seq": _move_seq})
	return _move_seq


# Eat a medicine / use an item (KItemList::EatMecidine): the zone answers with the item's new
# stack (G2C_ITEM_ADD) or G2C_ITEM_REMOVE when it was the last one.
func item_use(id: int) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.ItemUseReq.new()
	req.set_id(id)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_ITEM_USE, req)
	Log.trace("item", "use request", {"id": id, "seq": _move_seq})
	return _move_seq


# Spend attribute points (KPlayer::AddBaseStrength.. of the old game, c2s_playeraddattribute):
# 0 strength, 1 dexterity, 2 vitality, 3 energy.  The zone answers with G2C_PLAYER_ATTRIB.
func add_point(attribute: int, points: int) -> int:
	if state != "world" or points <= 0:
		return 0
	_move_seq += 1
	var req := Proto.AddPointReq.new()
	req.set_attribute(attribute)
	req.set_points(points)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_ADD_POINT, req)
	Log.trace("player", "add point request", {"attribute": attribute, "points": points, "seq": _move_seq})
	return _move_seq


# Pick a thing up from the ground (KPlayer::ServerPickUpItem): the zone answers with G2C_ITEM_ADD /
# G2C_MONEY and the object's despawn, or G2C_ITEM_RESULT (too far, kept for somebody else, no room).
func pick_up(entity_id: int) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.PickUpReq.new()
	req.set_entity_id(entity_id)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_PICK_UP, req)
	Log.trace("item", "pick up request", {"entity": entity_id, "seq": _move_seq})
	return _move_seq


func item_drop(id: int) -> int:
	if state != "world":
		return 0
	_move_seq += 1
	var req := Proto.ItemDropReq.new()
	req.set_id(id)
	req.set_seq(_move_seq)
	Net.send_msg(Proto.MsgId.C2G_ITEM_DROP, req)
	Log.trace("item", "drop request", {"id": id, "seq": _move_seq})
	return _move_seq


# The plain attack of the worn weapon (0x005EBBA0 for one's own character: the worn weapon's DetailType /
# ParticularType through \settings\武器物理攻击对照表.txt, no weapon -> the bare-hand row); 0 without a table
func weapon_attack_skill() -> int:
	if not _weapon_table_loaded:
		_weapon_table_loaded = true
		_weapon_table = KWeaponSkillTable.parse(Assets.load_json(Assets.assets_root() + "/weapon_skill.json"))
		if _weapon_table.get("bare", 0) == 0:
			Log.warn("player", "weapon skill table missing", {"file": Assets.assets_root() + "/weapon_skill.json"})
	var detail := -1
	var particular := -1
	var weapon := item_worn(ITEMPART_WEAPON)
	if weapon != 0:
		var it: Dictionary = items[weapon]
		detail = int(it.get("detail", -1))
		particular = int(it.get("particular", -1))
	return KWeaponSkillTable.skill_of(_weapon_table, detail, particular)


# UpdateWeaponSkill 0x005FE820 (KProtocolProcess::SyncEnd 0x00654F5B of the 2.0 client): both mouse skills become
# the weapon's plain attack; SetLeftSkill 0x005F7550 / SetRightSkill 0x005FB280 only take a skill held at level >= 1
func update_weapon_skill() -> void:
	var id := weapon_attack_skill()
	if id <= 0 or int(skills.get(id, {}).get("level", 0)) < 1:
		return
	if left_skill == id and right_skill == id:
		return
	left_skill = id
	right_skill = id
	Log.debug("player", "weapon skill", {"skill": id, "weapon": item_worn(ITEMPART_WEAPON)})
	mouse_skill_changed.emit()


# C2G_SKILL_DESC: the numbers of a skill level for its tip (KSkill::GetDesc 0x006FBC90 of the 2.0 client runs the level
# script itself; the zone runs it for this client) - the answer lands in skill_descs and skill_desc_received
func skill_desc_request(skill_id: int, level: int) -> void:
	if state != "world":
		return
	var req := Proto.SkillDescReq.new()
	req.set_skill_id(skill_id)
	req.set_level(maxi(0, level))
	Net.send_msg(Proto.MsgId.C2G_SKILL_DESC, req)
	Log.trace("player", "skill desc request", {"skill": skill_id, "level": level})


# the answer kept for (skill, level), null while none arrived
func skill_desc(skill_id: int, level: int):
	return skill_descs.get("%d:%d" % [skill_id, maxi(0, level)])


# text/skill_desc.json of jxassets export-skill-desc: {strings (G_*), descript, skill_attrib, skill_type, weapon_limit}
func skill_text() -> Dictionary:
	if not _skill_text_loaded:
		_skill_text_loaded = true
		var d = Assets.load_json(Assets.assets_root() + "/text/skill_desc.json")
		if d is Dictionary:
			_skill_text = d
		else:
			Log.warn("player", "skill texts missing", {"file": Assets.assets_root() + "/text/skill_desc.json"})
	return _skill_text


# the cells of a skills.json row (SkillName, SkillDesc, ReqLevel, Attrib, ...), empty for an unknown id
func skill_row(skill_id: int) -> Dictionary:
	if not _skill_rows_loaded:
		_skill_rows_loaded = true
		var tab = Assets.load_json("%s/skills.json" % Assets.assets_root())
		if tab is Dictionary:
			for r in tab.get("rows", []):
				_skill_rows[int(r.get("id", 0))] = r.get("cells", {})
	return _skill_rows.get(skill_id, {})


# the drawing row of a missile (AnimFile* of missles.txt as jxassets export-missle-res wrote them), empty when unknown
func missle_row(missle_id: int) -> Dictionary:
	if not _missle_res_loaded:
		_missle_res_loaded = true
		var d = Assets.load_json(Assets.assets_root() + "/missles/missle_res.json")
		if d is Dictionary:
			_missle_res = d.get("rows", {})
		else:
			Log.warn("world", "missile drawings missing", {"file": Assets.assets_root() + "/missles/missle_res.json"})
	return _missle_res.get(str(missle_id), {})


func skill_name(skill_id: int) -> String:
	return str(skill_row(skill_id).get("SkillName", str(skill_id)))


func _skill_desc_level(l) -> Dictionary:
	var attribs := []
	for a in l.get_attribs():
		attribs.append({"group": int(a.get_group()), "name": str(a.get_name()), "v0": int(a.get_v0()), "v1": int(a.get_v1()), "v2": int(a.get_v2())})
	var appends := []
	for p in l.get_appends():
		appends.append({"skill_id": int(p.get_skill_id()), "value": int(p.get_value())})
	var related := []
	for r in l.get_related():
		var ra := []
		for a in r.get_attribs():
			ra.append({"group": int(a.get_group()), "name": str(a.get_name()), "v0": int(a.get_v0()), "v1": int(a.get_v1()), "v2": int(a.get_v2())})
		related.append({"skill_id": int(r.get_skill_id()), "level": int(r.get_level()), "flags": int(r.get_flags()), "attribs": ra})
	return {"level": int(l.get_level()), "cost": int(l.get_cost()), "cost_type": int(l.get_cost_type()),
		"attack_radius": int(l.get_attack_radius()), "attribs": attribs, "appends": appends, "related": related}


# The item worn on a part (0 = none), and what lies on a cell of a room (0 = nothing)
func item_worn(part: int) -> int:
	for id in items:
		var it: Dictionary = items[id]
		if it.room == ROOM_BODY and it.x == part:
			return int(id)
	return 0


func item_at(room: int, x: int, y: int) -> int:
	for id in items:
		var it: Dictionary = items[id]
		if it.room == room and x >= it.x and x < it.x + it.w and y >= it.y and y < it.y + it.h:
			return int(id)
	return 0


func ping() -> void:
	var p := Proto.Ping.new()
	_ping_sent_ms = Time.get_ticks_msec()
	p.set_client_ms(_ping_sent_ms)
	Net.send_msg(Proto.MsgId.C2G_PING, p)


func logout(reason: String = "logout") -> void:
	Net.disconnect_from(reason)
	_set_state("offline")


func _process(delta: float) -> void:
	if state == "world":
		_ping_timer += delta
		if _ping_timer >= PING_INTERVAL:
			_ping_timer = 0.0
			ping()
		# heartbeat watchdog: a dead link is noticed here, not after minutes of TCP retries
		if _last_pong_ms > 0 and Time.get_ticks_msec() - _last_pong_ms > int(PONG_TIMEOUT * 1000):
			Log.warn("net", "no pong from gateway", {"seconds": PONG_TIMEOUT})
			last_notice = KLogin.result_text(Proto.Result.TIMEOUT)
			Net.disconnect_from("timeout")


# ---- connection events -------------------------------------------------------------------

func _on_connected() -> void:
	var hello := Proto.Hello.new()
	hello.set_protocol_version(Proto.Protocol.VERSION)   # godobuf strips the enum name prefix
	hello.set_client_version(CLIENT_VERSION)
	hello.set_platform(OS.get_name().to_lower())
	hello.set_device_id(OS.get_unique_id())
	_set_state("hello")
	Net.send_msg(Proto.MsgId.C2G_HELLO, hello)


func _on_disconnected(reason: String) -> void:
	var was := state
	_set_state("offline")
	sid = 0
	entity_id = 0
	entities = {}
	Log.ctx["sid"] = 0
	Log.ctx["zone"] = 0
	_last_pong_ms = 0
	if last_notice == "" and reason != "logout" and reason != "back to login":
		last_notice = "Mất kết nối: " + reason
	if was == "connecting" or was == "hello" or was == "auth":
		if last_login_result == Proto.Result.OK:
			last_login_result = LOGIN_NO_CONNECTION
		login_result.emit(false, last_notice if last_notice != "" else "Mất kết nối: " + reason)
	connection_lost.emit(reason)


func _decode(msg, payload: PackedByteArray) -> bool:
	var err = msg.from_bytes(payload)
	if err != Proto.PB_ERR.NO_ERRORS:
		Log.error("net", "decode failed", {"error": str(err)})
		return false
	return true


func _on_message(msg_id: int, payload: PackedByteArray) -> void:
	match msg_id:
		Proto.MsgId.G2C_HELLO_ACK:
			var ack := Proto.HelloAck.new()
			if not _decode(ack, payload):
				return
			sid = ack.get_sid()
			auth_mode = ack.get_auth_mode()
			heartbeat_s = maxi(int(ack.get_heartbeat_s()), 1)
			Log.ctx["sid"] = sid
			Log.info("net", "hello ack", {"server_version": ack.get_server_version(), "protocol": ack.get_protocol_version(),
				"auth_mode": auth_mode, "heartbeat_s": heartbeat_s})
			var req := Proto.LoginReq.new()
			req.set_account(_account)
			req.set_password(_password)
			_set_state("auth")
			Net.send_msg(Proto.MsgId.C2G_LOGIN, req)

		Proto.MsgId.G2C_LOGIN_RES:
			var res := Proto.LoginRes.new()
			if not _decode(res, payload):
				return
			if res.get_result() == Proto.Result.OK:
				account_id = res.get_account_id()
				_set_state("lobby")
				Log.info("auth", "login ok", {"account": _account, "account_id": account_id})
				login_result.emit(true, "")
			else:
				last_login_result = res.get_result()
				Log.warn("auth", "login failed", {"result": res.get_result(), "text": res.get_text()})
				login_result.emit(false, KLogin.result_text(res.get_result(), res.get_text()))

		Proto.MsgId.G2C_CHAR_LIST_RES:
			var res := Proto.CharListRes.new()
			if not _decode(res, payload):
				return
			chars = []
			for c in res.get_chars():
				chars.append(_summary_dict(c))
			max_chars = maxi(int(res.get_max_chars()), 1)
			Log.info("lobby", "char list", {"count": chars.size(), "max": res.get_max_chars()})
			char_list.emit(chars)

		Proto.MsgId.G2C_CHAR_CREATE_RES:
			var res := Proto.CharCreateRes.new()
			if not _decode(res, payload):
				return
			var ok: bool = res.get_result() == Proto.Result.OK
			var summary := _summary_dict(res.get_summary()) if ok else {}
			Log.info("lobby", "char create", {"ok": ok, "result": res.get_result()})
			char_created.emit(ok, res.get_result(), summary)

		Proto.MsgId.G2C_ENTER_WORLD_RES:
			var res := Proto.EnterWorldRes.new()
			if not _decode(res, payload):
				return
			if res.get_result() == Proto.Result.OK:
				entity_id = res.get_entity_id()
				zone_id = res.get_zone_id()
				zone_name = res.get_zone_name()
				tick_hz = res.get_tick_hz()
				map_id = res.get_map_id()
				scene_w = res.get_scene_w()
				scene_h = res.get_scene_h()
				Log.ctx["zone"] = zone_id
				entities = {}
				_last_pong_ms = Time.get_ticks_msec()
				_ping_timer = PING_INTERVAL   # first heartbeat right away
				_set_state("world")
				var info := {"zone_id": zone_id, "zone_name": zone_name, "entity_id": entity_id,
					"x": res.get_pos().get_x(), "y": res.get_pos().get_y(), "tick_hz": tick_hz,
					"map_id": map_id, "scene_w": scene_w, "scene_h": scene_h}
				Log.info("world", "entered", info)
				entered_world.emit(info)
			else:
				_set_state("lobby")
				Log.warn("world", "enter failed", {"result": res.get_result()})
				enter_failed.emit(res.get_result())

		Proto.MsgId.G2C_CHANGE_MAP:
			var cm := Proto.ChangeMap.new()
			if not _decode(cm, payload):
				return
			map_id = cm.get_map_id()
			scene_w = cm.get_scene_w()
			scene_h = cm.get_scene_h()
			entity_id = cm.get_entity_id()
			entities = {}
			var cinfo := {"map_id": map_id, "scene_w": scene_w, "scene_h": scene_h, "entity_id": entity_id,
				"x": cm.get_pos().get_x(), "y": cm.get_pos().get_y()}
			Log.info("world", "map changed", cinfo)
			map_changed.emit(cinfo)

		Proto.MsgId.G2C_ENTITY_SPAWN:
			var m := Proto.EntitySpawn.new()
			if not _decode(m, payload):
				return
			var list: Array = []
			for e in m.get_entities():
				var d := _entity_dict(e)
				entities[int(d.id)] = d
				list.append(d)
			Log.debug("world", "spawn", {"count": list.size(), "total": entities.size()})
			entity_spawn.emit(list)

		Proto.MsgId.G2C_ENTITY_DESPAWN:
			var m := Proto.EntityDespawn.new()
			if not _decode(m, payload):
				return
			var ids := Array(m.get_entity_ids())
			for id in ids:
				entities.erase(int(id))
			Log.debug("world", "despawn", {"count": ids.size(), "total": entities.size()})
			entity_despawn.emit(ids)

		Proto.MsgId.G2C_ENTITY_MOVE:
			var m := Proto.EntityMove.new()
			if not _decode(m, payload):
				return
			_apply_move(m)

		Proto.MsgId.G2C_ENTITY_MOVES:
			# what moved far from us, gathered by the zone over a few ticks (N3): each entry is a
			# whole EntityMove, handled exactly like one that came alone
			var ms := Proto.EntityMoves.new()
			if not _decode(ms, payload):
				return
			for m in ms.get_moves():
				_apply_move(m)

		Proto.MsgId.G2C_ENTITY_ACTION:
			var m := Proto.EntityAction.new()
			if not _decode(m, payload):
				return
			var aim = m.get_aim()   # the spot of a spot cast or a jump; null when the action has a target
			var a := {"id": m.get_entity_id(), "action": int(m.get_action()), "target": m.get_target(), "dir": m.get_dir(),
				"frames": m.get_frames(), "x": m.get_pos().get_x(), "y": m.get_pos().get_y(), "tick": m.get_tick(),
				"skill": int(m.get_skill_id()), "ax": aim.get_x() if aim != null else m.get_pos().get_x(),
				"ay": aim.get_y() if aim != null else m.get_pos().get_y()}
			var d = entities.get(int(a.id))
			if d != null:
				d.doing = a.action
				d.doing_frames = a.frames
				d.x = a.x
				d.y = a.y
				d.path = []
			entity_action.emit(a)

		Proto.MsgId.G2C_ENTITY_LIFE:
			var m := Proto.EntityLife.new()
			if not _decode(m, payload):
				return
			var l := {"id": m.get_entity_id(), "life": m.get_life(), "life_max": m.get_life_max(), "delta": m.get_delta(),
				"source": m.get_source()}
			var d = entities.get(int(l.id))
			if d != null:
				d.life = l.life
				d.life_max = l.life_max
			entity_life.emit(l)

		Proto.MsgId.G2C_ENTITY_RIDE:
			var m := Proto.EntityRide.new()
			if not _decode(m, payload):
				return
			var d = entities.get(int(m.get_entity_id()))
			if d != null:
				d.riding = m.get_riding()
			entity_ride.emit({"id": m.get_entity_id(), "riding": m.get_riding()})

		Proto.MsgId.G2C_ENTITY_CAMP:
			# the 0x59 handler (slot 0x5a of the 2.0 client): the npc's camp; the player's own npc too
			var m := Proto.EntityCamp.new()
			if not _decode(m, payload):
				return
			var d = entities.get(int(m.get_entity_id()))
			if d != null:
				d.camp = m.get_camp()
				d.current_camp = m.get_current_camp()
			if int(m.get_entity_id()) == entity_id:
				camp = m.get_camp()
			entity_camp.emit({"id": m.get_entity_id(), "camp": m.get_camp(), "current_camp": m.get_current_camp()})

		Proto.MsgId.G2C_PLAYER_FACTION:
			# the 0x7b handler 0x00651280: camp, current (+0x12078), last added (+0x12080), count (+0x12084), then the UI
			# is told; the 0x7c one (0x006511B0) leaves the current at -1 and the camp at 4 - the same message says so
			var m := Proto.PlayerFaction.new()
			if not _decode(m, payload):
				return
			camp = m.get_camp()
			faction = m.get_faction()
			faction_last = m.get_faction_last()
			faction_count = m.get_faction_count()
			Log.info("net", "faction", {"faction": faction, "last": faction_last, "count": faction_count, "camp": camp})
			faction_changed.emit()

		Proto.MsgId.G2C_MISSLE:
			var m := Proto.MissleSync.new()
			if not _decode(m, payload):
				return
			missle_packets += 1
			missle_sync.emit({"index": int(m.get_index()), "missle_id": int(m.get_missle_id()), "skill_id": int(m.get_skill_id()),
				"level": int(m.get_level()), "launcher": m.get_launcher(), "x": int(m.get_x()), "y": int(m.get_y()), "z": int(m.get_z()),
				"dir": int(m.get_dir()), "x_factor": int(m.get_x_factor()), "y_factor": int(m.get_y_factor()), "speed": int(m.get_speed()),
				"life_time": int(m.get_life_time()), "start_life_time": int(m.get_start_life_time()), "current_life": int(m.get_current_life()),
				"status": int(m.get_status()), "removed": m.get_removed(), "move_kind": int(m.get_move_kind()), "collided": m.get_collided(),
				"height": int(m.get_height()), "height_speed": int(m.get_height_speed()), "z_acceleration": int(m.get_z_acceleration())})

		Proto.MsgId.G2C_SKILL_DESC:
			var m := Proto.SkillDesc.new()
			if not _decode(m, payload):
				return
			var d := {"skill_id": int(m.get_skill_id()), "max_level": int(m.get_max_level()), "has_cur": m.get_with_cur(), "has_next": m.get_with_next(),
				"level_inc": int(m.get_level_inc()), "enhance": int(m.get_enhance()), "held_level": int(m.get_held_level()),
				"equip_percent": int(m.get_equip_percent())}
			if m.get_with_modifier():
				var ma = m.get_modifier()
				d["modifier"] = {"group": int(ma.get_group()), "name": str(ma.get_name()), "v0": int(ma.get_v0()), "v1": int(ma.get_v1()), "v2": int(ma.get_v2())}
			if m.get_with_cur():
				d["cur"] = _skill_desc_level(m.get_cur())
			if m.get_with_next():
				d["next"] = _skill_desc_level(m.get_next())
			var level: int = int(d.cur.level) if m.get_with_cur() else 0
			skill_descs["%d:%d" % [d.skill_id, level]] = d
			if level != int(m.get_held_level()):
				skill_descs["%d:%d" % [d.skill_id, int(m.get_held_level())]] = d   # asked at another level: the zone answered for the one it holds
			Log.debug("player", "skill desc", {"skill": d.skill_id, "level": level, "max": d.max_level})
			skill_desc_received.emit(int(d.skill_id))

		Proto.MsgId.G2C_STATE_ICONS:
			# the 0x7a packet: the six state icons over an npc (KNpc 0x08079F60); kept with the entity, drawn later (B4e)
			var m := Proto.EntityStateIcons.new()
			if not _decode(m, payload):
				return
			var d = entities.get(int(m.get_entity_id()))
			if d != null:
				var icons := []
				for v in m.get_icons():
					icons.append(int(v))
				d["state_icons"] = icons
				state_icons_changed.emit(int(m.get_entity_id()))

		Proto.MsgId.G2C_PK_STATE:
			# the 0x90 handler (state) / 0x93 (value) of the 2.0 client: one's own KPlayerPK
			var m := Proto.PKState.new()
			if not _decode(m, payload):
				return
			pk_state = int(m.get_state())
			pk_value = int(m.get_value())
			var own = entities.get(entity_id)
			if own != null:
				own["pk_state"] = pk_state
			pk_changed.emit(pk_state, pk_value, bool(m.get_refused()))
			Log.info("player", "pk state", {"state": pk_state, "value": pk_value, "refused": bool(m.get_refused())})

		Proto.MsgId.G2C_ENTITY_PK:
			# the 0x4b sync's flag & 3 -> KNpc+0x16e4 of the others (0x0065D617): the life-bar colour
			var m := Proto.EntityPK.new()
			if not _decode(m, payload):
				return
			var d = entities.get(int(m.get_entity_id()))
			if d != null:
				d["pk_state"] = int(m.get_pk_state())
			entity_pk.emit({"id": int(m.get_entity_id()), "pk_state": int(m.get_pk_state())})

		Proto.MsgId.G2C_TRADE_STATE:
			# s2c_tradechangestate of KPlayerMenuState::SetState 0x080C29D0: 0 normal, 1 open for trade, 2 trading {partner}
			var m := Proto.TradeState.new()
			if not _decode(m, payload):
				return
			var was := int(trade.state)
			trade.state = int(m.get_state())
			trade.partner = int(m.get_partner())
			trade.partner_name = str(m.get_partner_name())
			if trade.state != 2 or was != 2:
				trade.self_lock = false
				trade.dest_lock = false
				trade.self_ok = false
				trade.dest_ok = false
				trade.self_money = 0
				trade.dest_money = 0
				trade.other_items = {}
			trade_changed.emit()
			Log.info("player", "trade state", {"state": trade.state, "partner": trade.partner, "name": trade.partner_name})

		Proto.MsgId.G2C_TRADE_SYNC:
			# the 0x81 packet of SyncTradeState 0x080A85B0 {self lock, dest lock, self ok, dest ok} + the 0x77 money of the partner
			var m := Proto.TradeSync.new()
			if not _decode(m, payload):
				return
			trade.self_lock = bool(m.get_self_lock())
			trade.dest_lock = bool(m.get_dest_lock())
			trade.self_ok = bool(m.get_self_ok())
			trade.dest_ok = bool(m.get_dest_ok())
			trade.self_money = int(m.get_self_money())
			trade.dest_money = int(m.get_dest_money())
			trade_changed.emit()
			Log.info("player", "trade sync", {"self_lock": trade.self_lock, "dest_lock": trade.dest_lock, "self_ok": trade.self_ok,
				"dest_ok": trade.dest_ok, "self_money": trade.self_money, "dest_money": trade.dest_money})

		Proto.MsgId.G2C_TRADE_ITEM:
			# the partner put an item on the table (the 0xcc-byte sync of ExchangeItem 0x08207172) or took it back
			var m := Proto.TradeItem.new()
			if not _decode(m, payload):
				return
			var removed := bool(m.get_removed())
			var d := _item_dict(m.get_item()) if m.has_item() else {}
			var id := int(d.get("id", 0))
			if removed:
				trade.other_items.erase(id)
			elif id != 0:
				trade.other_items[id] = d
			trade_item.emit({"item": d, "removed": removed})
			trade_changed.emit()

		Proto.MsgId.G2C_TRADE_APPLY:
			# the 0x8b packet (0x080B4DE0): somebody asks to trade with me
			var m := Proto.TradeApply.new()
			if not _decode(m, payload):
				return
			trade_apply.emit({"id": int(m.get_entity_id()), "name": str(m.get_name())})
			Log.info("player", "trade apply", {"id": int(m.get_entity_id()), "name": str(m.get_name())})

		Proto.MsgId.G2C_TRADE_END:
			# the 0x78 packet: over - the state packet that follows (NORMAL, or the one before a cancel) resets the table
			var m := Proto.TradeEnd.new()
			if not _decode(m, payload):
				return
			trade.other_items = {}
			trade.self_money = 0
			trade.dest_money = 0
			trade_end.emit(bool(m.get_ok()))
			trade_changed.emit()
			Log.info("player", "trade end", {"ok": bool(m.get_ok())})

		Proto.MsgId.G2C_SYS_MSG:
			# the 0x86 packet {word 8, word id, dword npc}: the client's 0x00657AD0 picks the sentence by id
			var m := Proto.SysMsg.new()
			if not _decode(m, payload):
				return
			sys_msg.emit(int(m.get_id()), int(m.get_entity_id()), str(m.get_name()))

		Proto.MsgId.G2C_SCRIPT_ACTION:
			var m := Proto.ScriptAction.new()
			if not _decode(m, payload):
				return
			var options: Array = []
			for o in m.get_options():
				options.append(str(o))
			var a := {"operate": int(m.get_operate()), "ui": int(m.get_ui_id()), "text": str(m.get_text()), "text_id": int(m.get_text_id()),
				"interactive": bool(m.get_interactive()), "param": int(m.get_param()), "options": options}
			Log.debug("world", "script action", {"ui": a.ui, "options": options.size(), "param": a.param})
			script_action.emit(a)

		Proto.MsgId.G2C_TASK_VALUE:
			# the 0xa7 packet {id, value}: the client's 0x006512F0 -> KPlayer::SetTaskValue 0x00601ED0 (+ the ui message 0x54)
			var m := Proto.TaskValue.new()
			if not _decode(m, payload):
				return
			task_packets += 1
			_set_task_value(int(m.get_id()), int(m.get_value()))

		Proto.MsgId.G2C_TASK_VALUES:
			# the 0xb5 packet: up to eighty {id, value} (the client's 0x00651350 applies each without a ui message)
			var m := Proto.TaskValues.new()
			if not _decode(m, payload):
				return
			task_packets += 1
			for v in m.get_values():
				_set_task_value(int(v.get_id()), int(v.get_value()))

		Proto.MsgId.G2C_ENTITY_MENU_STATE:
			# s2c_npcsetmenustate (the client's 0x006522F0 -> KNpc 0x005EB2A0): the sign over a player's head, its sentence
			var m := Proto.EntityMenuState.new()
			if not _decode(m, payload):
				return
			var d = entities.get(int(m.get_entity_id()))
			if d != null:
				d["menu_state"] = int(m.get_state())
				d["menu_sentence"] = str(m.get_sentence())
			entity_menu_state.emit(int(m.get_entity_id()))

		Proto.MsgId.G2C_TEAM_SELF:
			# the 0x69 sub 2 handler of the 2.0 client (0x005F8280: +0x7258 flag, +0x725c figure, the captain 0x1f17610, the
			# members 0x1f17614.., the names, the levels, the lead exp +0x7230 -> level) or sub 9 with one's own npc (out of a team)
			var m := Proto.TeamSelf.new()
			if not _decode(m, payload):
				return
			var members: Array = []
			for mm in m.get_members():
				members.append({"id": int(mm.get_entity_id()), "name": str(mm.get_name()), "level": int(mm.get_level())})
			var leader := {}
			if m.get_in_team() and m.has_leader():
				var l = m.get_leader()
				leader = {"id": int(l.get_entity_id()), "name": str(l.get_name()), "level": int(l.get_level())}
			team = {"in_team": bool(m.get_in_team()), "team_id": int(m.get_team_id()), "state": int(m.get_state()),
				"captain": bool(m.get_captain()), "leader": leader, "members": members,
				"lead_level": int(m.get_lead_level()), "lead_exp": int(m.get_lead_exp()), "members_max": int(m.get_members_max())}
			team_changed.emit()
			Log.info("player", "team", {"in_team": team.in_team, "team": team.team_id, "state": team.state, "captain": team.captain,
				"members": members.size(), "lead_level": team.lead_level})

		Proto.MsgId.G2C_TEAM_EVENT:
			# the other 0x69 sub-commands (4 create ok, 5 create fail, 6 open/close, 7 apply, 8 add member, 9 leave, 0xc invite, 0xd
			# change captain, ...) and the 0x86 team messages: the windows decide what to show
			var m := Proto.TeamEvent.new()
			if not _decode(m, payload):
				return
			var ev := {"event": int(m.get_event()), "id": int(m.get_entity_id()), "name": str(m.get_name()), "level": int(m.get_level()),
				"arg": int(m.get_arg()), "leader": {}, "members": []}
			if m.has_leader():
				var l = m.get_leader()
				ev.leader = {"id": int(l.get_entity_id()), "name": str(l.get_name()), "level": int(l.get_level())}
			for mm in m.get_members():
				ev.members.append({"id": int(mm.get_entity_id()), "name": str(mm.get_name()), "level": int(mm.get_level())})
			team_event.emit(ev)
			Log.info("player", "team event", {"event": ev.event, "id": ev.id, "name": ev.name, "arg": ev.arg})

		Proto.MsgId.G2C_ENTITY_RES:
			# the 0xad handler of the 2.0 client (0x006515A0): the rows into KNpc::SetPlayerRes 0x005ED920, the version into +0x1408
			var m := Proto.EntityRes.new()
			if not _decode(m, payload):
				return
			var d = entities.get(int(m.get_entity_id()))
			var rows := {0: int(m.get_helm_res()), 1: int(m.get_armor_res()), 2: int(m.get_weapon_res()), 3: int(m.get_horse_res()),
				4: int(m.get_mantle_res())}
			if d != null:
				d["res"] = rows
			entity_res.emit({"id": int(m.get_entity_id()), "res": rows, "version": int(m.get_version())})

		Proto.MsgId.G2C_NPC_GOLD:
			# the 0x9a handler of the 2.0 client (0x00653110): a npc (kind 0) -> KNpcGold::SetGoldType(word) 0x006E3560
			var m := Proto.NpcGold.new()
			if not _decode(m, payload):
				return
			var d = entities.get(int(m.get_entity_id()))
			if d != null and int(d.get("type", 0)) != Proto.EntityType.ENTITY_PLAYER:
				d["gold_type"] = int(m.get_gold_type())
				gold_changed.emit(int(m.get_entity_id()))
				Log.debug("npc", "gold monster", {"entity": int(m.get_entity_id()), "type": int(m.get_gold_type())})

		Proto.MsgId.G2C_ENTITY_STATE:
			# the 0x87 handler of the 2.0 client (0x006526E0 -> KNpc::SetStateSkillEffect 0x005EDFC0): the character's own states
			var m := Proto.EntityState.new()
			if not _decode(m, payload):
				return
			var sid := int(m.get_skill_id())
			if m.get_removed() or int(m.get_entity_id()) != entity_id:
				states.erase(sid)
			else:
				var list := []
				for a in m.get_states():
					list.append({"type": a.get_type(), "v0": a.get_v0(), "v1": a.get_v1(), "v2": a.get_v2()})
				var frames := int(m.get_time())
				states[sid] = {"level": m.get_level(), "time": frames, "special_id": m.get_special_id(),
					"until_ms": Time.get_ticks_msec() + (frames * 1000 / 18 if frames >= 0 else 0), "states": list}
			Log.debug("net", "skill state", {"skill": sid, "level": m.get_level(), "frames": m.get_time(), "removed": m.get_removed(), "held": states.size()})
			state_changed.emit(sid)

		Proto.MsgId.G2C_CHAT_MSG:
			var m := Proto.ChatMsg.new()
			if not _decode(m, payload):
				return
			chat_msg.emit({"id": m.get_entity_id(), "name": m.get_name(), "text": m.get_text(), "channel": int(m.get_channel())})

		Proto.MsgId.G2C_ITEM_LIST:
			var m := Proto.InventorySync.new()
			if not _decode(m, payload):
				return
			items = {}
			for v in m.get_items():
				var d := _item_dict(v)
				items[int(d.id)] = d
			money = m.get_money()
			bank_money = m.get_bank_money()
			Log.debug("item", "item list", {"count": items.size(), "money": money, "bank_money": bank_money})
			items_changed.emit()
			update_weapon_skill()   # SyncEnd of the 2.0 client: the mouse skills follow the worn weapon
			money_changed.emit(money, bank_money)

		Proto.MsgId.G2C_ITEM_ADD:
			var m := Proto.ItemAdd.new()
			if not _decode(m, payload):
				return
			var d := _item_dict(m.get_item())
			items[int(d.id)] = d
			Log.debug("item", "item added", {"id": d.id, "name": d.name, "count": d.count, "room": d.room, "x": d.x, "y": d.y})
			item_changed.emit(d)

		Proto.MsgId.G2C_ITEM_REMOVE:
			var m := Proto.ItemRemove.new()
			if not _decode(m, payload):
				return
			var rid := int(m.get_id())
			items.erase(rid)
			Log.debug("item", "item removed", {"id": rid})
			item_removed.emit(rid)

		Proto.MsgId.G2C_ITEM_MOVE:
			var m := Proto.ItemMove.new()
			if not _decode(m, payload):
				return
			var d = items.get(int(m.get_id()))
			if d == null:
				Log.warn("item", "moved item unknown", {"id": m.get_id()})
				return
			var was_weapon: bool = int(d.room) == ROOM_BODY and int(d.x) == ITEMPART_WEAPON
			d.room = int(m.get_room())
			d.x = int(m.get_x())
			d.y = int(m.get_y())
			Log.debug("item", "item moved", {"id": d.id, "room": d.room, "x": d.x, "y": d.y, "seq": m.get_seq()})
			item_changed.emit(d)
			if was_weapon or (d.room == ROOM_BODY and d.x == ITEMPART_WEAPON):
				update_weapon_skill()   # the weapon changed: the mouse skills follow it (as after SyncEnd)

		Proto.MsgId.G2C_ITEM_RESULT:
			var m := Proto.ItemResult.new()
			if not _decode(m, payload):
				return
			Log.debug("item", "item request refused", {"seq": m.get_seq(), "result": int(m.get_result())})
			item_result.emit(m.get_seq(), int(m.get_result()))

		Proto.MsgId.G2C_MONEY:
			var m := Proto.MoneySync.new()
			if not _decode(m, payload):
				return
			money = m.get_money()
			bank_money = m.get_bank_money()
			money_changed.emit(money, bank_money)

		Proto.MsgId.G2C_SKILL_LIST:
			var m := Proto.SkillListSync.new()
			if not _decode(m, payload):
				return
			skills = {}
			for e in m.get_skills():
				skills[int(e.get_skill_id())] = _skill_dict(e)
			skills_forbidden = m.get_forbid_all()
			Log.debug("player", "skill list", {"count": skills.size(), "forbid_all": skills_forbidden})
			skills_changed.emit()
			update_weapon_skill()

		Proto.MsgId.G2C_SKILL_LEVEL:
			var m := Proto.SkillLevelSync.new()
			if not _decode(m, payload):
				return
			var sid := int(m.get_skill_id())
			if m.get_level() < 0:
				skills.erase(sid)   # DelMagic: the skill is gone
			else:
				var d: Dictionary = skills.get(sid, {"id": sid, "level": 0, "current_level": 0, "exp_percent": 0, "max_level": 0,
					"req_level": 0, "forbidden": false, "cool_down_left": 0, "only_inc": false})
				d.level = m.get_level()
				if d.current_level < d.level:
					d.current_level = d.level
				d.exp_percent = m.get_exp_percent()
				skills[sid] = d
			player_attrib.skill_point = m.get_skill_point()
			Log.debug("player", "skill level", {"skill": sid, "level": m.get_level(), "skill_point": m.get_skill_point(), "seq": m.get_seq(), "level_up": m.get_level_up()})
			skill_changed.emit(sid)

		Proto.MsgId.G2C_SKILL_FORBID:
			var m := Proto.SkillForbidSync.new()
			if not _decode(m, payload):
				return
			if m.get_skill_id() == 0:
				skills_forbidden = m.get_forbid()
			elif skills.has(int(m.get_skill_id())):
				skills[int(m.get_skill_id())].forbidden = m.get_forbid()
			skill_changed.emit(int(m.get_skill_id()))

		Proto.MsgId.G2C_PLAYER_ATTRIB:
			var m := Proto.PlayerAttribSync.new()
			if not _decode(m, payload):
				return
			# the login sync of the old game carried the faction record too (0x080A9750 +0xb0e / +0xb12)
			var faction_was := faction_last
			faction = m.get_faction()
			faction_last = m.get_faction_last()
			player_attrib = {
				"level": m.get_level(), "exp": m.get_exp(), "next_level_exp": m.get_next_level_exp(), "level_exp": m.get_level_exp(),
				"attribute_point": m.get_attribute_point(), "skill_point": m.get_skill_point(),
				"faction": m.get_faction(), "faction_last": m.get_faction_last(),
				"strength": m.get_strength(), "dexterity": m.get_dexterity(), "vitality": m.get_vitality(),
				"energy": m.get_energy(), "lucky": m.get_lucky(),
				"cur_strength": m.get_cur_strength(), "cur_dexterity": m.get_cur_dexterity(),
				"cur_vitality": m.get_cur_vitality(), "cur_energy": m.get_cur_energy(), "cur_lucky": m.get_cur_lucky(),
				"life": m.get_life(), "life_max": m.get_life_max(), "mana": m.get_mana(), "mana_max": m.get_mana_max(),
				"stamina": m.get_stamina(), "stamina_max": m.get_stamina_max(),
				"attack_rating": m.get_attack_rating(), "defend": m.get_defend(),
				"min_damage": m.get_min_damage(), "max_damage": m.get_max_damage(),
				"fire_resist": m.get_fire_resist(), "cold_resist": m.get_cold_resist(), "poison_resist": m.get_poison_resist(),
				"light_resist": m.get_light_resist(), "physics_resist": m.get_physics_resist(),
				"walk_speed": m.get_walk_speed(), "run_speed": m.get_run_speed(),
				"attack_speed": m.get_attack_speed(), "cast_speed": m.get_cast_speed(), "seq": m.get_seq(),
			}
			# our own entity follows (the level and the life the world draws)
			var me = entities.get(entity_id)
			if me != null:
				me.level = m.get_level()
				me.life = m.get_life()
				me.life_max = m.get_life_max()
			Log.debug("player", "attributes", {"level": m.get_level(), "exp": m.get_exp(), "points": m.get_attribute_point(), "life": m.get_life(), "life_max": m.get_life_max()})
			player_attrib_changed.emit(player_attrib)

		Proto.MsgId.G2C_PONG:
			var m := Proto.Pong.new()
			if not _decode(m, payload):
				return
			_last_pong_ms = Time.get_ticks_msec()
			last_rtt_ms = _last_pong_ms - int(m.get_client_ms())
			pong.emit(last_rtt_ms, m.get_server_ms())

		Proto.MsgId.G2C_KICK:
			var k := Proto.Kick.new()
			if not _decode(k, payload):
				return
			Log.warn("net", "kicked", {"reason": k.get_reason(), "text": k.get_text()})
			if KLogin.session_ends(k.get_reason()):
				# the gateway closes the socket right after: remember why for the login screen
				last_notice = KLogin.result_text(k.get_reason(), k.get_text())
			elif state == "world" or state == "entering":
				_set_state("lobby")
			kicked.emit(k.get_reason(), k.get_text())

		_:
			Log.warn("net", "unhandled message", {"msg": msg_id})


func _summary_dict(c) -> Dictionary:
	return {"pid": c.get_player_id(), "name": c.get_name(), "level": c.get_level(), "series": c.get_series(),
		"sex": c.get_sex(), "faction": c.get_faction(), "zone_id": c.get_zone_id()}


func _apply_move(m) -> void:
	var mv := {"id": m.get_entity_id(), "x": m.get_pos().get_x(), "y": m.get_pos().get_y(),
		"tx": m.get_target().get_x(), "ty": m.get_target().get_y(), "speed": m.get_move_speed(),
		"tick": m.get_tick(), "seq": m.get_seq(), "path": _path_list(m.get_path())}
	var d = entities.get(int(mv.id))
	if d != null:
		d.x = mv.x
		d.y = mv.y
		d.tx = mv.tx
		d.ty = mv.ty
		d.speed = mv.speed
		d.path = mv.path
	entity_move.emit(mv)


func _magic_list(list: Array) -> Array:
	var out: Array = []
	for a in list:
		out.append({"type": int(a.get_type()), "value": Array(a.get_value())})
	return out


func _skill_dict(e) -> Dictionary:
	return {"id": int(e.get_skill_id()), "level": int(e.get_level()), "current_level": int(e.get_current_level()),
		"exp_percent": int(e.get_exp_percent()), "max_level": int(e.get_max_level()), "req_level": int(e.get_req_level()),
		"forbidden": e.get_forbidden(), "cool_down_left": int(e.get_cool_down_left()), "only_inc": e.get_only_inc()}


func _item_dict(v) -> Dictionary:
	return {"id": int(v.get_id()), "genre": int(v.get_genre()), "detail": int(v.get_detail()),
		"particular": int(v.get_particular()), "level": int(v.get_level()), "series": int(v.get_series()),
		"count": int(v.get_count()), "durability": int(v.get_durability()), "max_durability": int(v.get_max_durability()),
		"ex_type": int(v.get_ex_type()), "room": int(v.get_room()), "x": int(v.get_x()), "y": int(v.get_y()),
		"w": int(v.get_width()), "h": int(v.get_height()), "name": v.get_name(), "image": v.get_image(),
		"intro": v.get_intro(), "price": int(v.get_price()), "base": _magic_list(v.get_base()),
		"require": _magic_list(v.get_require()), "magic": _magic_list(v.get_magic()), "version": int(v.get_version()),
		"gen_param": int(v.get_gen_param())}


func _path_list(points: Array) -> Array:
	var out: Array = []
	for p in points:
		out.append([p.get_x(), p.get_y()])
	return out


func _entity_dict(e) -> Dictionary:
	return {"id": e.get_entity_id(), "type": e.get_entity_type(), "name": e.get_name(),
		"x": e.get_pos().get_x(), "y": e.get_pos().get_y(), "tx": e.get_target().get_x(), "ty": e.get_target().get_y(),
		"speed": e.get_move_speed(), "level": e.get_level(), "series": e.get_series(), "sex": e.get_sex(),
		"template_id": e.get_template_id(), "path": _path_list(e.get_path()), "dir": e.get_dir(),
		"life": e.get_life(), "life_max": e.get_life_max(), "doing": e.get_doing(), "doing_frames": e.get_doing_frames(),
		"count": e.get_count(), "riding": e.get_riding() if e.has_method("get_riding") else false,
		"gold_type": e.get_gold_type() if e.has_method("get_gold_type") else 0,
		"camp": e.get_camp() if e.has_method("get_camp") else 4, "current_camp": e.get_current_camp() if e.has_method("get_current_camp") else 4,
		"res": _res_dict(e), "pk_state": int(e.get_pk_state()) if e.has_method("get_pk_state") else 0,
		"menu_state": int(e.get_menu_state()) if e.has_method("get_menu_state") else 0,
		"menu_sentence": str(e.get_menu_sentence()) if e.has_method("get_menu_sentence") else "",
		"npc_kind": int(e.get_npc_kind()) if e.has_method("get_npc_kind") else 0}


# the equipment rows of the 0x4a / 0x4b player sync (KNpc+0x13f0 helm, +0x13f4 armour, +0x1400 weapon, +0x13fc horse,
# +0x13f8 mantle), keyed by the part group of the resource tables (0 head, 1 body, 2 weapon, 3 horse, 4 mantle); -1 = none
func _res_dict(e) -> Dictionary:
	if not e.has_method("get_helm_res"):
		return {}
	return {0: int(e.get_helm_res()), 1: int(e.get_armor_res()), 2: int(e.get_weapon_res()), 3: int(e.get_horse_res()),
		4: int(e.get_mantle_res())}
