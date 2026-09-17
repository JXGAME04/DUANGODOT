# Headless unit tests for the client (no editor, no window):
#   godot --headless --path client -s tests/run.gd
# Exit code 0 when every check passes.  Keep the checks aligned with the C++/Go contracts.
extends SceneTree

const Proto := preload("res://proto/jx_pb.gd")
const NetScript := preload("res://net/KProtocol.gd")   # pure framing (autoload scripts cannot be preloaded here)
const KLogin := preload("res://net/KLogin.gd")
const NetAddress := preload("res://net/KNetAddress.gd")
const LogScript := preload("res://autoload/KDebug.gd")
const SceneMath := preload("res://scenes/KSceneMath.gd")
const IpoTree := preload("res://scenes/KIpoTree.gd")
const IpotLeaf := preload("res://scenes/KIpotLeaf.gd")
const KMath := preload("res://scenes/KMath.gd")
const NpcResNode := preload("res://scenes/KNpcResNode.gd")

var _failed := 0
var _passed := 0


func check(cond: bool, what: String) -> void:
	if cond:
		_passed += 1
	else:
		_failed += 1
		printerr("FAIL: " + what)


func _init() -> void:
	test_frame_vectors()
	test_frame_split()
	test_frame_fuzz()
	test_log_format()
	test_proto_round_trip()
	test_login_text()
	test_net_address()
	test_scene_math()
	test_ipot_order()
	test_kmath_direction()
	test_npcres_tables()
	print("client tests: %d passed, %d failed" % [_passed, _failed])
	quit(0 if _failed == 0 else 1)


# ---- characters (KMath.h, KNpcResNode / KNpcRes) --------------------------------------------

func test_kmath_direction() -> void:
	check(KMath.get_dir_index(0, 0, 0, 100) == 0, "down is 0")
	check(KMath.get_dir_index(0, 0, -100, 0) == 16, "left is 16: %d" % KMath.get_dir_index(0, 0, -100, 0))
	check(KMath.get_dir_index(0, 0, 0, -100) == 31, "up is 31 (x equal): %d" % KMath.get_dir_index(0, 0, 0, -100))
	check(KMath.get_dir_index(0, 0, 100, 0) == 47, "right is 47: %d" % KMath.get_dir_index(0, 0, 100, 0))
	check(KMath.get_dir_index(0, 0, -100, 100) == 8, "down-left is 8")
	check(KMath.get_dir_index(0, 0, 100, -100) == 39, "up-right is 39")
	check(KMath.get_dir_index(5, 5, 5, 5) == -1, "no movement")
	check(KMath.dir64_to_sprite(0, 8) == 0 and KMath.dir64_to_sprite(16, 8) == 2 and KMath.dir64_to_sprite(31, 8) == 4
		and KMath.dir64_to_sprite(63, 8) == 0 and KMath.dir64_to_sprite(47, 8) == 6, "64 directions to 8 sprite directions")
	check(KMath.dir64_to_sprite(20, 1) == 0, "single direction sprite")


