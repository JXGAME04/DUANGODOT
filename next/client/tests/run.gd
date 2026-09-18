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
	test_skill_book_layout()
	test_part_math()
	test_skill_tree_layout()
	test_state_math()
	test_shortcuts()
	test_weapon_skill()
	test_shortcut_items()
	test_magic_desc()
	test_skill_desc()
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


# ---- the skill book (UiSkills.gd; gamecl.exe 2.0 KUiSkills, docs/CLIENT-2.0.md §6) --------------------------
func test_skill_book_layout() -> void:
	var UiSkills = load("res://ui/uicase/KUiSkillsLayout.gd")   # the numbers UiSkills.gd draws with
	# 0x00494AAF: the three branch buttons 103 px apart from [FightBtn] Left=8; the fourth slot is where the file
	# puts [CommonBtn] (Left=317)
	check(UiSkills.branch_button_x(8, 0) == 8 and UiSkills.branch_button_x(8, 1) == 111 and UiSkills.branch_button_x(8, 2) == 214, "branch buttons 103 px apart")
	check(UiSkills.branch_button_x(8, 3) == 317, "the fourth slot is the common button's")
	# 0x00493982: box index = slot * 10 + tier
	check(UiSkills.box_index(0, 0) == 0 and UiSkills.box_index(2, 9) == 29 and UiSkills.box_index(1, 3) == 13, "box index")
	# 0x00493A40: the common page fills column by column - the three slots of tier 0, then tier 1
	var order := []
	for n in 5:
		order.append(UiSkills.common_fill_index(n))
	check(order == [0, 10, 20, 1, 11], "common fill order %s" % [order])
	check(UiSkills.BRANCHES == 3 and UiSkills.TIER_COLS == 10 and UiSkills.SLOT_ROWS == 3, "three pages of 3 x 10")
	check(UiSkills.ROW_PITCH == 58 and UiSkills.ROW_PITCH_COMMON == 61 and UiSkills.COL_PITCH == 51, "row / column pitches")


# ---- the bars (KUiPartMath.gd; gamecl.exe 2.0 KSpriteImagePart 0x00450F00, Player_Exp 0x0044AD20) ----------
func test_part_math() -> void:
	var M = load("res://ui/KUiPartMath.gd")
	# PartType 0: from the left; 1: from the right; 2: from the top; 3: from the bottom (integer division)
	check(M.part_rect(0, 25, 100, 91, 10) == Rect2i(0, 0, 22, 10), "part type 0")
	check(M.part_rect(1, 25, 100, 91, 10) == Rect2i(69, 0, 22, 10), "part type 1")
	check(M.part_rect(2, 50, 100, 91, 10) == Rect2i(0, 0, 91, 5), "part type 2")
	check(M.part_rect(3, 50, 100, 91, 10) == Rect2i(0, 5, 91, 5), "part type 3")
	# full, empty, and a max of 0 (SetPart skips: the whole picture)
	check(M.part_rect(0, 100, 100, 91, 10) == Rect2i(0, 0, 91, 10) and M.part_rect(1, 150, 100, 91, 10) == Rect2i(0, 0, 91, 10), "full")
	check(M.part_rect(0, -1, 100, 91, 10).size == Vector2i(0, 0), "empty")
	check(M.part_rect(3, 5, 0, 91, 10) == Rect2i(0, 0, 91, 10), "max 0")
	# the way into the level in hundredths
	check(M.exp_percent(150, 100, 300) == 25 and M.exp_percent(100, 100, 300) == 0 and M.exp_percent(300, 100, 300) == 100, "exp percent")
	check(M.exp_percent(50, 100, 300) == 0 and M.exp_percent(5, 0, 0) == 100, "exp percent clamps")


