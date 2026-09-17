# Game - the client side of the protocol flow (docs/PROTOCOL.md section 3).
# UI scenes call the methods and listen to the signals; protobuf never leaks into scenes.
extends Node

const Proto := preload("res://proto/jx_pb.gd")
const CLIENT_VERSION := "0.1.0"
const PING_INTERVAL := 5.0

signal login_result(ok: bool, text: String)
signal char_list(chars: Array)
signal char_created(ok: bool, result: int, summary: Dictionary)
signal entered_world(info: Dictionary)
signal enter_failed(result: int)
signal entity_spawn(entities: Array)
signal entity_despawn(ids: Array)
signal entity_move(mv: Dictionary)
signal entity_action(a: Dictionary)
signal entity_life(l: Dictionary)
signal chat_msg(msg: Dictionary)
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
var chars: Array = []
# entity_id -> Dictionary; the model of what the zone shows us.  Kept here (not in the scene) so
# packets that arrive before the world scene is loaded are not lost.
var entities := {}

var _account := ""
var _password := ""
var _move_seq := 0
var _ping_timer := 0.0
var _ping_sent_ms := 0


func _ready() -> void:
	Net.connected.connect(_on_connected)
	Net.disconnected.connect(_on_disconnected)
	Net.message.connect(_on_message)


func _set_state(s: String) -> void:
	Log.debug("session", "state", {"from": state, "to": s})
	state = s


# ---- requests ----------------------------------------------------------------------------

func login(server: String, account: String, password: String) -> void:
	var parts := server.split(":")
	host = parts[0] if parts.size() > 0 and parts[0] != "" else "127.0.0.1"
	port = int(parts[1]) if parts.size() > 1 else 17100
	_account = account
	_password = password
	_set_state("connecting")
	if Net.connect_to(host, port) != OK:
		_set_state("offline")
		login_result.emit(false, "Không kết nối được tới %s:%d" % [host, port])


func request_char_list() -> void:
	Net.send_msg(Proto.MsgId.C2G_CHAR_LIST, Proto.CharListReq.new())


func create_char(name: String, series: int, sex: int) -> void:
	var req := Proto.CharCreateReq.new()
	req.set_name(name)
	req.set_series(series)
	req.set_sex(sex)
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
	if was == "connecting" or was == "hello" or was == "auth":
		login_result.emit(false, "Mất kết nối: " + reason)
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
			Log.ctx["sid"] = sid
			Log.info("net", "hello ack", {"server_version": ack.get_server_version(), "protocol": ack.get_protocol_version()})
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
				Log.warn("auth", "login failed", {"result": res.get_result(), "text": res.get_text()})
				login_result.emit(false, "Đăng nhập thất bại: " + res.get_text())

		Proto.MsgId.G2C_CHAR_LIST_RES:
			var res := Proto.CharListRes.new()
			if not _decode(res, payload):
				return
			chars = []
			for c in res.get_chars():
				chars.append(_summary_dict(c))
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

		Proto.MsgId.G2C_PONG:
			var m := Proto.Pong.new()
			if not _decode(m, payload):
				return
			last_rtt_ms = Time.get_ticks_msec() - int(m.get_client_ms())
			pong.emit(last_rtt_ms, m.get_server_ms())

		Proto.MsgId.G2C_KICK:
			var k := Proto.Kick.new()
			if not _decode(k, payload):
				return
			Log.warn("net", "kicked", {"reason": k.get_reason(), "text": k.get_text()})
			if state == "world" or state == "entering":
				_set_state("lobby")
			kicked.emit(k.get_reason(), k.get_text())

		_:
			Log.warn("net", "unhandled message", {"msg": msg_id})


func _summary_dict(c) -> Dictionary:
	return {"pid": c.get_player_id(), "name": c.get_name(), "level": c.get_level(), "series": c.get_series(),
		"sex": c.get_sex(), "faction": c.get_faction(), "zone_id": c.get_zone_id()}


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
		"life": e.get_life(), "life_max": e.get_life_max(), "doing": e.get_doing(), "doing_frames": e.get_doing_frames()}