func test_npcres_tables() -> void:
	var def: Array = []
	for i in 16:
		def.append([i, 1, 0])
	var res := {"special": true, "no_horse": [[4, 3, 2, 12, 13], [20, 21, 22, 23, 24]],
		"sort": {"default": def, "acts": {"3": {"use_default": true, "lines": [[7, 9, 8]]},
			"4": {"use_default": false, "dirs": [[40], [41], [42], [43], [44], [45], [46], [47], [48], [49], [50], [51], [52], [53], [54], [55]], "lines": [[2, 60]]}}}}
	check(NpcResNode.act_no(res, 1, 0, false) == 3 and NpcResNode.act_no(res, 4, 1, false) == 24, "doing -> action through the weapon row")
	check(NpcResNode.act_no(res, 9, 0, false) == -1 and NpcResNode.act_no(res, 1, 5, false) == -1, "out of table")
	check(NpcResNode.act_no({"special": false}, 6, 0, false) == 6, "normal npc uses the doing")
	check(NpcResNode.sort_order(res, 3, 0, 7) == [9, 8], "frame line of an action")
	check(NpcResNode.sort_order(res, 3, 2, 1) == [2, 1, 0], "default row when the frame has no line")
	check(NpcResNode.sort_order(res, 4, 1, 0) == [41], "own direction rows")
	check(NpcResNode.sort_order(res, 4, 1, 2) == [60], "frame line wins over own rows")
	check(NpcResNode.sort_order(res, 9, 5, 0) == [5, 1, 0], "action without a block")
	check(NpcResNode.sort_order({"special": false}, 1, 0, 0) == [5], "normal npc single image slot")
	# KNpcRes::Draw frame arithmetic: 48 frames in 8 directions, half way through a 28 frame stand
	check(NpcResNode.frame_no(0, 28, 14, 48, 8) == 3, "frame 3 of direction 0: %d" % NpcResNode.frame_no(0, 28, 14, 48, 8))
	check(NpcResNode.frame_no(16, 28, 0, 48, 8) == 12, "first frame of direction 2 (left)")
	check(NpcResNode.frame_no(63, 10, 9, 48, 8) == 5, "last frame of direction 0")
	check(NpcResNode.frame_no(0, 10, 5, 120, 8) == 7, "120 frames: 15 per direction")
	check(NpcResNode.ref_spot(320, 0, 0) == Vector2(160, 192) and NpcResNode.ref_spot(320, 160, 222) == Vector2(160, 222)
		and NpcResNode.ref_spot(100, 0, 0) == Vector2.ZERO, "reference spot rules")
	check(NpcResNode.player_res_name(0) == "MainMan" and NpcResNode.player_res_name(1) == "MainLady", "player resource names")


# ---- object sorting (SceneMath.cpp / KIpotBranch.cpp) ---------------------------------------

func test_scene_math() -> void:
	var a := Vector2i(0, 0)
	var b := Vector2i(100, 0)
	check(SceneMath.relation_point_line(Vector2i(50, -10), a, b) == SceneMath.UP, "point above the line is UP")
	check(SceneMath.relation_point_line(Vector2i(50, 10), a, b) == SceneMath.DOWN, "point below the line is DOWN")
	check(SceneMath.relation_point_line(Vector2i(50, 0), a, b) == SceneMath.ON, "point on the line")
	check(SceneMath.relation_point_line(Vector2i(101, 2), a, b) == SceneMath.ON, "end point tolerance")
	check(SceneMath.distance_point_line(Vector2i(0, 10), a, b) == 10, "distance point-line")
	var cut := SceneMath.relation_line_line_check_cut(Vector2i(0, 10), Vector2i(100, -10), a, b)
	check(cut.rel == SceneMath.CROSS and cut.poi == Vector2i(50, 0), "crossing lines: " + str(cut))
	check(SceneMath.relation_line_line_check_cut(Vector2i(0, -10), Vector2i(100, -5), a, b).rel == SceneMath.UP, "line above")
	check(SceneMath.is_line_linkable(0.5, 100.0, 0.505, 110.0) and not SceneMath.is_line_linkable(0.5, 100.0, 0.52, 100.0), "linkable lines")


func _leaf(sort: int, p1: Vector2i, p2: Vector2i, name: String, runtime: bool = false):
	var l = IpotLeaf.new()
	l.type = IpotLeaf.Type.RUNTIME if runtime else IpotLeaf.Type.BUILDIN
	l.sort = sort
	l.line_start = p1
	l.line_end = p2
	l.item = name
	l.reset()
	return l


func _order(tree) -> Array:
	var out: Array = []
	tree.paint(out)
	var names: Array = []
	for l in out:
		names.append(l.item)
	return names