# ---- the mouse-skill tree (KUiSkillTreeLayout.gd; gamecl.exe 2.0 KUiSkillTree 0x00495A40 / 0x00495B80) ----
func test_skill_tree_layout() -> void:
	var T = load("res://ui/KUiSkillTreeLayout.gd")
	# entry 0 (the current skill, group 0), then held skills with group = index / 8; seven per row at most, a new
	# group starts a new row: 0..6 | 7 | 8..14 | 15 (the walk of 0x00495A62)
	var groups := []
	for i in 17:
		@warning_ignore("integer_division")
		groups.append(i / 8)
	var p = T.place(groups, 7)
	check(p.rows == 5 and p.cols == 7, "rows %d cols %d" % [p.rows, p.cols])
	check(p.cells[0] == {"row": 0, "col": 0} and p.cells[6] == {"row": 0, "col": 6}, "first row")
	check(p.cells[7] == {"row": 1, "col": 0} and p.cells[8] == {"row": 2, "col": 0} and p.cells[15] == {"row": 3, "col": 0} and p.cells[16] == {"row": 4, "col": 0}, "row breaks")
	# the window grows upward from the anchor (LeftBtnPos = the lowest, leftmost button)
	var r = T.window_rect(Vector2i(760, 650), Vector2i(36, 36), 5, 7)
	check(r == Rect2i(760, 650 - 36 * 4, 36 * 7, 36 * 5), "window rect %s" % [r])
	check(T.cell_pos(p.cells[0], Vector2i(36, 36), 5) == Vector2i(0, 36 * 4) and T.cell_pos(p.cells[16], Vector2i(36, 36), 5) == Vector2i(0, 0), "cells from the bottom")
	check(T.hit(Vector2i(40, 36 * 4 + 5), Vector2i(36, 36), 5, p.cells) == 1 and T.hit(Vector2i(5, 5), Vector2i(36, 36), 5, p.cells) == 16 and T.hit(Vector2i(100, 5), Vector2i(36, 36), 5, p.cells) == -1, "hit test")
	# GDI 0x3f7 / 0x3f8: what each side lists (style, aura, LRSkill, side, ReqLevel, level)
	check(T.listed(0, false, 0, false) and T.listed(7, false, 0, false) and not T.listed(13, false, 0, false) and not T.listed(0, true, 0, false), "left list")
	check(T.listed(2, false, 0, true) and T.listed(14, false, 0, true) and not T.listed(7, false, 0, true) and not T.listed(3, true, 0, true), "right list")
	# LRSkill: 1 = the left only (even an aura), 2 = the right only, 3 = never; the level check only for styles 0..4 / 14
	check(T.listed(0, true, 1, false) and not T.listed(0, false, 1, true) and T.listed(0, false, 2, true) and not T.listed(0, false, 2, false), "LRSkill sides")
	check(not T.listed(0, false, 3, false) and not T.listed(0, false, 3, true) and T.listed(7, false, 3, false), "LRSkill 3 hidden, style 7 always")
	check(not T.listed(0, false, 0, false, 10, 1) and T.listed(0, false, 0, false, 10, 10) and T.listed(7, false, 0, false, 99, 1), "ReqLevel")


# ---- the skill state list (KUiStateMath.gd; gamecl.exe 2.0 KUiSkillState 0x0041F460 / 0x0041D720) ----------
func test_state_math() -> void:
	var M = load("res://ui/KUiStateMath.gd")
	check(M.time_text(0) == "N/A" and M.time_text(-5) == "N/A", "no time")
	check(M.time_text(59) == "59s" and M.time_text(60) == "1m" and M.time_text(3599) == "59m", "seconds and minutes")
	check(M.time_text(3600) == "1h" and M.time_text(356399) == "98h" and M.time_text(356400) == "4d", "hours and days")
	check(M.time_text(3725, true) == "01h:02m:05s", "long form")
	check(M.slot_pos(3, false) == Vector2i(72, 0) and M.slot_pos(9, true) == Vector2i(216, 36), "slots 24 px apart, debuffs at y 36")
	check(M.seconds_left(18) == 1 and M.seconds_left(19) == 2 and M.seconds_left(-1) == -1 and M.seconds_left(0) == 0, "frames to seconds")


