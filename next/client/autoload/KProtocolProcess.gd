# Game - the client side of the protocol flow (docs/PROTOCOL.md section 3).
# UI scenes call the methods and listen to the signals; protobuf never leaks into scenes.
extends Node

const Proto := preload("res://proto/jx_pb.gd")
const KLogin := preload("res://net/KLogin.gd")
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


func chat(text: String) -> void:
	if state != "world" or text.strip_edges() == "":
		return
	var req := Proto.ChatReq.new()
	req.set_text(text)
	Net.send_msg(Proto.MsgId.C2G_CHAT, req)


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
			var a := {"id": m.get_entity_id(), "action": int(m.get_action()), "target": m.get_target(), "dir": m.get_dir(),
				"frames": m.get_frames(), "x": m.get_pos().get_x(), "y": m.get_pos().get_y(), "tick": m.get_tick()}
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

		Proto.MsgId.G2C_CHAT_MSG:
			var m := Proto.ChatMsg.new()
			if not _decode(m, payload):
				return
			chat_msg.emit({"id": m.get_entity_id(), "name": m.get_name(), "text": m.get_text()})

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
			d.room = int(m.get_room())
			d.x = int(m.get_x())
			d.y = int(m.get_y())
			Log.debug("item", "item moved", {"id": d.id, "room": d.room, "x": d.x, "y": d.y, "seq": m.get_seq()})
			item_changed.emit(d)

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

		Proto.MsgId.G2C_PLAYER_ATTRIB:
			var m := Proto.PlayerAttribSync.new()
			if not _decode(m, payload):
				return
			player_attrib = {
				"level": m.get_level(), "exp": m.get_exp(), "next_level_exp": m.get_next_level_exp(),
				"attribute_point": m.get_attribute_point(), "skill_point": m.get_skill_point(),
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
		"count": e.get_count()}