func test_ipot_order() -> void:
	var tree = IpoTree.new()
	tree.set_permanent_branch_pos(-2000, 2000, -1000)
	var wall = _leaf(IpotLeaf.Sort.LINE, Vector2i(0, 100), Vector2i(200, 100), "wall")
	var fence = _leaf(IpotLeaf.Sort.LINE, Vector2i(0, 300), Vector2i(200, 300), "fence")
	tree.add_leaf_line(fence)
	tree.add_leaf_line(wall)
	for l in [_leaf(IpotLeaf.Sort.POINT, Vector2i(100, 400), Vector2i.ZERO, "front"),
			_leaf(IpotLeaf.Sort.POINT, Vector2i(100, 50), Vector2i.ZERO, "behind"),
			_leaf(IpotLeaf.Sort.POINT, Vector2i(100, 200), Vector2i.ZERO, "between")]:
		tree.add_leaf_point(l)
	check(_order(tree) == ["behind", "wall", "between", "fence", "front"], "point/line order: " + str(_order(tree)))
	# a character walking from behind the wall to in front of it
	var hero = _leaf(IpotLeaf.Sort.POINT, Vector2i(100, 50), Vector2i.ZERO, "hero", true)
	tree.add_leaf_point(hero)
	var o := _order(tree)
	check(o.find("hero") < o.find("wall"), "hero behind the wall: " + str(o))
	tree.pluck_rto(hero)
	hero.position = Vector2i(100, 150)
	tree.add_leaf_point(hero)
	o = _order(tree)
	check(o.find("hero") > o.find("wall") and o.find("hero") < o.find("fence") and o.count("hero") == 1, "hero in front of the wall: " + str(o))
	tree.pluck_rto(hero)
	check(not _order(tree).has("hero"), "plucked hero is gone")
	# tree objects: the branch line splits the scene, a crossing object is cut in two parts
	var t = IpoTree.new()
	var road = _leaf(IpotLeaf.Sort.TREE, Vector2i(0, 500), Vector2i(1000, 500), "road")
	road.nodical = 500.0
	var cross = _leaf(IpotLeaf.Sort.TREE, Vector2i(300, 400), Vector2i(700, 600), "cross")
	cross.angle = 0.5
	cross.nodical = 250.0
	t.add_branch(road)
	t.add_branch(cross)
	t.add_leaf_point(_leaf(IpotLeaf.Sort.POINT, Vector2i(100, 600), Vector2i.ZERO, "near"))
	t.add_leaf_point(_leaf(IpotLeaf.Sort.POINT, Vector2i(100, 400), Vector2i.ZERO, "far"))
	var o2 := _order(t)
	check(o2.find("far") < o2.find("road") and o2.find("road") < o2.find("near"), "branch order: " + str(o2))
	check(o2.count("cross") == 2 and cross.img_part and cross.end_pos == Vector2i(500, 500), "crossing tree object cut at (500,500): " + str(cross.end_pos))
	check(o2.find("cross") < o2.find("road") and o2.rfind("cross") > o2.find("road"), "one part behind the road, one in front: " + str(o2))
	t.fell()
	check(_order(t).is_empty(), "felled tree is empty")


func test_frame_vectors() -> void:
	var f := NetScript.encode(1001, "hi".to_utf8_buffer())
	check(f == PackedByteArray([0x06, 0x00, 0x00, 0x00, 0xE9, 0x03, 0x00, 0x00, 0x68, 0x69]), "frame vector 1: " + str(f))
	var e := NetScript.encode(0x1234, PackedByteArray(), 3)
	check(e == PackedByteArray([0x04, 0x00, 0x00, 0x00, 0x34, 0x12, 0x03, 0x00]), "frame vector 2: " + str(e))


func test_frame_split() -> void:
	var stream := PackedByteArray()
	stream.append_array(NetScript.encode(1, "one".to_utf8_buffer()))
	stream.append_array(NetScript.encode(2, PackedByteArray()))
	stream.append_array(NetScript.encode(3, "three".to_utf8_buffer(), 1))
	for chunk in range(1, stream.size() + 1):
		var buf := PackedByteArray()
		var got := []
		var pos := 0
		while pos < stream.size():
			var end: int = min(pos + chunk, stream.size())
			buf.append_array(stream.slice(pos, end))
			pos = end
			var parsed := NetScript.parse(buf)
			buf = parsed["rest"]
			check(parsed["error"] == "", "no error at chunk %d" % chunk)
			for fr in parsed["frames"]:
				got.append(fr)
		var ok: bool = got.size() == 3 and got[0][0] == 1 and got[0][2].get_string_from_utf8() == "one" \
			and got[1][0] == 2 and got[1][2].size() == 0 and got[2][0] == 3 and got[2][1] == 1 \
			and got[2][2].get_string_from_utf8() == "three" and buf.size() == 0
		check(ok, "split at chunk %d" % chunk)
	var bad := NetScript.parse(PackedByteArray([0x02, 0x00, 0x00, 0x00, 0x00, 0x00]))
	check(bad["error"] == "corrupt", "corrupt frame detected")
	var big := NetScript.parse(NetScript.encode(7, "xxxxxxxxxxxxxxxxx".to_utf8_buffer()), 16)
	check(big["error"] == "too_large", "oversized frame detected")
	var partial := NetScript.parse(PackedByteArray([0x06, 0x00, 0x00]))
	check(partial["frames"].size() == 0 and partial["rest"].size() == 3 and partial["error"] == "", "partial frame waits")