# ---- the shortcut skills (KUiShortcut.gd; gamecl.exe 2.0 ShortcutSkill 0x00495F70, autoexec.lua Q W E A S D Z X C) ----
func test_shortcuts() -> void:
	var K = load("res://ui/KUiShortcut.gd")
	var sc = K.new()
	check(sc.slots.size() == 9 and int(sc.slot(0).id) == 0, "nine empty slots")
	check(sc.assign(0, 14, false) and int(sc.slot(0).id) == 14 and sc.slot_of(14, false) == 0, "slot 0 takes 14")
	# the same skill on the same side moves: the old slot is cleared (0x00495FEA); another side may keep it
	check(sc.assign(3, 14, false) and int(sc.slot(0).id) == 0 and sc.slot_of(14, false) == 3, "14 moves from Q to A")
	check(sc.assign(5, 14, true) and sc.slot_of(14, false) == 3 and sc.slot_of(14, true) == 5, "the right side is another slot")
	check(not sc.assign(9, 1, false) and not sc.assign(0, 0, false), "out of range / empty refused")
	check(K.key_of(0) == "Q" and K.key_of(8) == "C" and K.key_of(9) == "", "key names")
	check(K.slot_of_key(KEY_Q) == 0 and K.slot_of_key(KEY_C) == 8 and K.slot_of_key(KEY_F1) == -1, "keys to slots")
	var again = K.new()
	again.from_json(sc.to_json())
	check(again.slot_of(14, false) == 3 and again.slot_of(14, true) == 5, "round trip")


# ---- the weapon -> plain attack table (KWeaponSkillTable.gd; gamecl.exe 2.0 0x005CB396 / 0x005EBBA0) -----------
func test_weapon_skill() -> void:
	var K = load("res://ui/KWeaponSkillTable.gd")
	var t = K.parse({"rows": [{"detail": -1, "particular": 0, "skill": 53}, {"detail": 0, "particular": 2, "skill": 1},
		{"detail": 0, "particular": 200, "skill": 1}, {"detail": 1, "particular": 0, "skill": 2}, {"detail": 0, "particular": 3, "skill": 0}]})
	check(int(t.bare) == 53 and t.melee.size() == 1 and t.ranged.size() == 1, "rows kept: bare, melee 2, ranged 0 (particular 200 and skill 0 dropped)")
	check(K.skill_of(t, -1, -1) == 53 and K.skill_of(t, 0, 2) == 1 and K.skill_of(t, 1, 0) == 2, "lookups")
	check(K.skill_of(t, 0, 5) == 0 and K.skill_of(t, 2, 0) == 0 and K.skill_of({}, -1, 0) == 0, "no row / no table -> 0")
	check(K.parse(null).bare == 0, "no json")


# ---- the quick slots (KUiShortcutItem.gd; gamecl.exe 2.0 ShortcutUseItem 0x00472F80, Exchange kind 7, op 0xe) -----
func test_shortcut_items() -> void:
	var K = load("res://ui/KUiShortcutItem.gd")
	var q = K.new()
	var potion := {"id": 11, "genre": 1, "detail": 0, "particular": 0, "room": 0, "count": 3}
	var potion2 := {"id": 12, "genre": 1, "detail": 0, "particular": 0, "room": 0, "count": 5}
	var mana := {"id": 13, "genre": 1, "detail": 1, "particular": 0, "room": 0, "count": 2}
	check(q.slots.size() == 9 and int(q.slot(0).genre) == 0, "nine empty cells")
	check(q.put_item(0, potion) and q.slot_of(K.GENRE_ITEM, 11) == 0, "a potion in cell 0")
	check(not q.put_item(3, potion2) and int(q.slot(3).genre) == 0, "another stack of the same kind is refused (0x006386C0)")
	check(q.put_item(0, potion2) and int(q.slot(0).id) == 12, "the same kind may replace its own cell")
	check(q.put_item(1, mana) and q.has_kind(1, 1, 0), "another kind takes cell 1")
	check(q.put_skill(2, 14) and q.put_skill(4, 14) and int(q.slot(2).genre) == 0 and q.slot_of(K.GENRE_SKILL, 14) == 4, "a skill moves between cells")
	check(not q.put_item(9, potion) and not q.put_skill(0, 0), "out of range / empty refused")
	# op 0xe: the stack in cell 0 is gone -> the other stack of that kind takes it; the mana potion vanished -> cleared
	var items := {11: potion}
	check(q.resolve(items, 0) and int(q.slot(0).id) == 11 and int(q.slot(1).genre) == 0 and int(q.slot(4).id) == 14, "resolve rebinds by kind, clears the rest")
	check(not q.resolve(items, 0), "nothing to change the second time")
	var moved := {11: {"id": 11, "genre": 1, "detail": 0, "particular": 0, "room": 10, "count": 3}}
	check(q.resolve(moved, 0) and int(q.slot(0).genre) == 0, "an item out of the bag leaves the cell")
	check(K.slot_of_key(KEY_1) == 0 and K.slot_of_key(KEY_9) == 8 and K.slot_of_key(KEY_KP_5) == 4 and K.slot_of_key(KEY_0) == -1, "keys 1..9")
	q.put_item(5, potion)
	var again = K.new()
	again.from_json(q.to_json())
	check(int(again.slot(5).id) == 11 and int(again.slot(5).detail) == 0 and int(again.slot(4).id) == 14, "round trip")
	q.remove(5)
	check(int(q.slot(5).genre) == 0, "remove")


# ---- the "#" grammar of magicdesc.ini (KMagicDesc.gd; KMagicDesc::GetDesc, gamecl.exe 2.0 0x0060A2B0 / 0x00608C00) ----
func test_magic_desc() -> void:
	var K = load("res://ui/KMagicDesc.gd")
	var ctx := {"series": ["Kim", "Mộc", "Thuỷ", "Hoả", "Thổ"], "series_none": "Vô hệ", "cost_types": ["Nội lực", "Sinh lực", "Thể lực", "Tiền"],
		"sex": ["Nam", "Nữ"], "factions": {0: "Thiếu Lâm"}, "skill_name": func(id: int) -> String: return "Kỹ năng %d" % id, "own_skill": "Võ công vốn có"}
	check(K.describe_line("Sát thương vật lý: #d1- đến #d3- điểm", [12, 0, 30], ctx) == "Sát thương vật lý: 12 đến 30 điểm", "d1 / d3")
	check(K.describe_line("Nộ trảnh: #d1+%", [15, 0, 0], ctx) == "Nộ trảnh: +15%", "the + sign")
	check(K.describe_line("Thời gian: #d1~%", [20, 0, 0], ctx) == "Thời gian: -20%", "the ~ sign on a positive")
	check(K.describe_line("x #d1~", [-5, 0, 0], ctx) == "x +5", "the ~ sign negates a negative")
	check(K.describe_line("#d4-/#d7-", [0x0102, 0, 0], ctx) == "1/2", "digits 4..6 shift, 7..9 mask")
	check(K.describe_line("#f6-s", [0, 0, 36 * 256], ctx) == "2.00s", "f6: frames to seconds")
	check(K.describe_line("Loại: #k1-", [2, 0, 0], ctx) == "Loại: Thể lực" and K.describe_line("#s1-", [3, 0, 0], ctx) == "Hoả" and K.describe_line("#s1-", [9, 0, 0], ctx) == "Vô hệ", "k and s")
	check(K.describe_line("#m1- #x1-", [0, 0, 0], ctx) == "Thiếu Lâm Nam" and K.describe_line("#x1-", [1, 0, 0], ctx) == "Nữ", "m and x")
	# a marker is always four characters: the "]" after "#l1" is its sign slot (KMagicDesc.cpp pTempDesc += 4), as in the game
	check(K.describe_line("Tăng [#l1] h#d3-%", [14, 0, 7], ctx) == "Tăng [[ Kỹ năng 14 ] h7%" and K.describe_line("[#l1]", [0, 0, 0], ctx) == "[Võ công vốn có", "l: a skill name or the own words")
	check(K.describe_line("không có dấu", [1, 2, 3], ctx) == "không có dấu" and K.describe_line("#d1", [1, 0, 0], ctx) == "#d1", "plain text; a marker needs four characters")