# Arbitrary bytes from the network may only give frames, an empty result or an error - never a
# crash and never a payload over the limit (C++: test_frame.cpp [fuzz], Go: frame_fuzz_test.go).
func test_frame_fuzz() -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 20260917
	var limit := 4096
	var failures := 0
	for round in 300:
		var data := PackedByteArray()
		if round % 2 == 0:
			data.append_array(NetScript.encode(rng.randi_range(0, 9000), "hello".to_utf8_buffer()))
		for i in rng.randi_range(0, 400):
			data.append(rng.randi_range(0, 255))
		var buf := PackedByteArray()
		var chunk := rng.randi_range(1, 17)
		var pos := 0
		while pos < data.size():
			var end: int = min(pos + chunk, data.size())
			buf.append_array(data.slice(pos, end))
			pos = end
			var parsed := NetScript.parse(buf, limit)
			buf = parsed["rest"]
			for fr in parsed["frames"]:
				if fr[2].size() > limit:
					failures += 1
			if parsed["error"] != "":
				if parsed["error"] != "corrupt" and parsed["error"] != "too_large":
					failures += 1
				break   # a broken stream closes the connection
	check(failures == 0, "garbage bytes never produce a bad frame (%d failures)" % failures)


func test_log_format() -> void:
	var lg = LogScript.new()
	lg.process = "test"
	var line: String = lg.format_line(LogScript.Level.INFO, "net", "connected", {"addr": "127.0.0.1", "port": 15622}, {"sid": 42, "pid": 7, "zone": 0, "tick": 99})
	var parsed = JSON.parse_string(line)
	check(parsed != null and parsed["lvl"] == "info" and parsed["cat"] == "net" and parsed["proc"] == "test" and parsed["msg"] == "connected", "log fields: " + line)
	check(parsed != null and parsed["addr"] == "127.0.0.1" and parsed["port"] == "15622", "log extra fields are strings")
	check(parsed != null and int(parsed["sid"]) == 42 and int(parsed["pid"]) == 7 and int(parsed["tick"]) == 99 and not parsed.has("zone"), "log context")
	check(line.begins_with('{"ts":') and line.find('"lvl"') < line.find('"cat"') and line.find('"cat"') < line.find('"msg"'), "log field order")
	var ts: String = parsed["ts"] if parsed != null else ""
	check(ts.length() == 27 and ts[10] == "T" and ts.ends_with("Z"), "timestamp format: " + ts)
	check(LogScript.level_from_name("warning") == LogScript.Level.WARN and LogScript.level_from_name("nope") == LogScript.Level.INFO, "level names")
	lg.set_levels("net=trace, zone.tick=debug, =warn")
	check(lg.effective_level("net.recv") == LogScript.Level.TRACE and lg.effective_level("zone.tick.ai") == LogScript.Level.DEBUG and lg.effective_level("zone") == LogScript.Level.WARN, "level prefix rules")
	lg.free()


# ---- gateway address (KNetAddress.gd: tcp / tls / ws / wss) ----------------------------------

func test_net_address() -> void:
	var a := NetAddress.parse("127.0.0.1:17100")
	check(a["scheme"] == "tcp" and a["host"] == "127.0.0.1" and a["port"] == 17100, "plain host:port is tcp")
	a = NetAddress.parse("  example.com  ")
	check(a["scheme"] == "tcp" and a["host"] == "example.com" and a["port"] == 17100, "default port")
	a = NetAddress.parse("tls://game.example.com:17101")
	check(a["scheme"] == "tls" and a["port"] == 17101 and NetAddress.is_tls(a) and not NetAddress.is_websocket(a), "tls url")
	a = NetAddress.parse("ws://127.0.0.1:17102/ws")
	check(a["scheme"] == "ws" and a["path"] == "/ws" and NetAddress.is_websocket(a) and not NetAddress.is_tls(a), "ws url")
	check(NetAddress.url(a) == "ws://127.0.0.1:17102/ws", "url round trip: %s" % NetAddress.url(a))
	a = NetAddress.parse("WSS://Game.Example.com:443/socket")
	check(a["scheme"] == "wss" and a["path"] == "/socket" and NetAddress.is_websocket(a) and NetAddress.is_tls(a), "wss url, scheme is case-insensitive")
	a = NetAddress.parse("ws://host/")
	check(a["port"] == 17100 and a["path"] == "/", "path without a port")
	a = NetAddress.parse("[::1]:17100")
	check(a["host"] == "[::1]" and a["port"] == 17100, "ipv6 in brackets: %s" % a["host"])
	a = NetAddress.parse("host:abc")
	check(a["host"] == "host" and a["port"] == 17100, "a bad port falls back to the default")
	a = NetAddress.parse("")
	check(a["host"] == "127.0.0.1" and a["port"] == 17100, "empty address")