# ---- the skill tip (KUiSkillDesc.gd; gamecl.exe 2.0 KSkill::GetDesc 0x006FBC90) ----------------------------------
func test_skill_desc() -> void:
	var D = load("res://ui/KUiSkillDesc.gd")
	var text := {"strings": {"G_SkillList_6": "Đẳng cấp yêu cầu: %d", "G_Skills_37": "Cấp hiện tại: %d", "G_Skills_45": "Tiêu hao nội lực: %d\n",
		"G_Skills_48": "Khoảng cách hiệu quả: %d\n", "G_Skills_44": "\n<color=Red> Đẳng cấp tiếp theo \n", "G_Skills_41": "Hạn chế vũ khí:",
		"G_Skills_42": "Trong lúc cưỡi ngựa không thể thi triển \n", "G_Skills_35": " (Công kích gần) \n", "G_Skills_49": "Tăng cho kỹ năng %s: %d%%\n"},
		"descript": {"physicsdamage_v": "Sát thương vật lý: #d1- đến #d3- điểm", "attackrating_p": "Độ chính xác: #d1-%"},
		"skill_attrib": {"202": "Võ công lưu phái: <color=Cyan>Quyền pháp<color>"}, "weapon_limit": {"f1": "<color=White>Tay không<color>", "2": "Côn"}}
	var row := {"SkillName": "Hàng Long Bất Vũ", "SkillDesc": "Võ công nhập môn", "ReqLevel": "10", "Attrib": "202", "IsAura": "0", "Series": "0",
		"EqtLimit": "-1", "HorseLimit": "1", "IsMelee": "0", "IsExpSkill": "0"}
	var desc := {"skill_id": 14, "max_level": 20, "has_cur": true, "has_next": true,
		"cur": {"level": 1, "cost": 10, "cost_type": 0, "attack_radius": 90, "attribs": [{"group": 1, "name": "physicsdamage_v", "v0": 12, "v1": 0, "v2": 30}, {"group": 0, "name": "attackrating_p", "v0": 5, "v1": 0, "v2": 0}], "appends": [{"skill_id": 10, "value": 7}]},
		"next": {"level": 2, "cost": 11, "cost_type": 0, "attack_radius": 90, "attribs": [{"group": 1, "name": "physicsdamage_v", "v0": 14, "v1": 0, "v2": 33}], "appends": []}}
	var name_of := func(id: int) -> String: return "Kim Cang Phục Ma" if id == 10 else str(id)
	var t: String = D.build(row, {"level": 1, "exp_percent": 0}, 20, desc, text, name_of)
	var lines := t.split("\n")
	check(lines[0] == "<color=Yellow>Hàng Long Bất Vũ" and lines[1] == "<bclr=Black><color>", "the title in yellow, the black outline back: %s" % [lines.slice(0, 2)])
	check(lines[2] == "Võ công nhập môn" and lines[3] == "", "the description after a blank")
	check(lines[4] == "Võ công lưu phái: <color=Cyan>Quyền pháp<color>" and lines[5] == "Cấp hiện tại: 1", "the SkillAttrib line and the level")
	check(t.find("Tiêu hao nội lực: 10\n") >= 0 and t.find("Khoảng cách hiệu quả: 90\n") >= 0, "cost and range")
	# the immediate attribute (group 0) is printed before the damage one (group 1)
	check(t.find("Độ chính xác: 5%") < t.find("Sát thương vật lý: 12 đến 30 điểm") and t.find("Độ chính xác: 5%") >= 0, "attribute order and texts")
	check(t.find("Tăng cho kỹ năng Kim Cang Phục Ma: 7%") >= 0, "the append line")
	check(t.find("Hạn chế vũ khí:<color=White>Tay không<color>\n") >= 0 and t.find("Trong lúc cưỡi ngựa") >= 0, "weapon and horse limits")
	var next_at := t.find("<color=Red> Đẳng cấp tiếp theo")
	check(next_at >= 0 and t.find("Sát thương vật lý: 14 đến 33 điểm") > next_at, "the next level after its red header")
	check(t.find("Đẳng cấp yêu cầu") < 0, "no requirement line at level 20")
	# a low character sees the requirement; an aura shows no level line and no next level; no answer yet -> the static part
	var low: String = D.build(row, {"level": 1}, 5, desc, text, name_of)
	check(low.find("Đẳng cấp yêu cầu: 10") >= 0, "the requirement line at level 5")
	var aura_row := row.duplicate()
	aura_row["IsAura"] = "1"
	var au: String = D.build(aura_row, {"level": 1}, 20, desc, text, name_of)
	check(au.find("Cấp hiện tại") < 0 and au.find("Đẳng cấp tiếp theo") < 0 and au.find("Tiêu hao nội lực: 10") >= 0, "an aura: no level, no next, its cost")
	var pending: String = D.build(row, {"level": 1}, 20, {}, text, name_of)
	check(pending.find("Tiêu hao") < 0 and pending.find("Cấp hiện tại: 1") >= 0, "without the zone's answer: the static lines only")
	check(D.build(row, {"level": 1}, 20, desc, text, name_of).find("(Công kích gần)") < 0 and D.build({"SkillName": "Đấm", "Attrib": "1", "IsMelee": "1"}, {"level": 1}, 20, {}, text, name_of).find("(Công kích gần)") >= 0, "the plain attack words for Attrib 1 / 2 only")