# ---- login results (KLogin.gd, old LOGIN_R_* -> CI_MI_* messages) ----------------------------

func test_login_text() -> void:
	check(KLogin.result_text(Proto.Result.OK) == "", "OK has no message")
	for r in [Proto.Result.UNAUTHORIZED, Proto.Result.ACCOUNT_IN_USE, Proto.Result.ACCOUNT_FROZEN, Proto.Result.NO_GAME_TIME,
			Proto.Result.SERVER_BUSY, Proto.Result.VERSION_MISMATCH, Proto.Result.ZONE_UNAVAILABLE, Proto.Result.RATE_LIMITED,
			Proto.Result.TIMEOUT, Proto.Result.REPLACED, Proto.Result.SERVER_SHUTDOWN]:
		check(KLogin.result_text(r) != "" and not KLogin.result_text(r).begins_with("Lỗi"), "result %d has its own message" % r)
	check(KLogin.result_text(Proto.Result.ACCOUNT_FROZEN, "gian lận").ends_with("(gian lận)"), "frozen text carries the reason")
	check(KLogin.result_text(999) == "Lỗi 999", "unknown result: %s" % KLogin.result_text(999))
	check(KLogin.session_ends(Proto.Result.REPLACED) and KLogin.session_ends(Proto.Result.SERVER_SHUTDOWN)
		and KLogin.session_ends(Proto.Result.RATE_LIMITED), "kicks that close the socket end the session")
	check(not KLogin.session_ends(Proto.Result.ZONE_UNAVAILABLE), "a zone outage keeps the session")
	# the proto enum carries the new values (godobuf strips the RESULT_ prefix)
	check(Proto.Result.ACCOUNT_IN_USE == 11 and Proto.Result.SERVER_SHUTDOWN == 18, "Result codes match common.proto")


func test_proto_round_trip() -> void:
	var hello := Proto.Hello.new()
	hello.set_protocol_version(1)
	hello.set_client_version("godot-test")
	hello.set_platform("test")
	var bytes: PackedByteArray = hello.to_bytes()
	var back := Proto.Hello.new()
	var err = back.from_bytes(bytes)
	check(err == Proto.PB_ERR.NO_ERRORS, "Hello from_bytes error %s" % str(err))
	check(back.get_protocol_version() == 1 and back.get_client_version() == "godot-test" and back.get_platform() == "test", "Hello round trip")
	check(Proto.MsgId.C2G_HELLO == 1001 and Proto.MsgId.G2C_ENTITY_MOVE == 2103 and Proto.Protocol.VERSION == 1 and Proto.Result.OK == 0 and Proto.Result.ZONE_UNAVAILABLE == 9, "enum values")

	var spawn := Proto.EntitySpawn.new()
	var e = spawn.add_entities()
	e.set_entity_id(77)
	e.set_name("Đại Hiệp")
	e.set_entity_type(Proto.EntityType.ENTITY_PLAYER)
	e.new_pos().set_x(-5)
	e.get_pos().set_y(4096)
	var spawn2 := Proto.EntitySpawn.new()
	check(spawn2.from_bytes(spawn.to_bytes()) == Proto.PB_ERR.NO_ERRORS, "EntitySpawn from_bytes")
	var ents: Array = spawn2.get_entities()
	check(ents.size() == 1 and ents[0].get_entity_id() == 77 and ents[0].get_name() == "Đại Hiệp" and ents[0].get_pos().get_x() == -5 and ents[0].get_pos().get_y() == 4096, "EntitySpawn round trip (UTF-8, negative int32)")
