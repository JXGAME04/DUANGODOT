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
const SprControl := preload("res://scenes/KSprControl.gd")
const StateSpr := preload("res://scenes/KStateSpr.gd")
const WavSound := preload("res://scenes/KWavSound.gd")
const NpcGold := preload("res://scenes/KNpcGold.gd")

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
	test_missle_math()
	test_knock_back()
	test_state_pictures()
	test_wav_sound()
	test_gold_name()
	test_team()
	test_chat_channels()
	test_dialog()
	test_task_values()
	test_describe_tip()
	test_give_item()
	print("client tests: %d passed, %d failed" % [_passed, _failed])
	quit(0 if _failed == 0 else 1)


# ---- characters (KMath.h, KNpcResNode / KNpcRes) --------------------------------------------

func test_kmath_direction() -> void:
	check(KMath.get_dir_index(0, 0, 0, 100) == 0, "down is 0")
	check(KMath.get_dir_index(0, 0, -100, 0) == 16, "left is 16: %d" % KMath.get_dir_index(0, 0, -100, 0))
	check(KMath.get_dir_index(0, 0, 0, -100) == 32, "up is 32 (x equal, 0x005E8DB0: 64 - 32): %d" % KMath.get_dir_index(0, 0, 0, -100))
	check(KMath.get_dir_index(0, 0, 100, 0) == 48, "right is 48 (64 - 16): %d" % KMath.get_dir_index(0, 0, 100, 0))
	check(KMath.get_dir_index(0, 0, -100, 100) == 8, "down-left is 8")
	check(KMath.get_dir_index(0, 0, 100, -100) == 40, "up-right is 40: %d" % KMath.get_dir_index(0, 0, 100, -100))
	check(KMath.get_dir_index(2000, 2000, 2050, 2000) == 48 and KMath.get_dir_index(2050, 2000, 2000, 2000) == 16, "east 48 / west 16 like the zone")
	# the nearer cell wins: nsin 726 sits between sin[7] 791 and sin[8] 724 -> 8; (100, 10): nsin 101 -> sin[15] 100 (1 away) not sin[14] 199
	check(KMath.get_dir_index(0, 0, -100, 10) == 15, "nearest cell: %d" % KMath.get_dir_index(0, 0, -100, 10))
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
		"G_Skills_42": "Trong lúc cưỡi ngựa không thể thi triển \n", "G_Skills_35": " (Công kích gần) \n", "G_Skills_49": "Tăng cho kỹ năng %s: %d%%\n",
		"G_Skills_38": "Cấp hiện tại: %d (%d+%d)", "G_Skills_39": "Tăng từ kỹ năng: %d%%\n", "G_Skills_50": "<color=Blue> Chiêu 1: <color>", "G_Skills_51": "<color=Blue> Chiêu 2: <color>",
		"G_Skills_52": "<color=Blue> Chiêu 3: <color>", "G_ITEM_22": "[Cấp %d]"},
		"descript": {"physicsdamage_v": "Sát thương vật lý: #d1- đến #d3- điểm", "attackrating_p": "Độ chính xác: #d1-%"},
		"skill_attrib": {"202": "Võ công lưu phái: <color=Cyan>Quyền pháp<color>"}, "weapon_limit": {"f1": "<color=White>Tay không<color>", "2": "Côn"}}
	var row := {"SkillName": "Hàng Long Bất Vũ", "SkillDesc": "Võ công nhập môn", "ReqLevel": "10", "Attrib": "202", "IsAura": "0", "Series": "0",
		"EqtLimit": "-1", "HorseLimit": "1", "IsMelee": "0", "IsExpSkill": "0"}
	var desc := {"skill_id": 14, "max_level": 20, "has_cur": true, "has_next": true,
		"cur": {"level": 1, "cost": 10, "cost_type": 0, "attack_radius": 90, "attribs": [{"group": 1, "name": "physicsdamage_v", "v0": 12, "v1": 0, "v2": 30}, {"group": 0, "name": "attackrating_p", "v0": 5, "v1": 0, "v2": 0}], "appends": [{"skill_id": 10, "value": 7}]},
		"next": {"level": 2, "cost": 11, "cost_type": 0, "attack_radius": 90, "attribs": [{"group": 1, "name": "physicsdamage_v", "v0": 14, "v1": 0, "v2": 33}], "appends": []}}
	var name_of := func(id: int) -> String: return "Kim Cang Phục Ma" if id == 10 else ("La Hán Trận" if id == 11 else str(id))
	var t: String = D.build(row, {"level": 1, "exp_percent": 0}, 20, desc, text, name_of)
	# G_Skills_76 (0x006FC300): Attrib 202 is [SkillType] 1 -> the equipment's percent; the state modifier's line follows (0x005EC4F0)
	var text76 := text.duplicate(true)
	text76.strings["G_Skills_76"] = "Trang bị gồm có:"
	text76["skill_type"] = {"202": 1, "304": 2}
	var desc76 := desc.duplicate(true)
	desc76["equip_percent"] = 12
	desc76["modifier"] = {"group": 2, "name": "attackrating_p", "v0": 7, "v1": 0, "v2": 0}
	var t76: String = D.build(row, {"level": 1, "exp_percent": 0}, 20, desc76, text76, name_of)
	check("Trang bị gồm có:12%
" in t76, "the equipment's share on a [SkillType] skill")
	check("Trang bị gồm có:Độ chính xác: 7%
" in t76, "the modifier's line through [Descript]")
	var row_other := row.duplicate()
	row_other["Attrib"] = "999"
	var t_other: String = D.build(row_other, {"level": 1, "exp_percent": 0}, 20, desc76, text76, name_of)
	check("Trang bị gồm có:12%" not in t_other and "Trang bị gồm có:Độ chính xác" in t_other, "no percent line outside [SkillType], the modifier line still")
	check("Trang bị gồm có" not in t, "nothing without equipment or a modifier")
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
	# a low character sees the requirement; a weapon skill (a plain attack) shows no level line and no next level; no answer yet
	# -> the static part
	var low: String = D.build(row, {"level": 1}, 5, desc, text, name_of)
	check(low.find("Đẳng cấp yêu cầu: 10") >= 0, "the requirement line at level 5")
	var weapon_row := row.duplicate()
	weapon_row["WeaponSkill"] = "1"
	var au: String = D.build(weapon_row, {"level": 1}, 20, desc, text, name_of)
	check(au.find("Cấp hiện tại") < 0 and au.find("Đẳng cấp tiếp theo") < 0 and au.find("Tiêu hao nội lực: 10") >= 0, "a weapon skill: no level, no next, its cost")
	var aura_row := row.duplicate()
	aura_row["IsAura"] = "1"
	check(D.build(aura_row, {"level": 1}, 20, desc, text, name_of).find("Cấp hiện tại: 1") >= 0, "an aura keeps its level line (IsAura is vtable +0x48, not the +0x4c GetDesc tests)")
	var pending: String = D.build(row, {"level": 1}, 20, {}, text, name_of)
	check(pending.find("Tiêu hao") < 0 and pending.find("Cấp hiện tại: 1") >= 0, "without the zone's answer: the static lines only")
	check(D.build(row, {"level": 1}, 20, desc, text, name_of).find("(Công kích gần)") < 0 and D.build({"SkillName": "Đấm", "Attrib": "1", "IsMelee": "1"}, {"level": 1}, 20, {}, text, name_of).find("(Công kích gần)") >= 0, "the plain attack words for Attrib 1 / 2 only")
	# the skills a level names (0x006FAA00 -> 0x006F7F70): the append skill held (flags 1: no "Chieu N:"), an event skill
	# (flags 0: "Chieu 2:" first - the counter starts at 1), an auto skill (flags 5: the name alone), each with its own lines
	var rel := desc.duplicate(true)
	rel.cur["related"] = [{"skill_id": 10, "level": 3, "flags": 1, "attribs": [{"group": 0, "name": "attackrating_p", "v0": 9, "v1": 0, "v2": 0}]},
		{"skill_id": 11, "level": 1, "flags": 0, "attribs": []}, {"skill_id": 12, "level": 2, "flags": 5, "attribs": []}]
	var tr: String = D.build(row, {"level": 1}, 20, rel, text, name_of)
	check(tr.find("<color=yellow>Kim Cang Phục Ma<color><color=blue>[Cấp 3]<color>\nĐộ chính xác: 9%\n") >= 0, "the append skill: name, level, its lines: %s" % tr)
	check(tr.find("<color=Blue> Chiêu 2: <color><color=yellow>La Hán Trận<color><color=blue>[Cấp 1]<color>\n") >= 0, "the event skill after Chieu 2: %s" % tr)
	check(tr.find("<color=yellow>12<color>\n") >= 0 and tr.find("Chiêu 3") < 0 and tr.find("Chiêu 1") < 0, "the auto skill: its name alone, no third counter")
	check(tr.find("Kim Cang Phục Ma<color><color=blue>[Cấp 3]") < tr.find("Tăng cho kỹ năng Kim Cang Phục Ma: 7%"), "the named skills before the addskilldamage lines")
	# G_Skills_38 with an increment, G_Skills_39 with the enhance map; the level shown is the zone's held level
	var inc := desc.duplicate(true)
	inc["held_level"] = 3
	inc["level_inc"] = 2
	inc["enhance"] = 25
	var ti: String = D.build(row, {"level": 1}, 20, inc, text, name_of)
	check(ti.find("<color=Blue>Cấp hiện tại: 3 (1+2)\n<bclr=Black><color>") >= 0 and ti.find("Tăng từ kỹ năng: 25%\n") >= 0, "level with increments and the enhance line: %s" % ti)
	check(t.find("Tăng từ kỹ năng") < 0 and t.find("Cấp hiện tại: 1\n") >= 0, "no enhance line and the plain level without them")
	# the addskilldamage line needs the target's ShowAddition (0x006FB3DF) when the rows are known
	var rows := func(id: int) -> Dictionary: return {"ShowAddition": "0"} if id == 10 else {"ShowAddition": "1"}
	check(D.build(row, {"level": 1}, 20, desc, text, name_of, rows).find("Tăng cho kỹ năng") < 0, "ShowAddition 0 hides the line")
	var rows1 := func(_id: int) -> Dictionary: return {"ShowAddition": "1"}
	check(D.build(row, {"level": 1}, 20, desc, text, name_of, rows1).find("Tăng cho kỹ năng Kim Cang Phục Ma: 7%") >= 0, "ShowAddition 1 shows it")


# ---- the missile frames (KMissleResMath.gd; KMissleRes::Draw of the old client, kept by the 2.0 one) ----------------
func test_missle_math() -> void:
	var M = load("res://scenes/KMissleResMath.gd")
	# 16 spokes over 64 directions: 4 each, rounding up from the half (KMissleRes::Draw 137..140)
	check(M.sprite_dir(0, 16) == 0 and M.sprite_dir(1, 16) == 0 and M.sprite_dir(2, 16) == 1 and M.sprite_dir(3, 16) == 1, "spokes of 4")
	check(M.sprite_dir(63, 16) == 0 and M.sprite_dir(61, 16) == 15 and M.sprite_dir(31, 8) == 4 and M.sprite_dir(5, 1) == 0, "the wrap, 8 spokes, one spoke")
	check(M.sprite_dir(10, 0) == 0 and M.sprite_dir(10, 100) == 0, "no spokes -> block 0")
	# a 64-frame sprite of 16 directions (4 a block), one pass over a 10-frame flight: 4 x cur / 10
	check(M.fly_frame(64, 16, 1, 0, 10, false, false, 0, 0) == 0 and M.fly_frame(64, 16, 1, 5, 10, false, false, 0, 0) == 2 and M.fly_frame(64, 16, 1, 9, 10, false, false, 0, 0) == 3, "one pass")
	check(M.fly_frame(64, 16, 1, 11, 10, false, false, 0, 0) == -1 and M.fly_frame(64, 16, 1, -1, 10, false, false, 0, 0) == -1, "past the life / before birth")
	check(M.fly_frame(64, 16, 1, 3, 0, false, false, 0, 0) == 3 and M.fly_frame(64, 16, 1, 4, 0, false, false, 0, 0) == -1, "all 0 = the block once")
	# LoopPlay: the frame cycles every `interval` frames; SubLoop cycles sub_start..sub_stop after the start
	check(M.fly_frame(64, 16, 1, 9, 100, true, false, 0, 0) == 1 and M.fly_frame(64, 16, 2, 9, 100, true, false, 0, 0) == 0, "loop, interval")
	check(M.fly_frame(64, 16, 1, 1, 100, true, true, 2, 4) == 1 and M.fly_frame(64, 16, 1, 2, 100, true, true, 2, 4) == 2 and M.fly_frame(64, 16, 1, 5, 100, true, true, 2, 4) == 3 and M.fly_frame(64, 16, 1, 6, 100, true, true, 2, 4) == 2, "sub loop 2..4")
	check(M.fly_frame(64, 16, 1, 7, 100, true, true, 3, 3) == 3, "sub loop with one frame")
	check(M.frame_index(64, 16, 1, 6, 5, 10, false, false, 0, 0) == 2 * 4 + 2 and M.frame_index(64, 16, 1, 6, 11, 10, false, false, 0, 0) == -1, "the sprite frame of direction 6 (block 2)")
	# the vanish movie: 6 frames of one block, interval 2 -> 12 frames long, one frame every 2
	check(M.special_frame(6, 1, 2, 20, 0) == 0 and M.special_frame(6, 1, 2, 20, 3) == 1 and M.special_frame(6, 1, 2, 20, 11) == 5 and M.special_frame(6, 1, 2, 20, 12) == -1, "special movie")
	# the climb of a MoveKind 7 missile (the zone's test: speed 512, acc 1024): up 512, then down, never below the ground
	var hs: Array = M.z_step(0, 512, 1024)
	check(hs[0] == 512 and hs[1] == -512, "first frame: 512 up, the speed turns: %s" % str(hs))
	hs = M.z_step(hs[0], hs[1], 1024)
	check(hs[0] == 0 and hs[1] == -1536, "second frame: back on the ground")
	check(M.z_step(100, -1536, 1024)[0] == 0, "never below the ground")
	check(KMath.get_dir_index(0, 0, 1024, 0) == 48 and KMath.get_dir_index(0, 0, 0, -1024) == 32, "a Z missile faces its vector: east 48, north 32")


# ---- knock back on the client (KNpc::KnockBack 0x005EE950 / OnKnockBack 0x005EFE00 of gamecl.exe) ------------------
func test_knock_back() -> void:
	# ((spot - here) << 10) / frames left, truncated toward zero, in 1/1024 units: 96 << 10 = 98304, / 5 = 19660 (not 19660.8)
	check(KMath.knock_step(Vector2(2050, 2000), Vector2(2146, 2000), 5) == Vector2(19660.0 / 1024.0, 0.0), "a fifth of 96, in 1/1024")
	check(KMath.knock_step(Vector2(2146, 2000), Vector2(2050, 2000), 5) == Vector2(-19660.0 / 1024.0, 0.0), "toward zero when negative")
	check(KMath.knock_step(Vector2(2050, 2000), Vector2(2146, 2000), 0) == Vector2(96.0, 0.0), "no frames left: the whole way")
	# five frames of the zone's test (2050 -> 2146 in 5): the integer part of `here` is what the 2.0 client sees (Map2Mps)
	var here := Vector2(2050, 2000)
	for f in 5:
		here += KMath.knock_step(here, Vector2(2146, 2000), 5 - f)
	check(absf(here.x - 2146.0) < 1.0 and here.y == 2000.0, "arrives within a unit: %s" % str(here))
	# facing: from the spot toward here (0x005E8DB0 of here - spot) - the pig pushed east looks west at the hero
	check(KMath.get_dir_index(2146, 2000, 2050, 2000) == 16, "faces the pusher")


# ---- the state pictures (KSprControl, KStateSpr; gamecl.exe 0x0070B920 / 0x006DF7E0 / 0x006E0610) ---------------

func test_state_pictures() -> void:
	# KSprControl::GetNextFrame 0x0070B920 - La Hán Trận (Status45: 10 frames, 1 dir, interval 8): frame = 10 * elapsed / 8
	var c := SprControl.new()
	c.set_spr_file("ring", 10, 1, 8, 100)
	check(c.check_exist() and c.total_frame == 10 and c.timer == 100 and c.one_dir_frames() == 10, "SetSprFile keeps the sprite")
	c.get_next_frame(103, true)
	check(c.cur_frame == 3, "10 * 3 / 8 = 3: %d" % c.cur_frame)
	c.get_next_frame(107, true)
	check(c.cur_frame == 8, "10 * 7 / 8 = 8: %d" % c.cur_frame)
	c.get_next_frame(108, true)
	check(c.cur_frame == 0 and c.timer == 108, "a Loop pass starts over at the interval")
	c.set_spr_file("ring", 10, 1, 8, 200)
	check(c.timer == 108, "the same sprite again changes nothing")
	var o := SprControl.new()
	o.set_spr_file("once", 8, 1, 4, 0)
	o.get_next_frame(4, false)
	check(o.cur_frame == 7 and o.check_end(), "a once-only picture holds its last frame (CheckEnd)")
	# SetCurDir64 0x0070B7C0 - eight blocks of 35 frames (Status238: 280 frames, 8 dirs, interval 36): dir 20 -> (20 + 4) / 8 = 3
	var d := SprControl.new()
	d.set_spr_file("eight", 280, 8, 36, 0)
	check(not d.set_cur_dir64(20, 5) and d.cur_dir == 3 and d.cur_frame == 105 and d.timer == 5, "a turn restarts at the block's first frame")
	check(d.set_cur_dir64(22, 6) and d.cur_dir == 3 and d.timer == 5, "the same block: no restart")
	check(not d.set_cur_dir64(62, 7) and d.cur_dir == 0 and d.cur_frame == 0, "dir 62 -> (62 + 4) / 8 = 8 -> block 0")
	d.get_next_frame(25, true)
	check(d.cur_frame == 17 and d.cur_dir_frame_no() == 17, "block 0, 35 * 18 / 36 = 17: %d" % d.cur_frame)
	d.set_cur_dir64(20, 30)
	d.get_next_frame(48, true)
	check(d.cur_frame == 105 + 17, "block 3, frame 17: %d" % d.cur_frame)
	var one := SprControl.new()
	one.set_spr_file("one", 10, 1, 8, 0)
	check(one.set_cur_dir64(40, 1) and one.cur_dir == 0, "a single block: every facing is block 0 ((40 + 32) / 64 = 1 -> 0)")
	check(not one.set_cur_dir64(64, 1) and not one.set_cur_dir64(-1, 1), "facings outside 0..63 are refused")
	# KStateSpr.sync (KNpcRes::SetState 0x006DF7E0): the six slots from the 0x7a packet's icons
	var rows := {
		45: {"sprite": "ring", "type": 2, "loop": true, "behind_start": 0, "behind_end": 0, "frames": 10, "dirs": 1, "interval": 8, "split": 1},
		52: {"sprite": "c", "type": 1, "loop": true, "behind_start": 0, "behind_end": 0, "frames": 13, "dirs": 1, "interval": 12, "split": 1},
		7: {"sprite": "shield", "type": 1, "loop": true, "behind_start": 0, "behind_end": 15, "frames": 20, "dirs": 1, "interval": 12, "split": 1},
		1: {"sprite": "", "type": 0, "special": true},
		77: {"sprite": "star", "type": 3, "loop": true, "frames": 10, "dirs": 1, "interval": 100},
	}
	var row_of := func(id: int): return rows.get(id, {})
	var slots: Array = []
	var fresh: Array = StateSpr.sync(slots, [0, 0, 0, 45, 45, 52], row_of, 10)
	check(slots.size() == 6 and fresh.size() == 2, "two pictures took slots: %d" % fresh.size())
	check(slots[0].id == 45 and slots[0].type == 2 and slots[1].id == 52 and slots[2].id == 0, "the first free slots, in icon order")
	check(slots[0].ctrl.file == "ring" and slots[0].ctrl.timer == 10 and slots[0].ctrl.total_frame == 10 and slots[0].loaded, "the control got the sprite")
	fresh = StateSpr.sync(slots, [45, 52], row_of, 11)
	check(fresh.is_empty() and slots[0].id == 45 and slots[0].ctrl.timer == 10, "nothing new: the same slots stay")
	fresh = StateSpr.sync(slots, [52, 7, 1, 77, 999], row_of, 12)
	check(slots[0].id == 7 and slots[1].id == 52 and slots[2].id == 77 and slots[3].id == 0 and fresh.size() == 2, "45 gone frees slot 0, 7 takes it; the Special row 1 and an unknown id take none, the MiniMap mark does")
	check(not slots[1].behind(), "c.spr (behind range 0..0) is in front of the body")
	slots[0].ctrl.cur_frame = 14
	check(slots[0].behind(), "the shield's frame 14 (range 0..15) is behind the body")
	slots[0].ctrl.cur_frame = 15
	check(not slots[0].behind(), "its frame 15 is in front")
	var foot: Array = []
	StateSpr.sync(foot, [45], row_of, 0)
	foot[0].ctrl.cur_frame = 9
	check(foot[0].behind(), "a Foot picture is always behind")
	# step (0x006E0610): a Loop one runs on, a once-only one is dropped at its last frame, a turn skips the step
	rows[9] = {"sprite": "once", "type": 0, "loop": false, "frames": 8, "dirs": 1, "interval": 4, "split": 1}
	StateSpr.sync(slots, [9], row_of, 20)
	var once = slots[0]
	check(once.id == 9 and slots[1].id == 0 and slots[2].id == 0, "9 alone: the rest freed")
	once.step(0, 21)
	check(once.loaded and once.ctrl.cur_frame == 2, "once: 8 * 1 / 4 = 2: %d" % once.ctrl.cur_frame)
	once.step(0, 24)
	check(not once.loaded and once.ctrl.cur_frame == 7, "dropped at its last frame")
	rows[238] = {"sprite": "eight", "type": 1, "loop": true, "frames": 280, "dirs": 8, "interval": 36, "split": 1}
	StateSpr.sync(slots, [238], row_of, 40)
	var eight = slots[0]
	eight.step(0, 58)
	check(eight.ctrl.cur_frame == 17, "block 0 after 18 of 36: %d" % eight.ctrl.cur_frame)
	eight.step(20, 59)
	check(eight.ctrl.cur_frame == 105 and eight.ctrl.timer == 59, "a turn to block 3 restarts and skips the step")
	eight.step(20, 77)
	check(eight.ctrl.cur_frame == 105 + 17, "then runs on in block 3: %d" % eight.ctrl.cur_frame)


# ---- the fight sounds (KWavSound; KMissleRes::PlaySound 0x00717ED0, KSkill::PlayCastSound 0x006F6D90) --------------

func test_wav_sound() -> void:
	# GetSndVolume 0x00717CC0 with nVol = -(|dx| + |dy|): hundredths of dB; the option scales the 10000 range
	check(WavSound.snd_volume(0, 0, 100) == 0, "at the focus: 0")
	check(WavSound.snd_volume(300, -200, 100) == -500, "500 units away: -5 dB (%d)" % WavSound.snd_volume(300, -200, 100))
	check(WavSound.snd_volume(3000, 0, 100) == -3000, "3000 units: -30 dB")
	check(WavSound.snd_volume(0, 0, 50) == -5000, "half the option: -50 dB even at the focus (the old law)")
	check(WavSound.snd_volume(20000, 0, 100) == -10000, "beyond 10000: DSBVOLUME_MIN")
	check(WavSound.snd_pan(300) == 1500 and WavSound.snd_pan(-300) == -1500, "pan = dx * 5")
	check(WavSound.snd_pan(5000) == 10000, "pan clamped to DSBPAN_RIGHT")
	# KWavSound::Play: the buffer's volume and place (playback itself needs a running tree - not in this runner)
	var w := WavSound.new()
	get_root().add_child(w)
	var stream := AudioStreamWAV.new()
	stream.format = AudioStreamWAV.FORMAT_16_BITS
	stream.mix_rate = 22050
	var silence := PackedByteArray()
	silence.resize(22050 * 2)   # half a second
	stream.data = silence
	w.stream_provider = func(file: String): return stream if file != "none" else null
	w.focus_provider = func(): return Vector2(1000, 2000)
	check(not w.play("", Vector2.ZERO) and not w.play("0", Vector2.ZERO), "no file: nothing")
	check(not w.play("none", Vector2.ZERO), "a file the archives lack: nothing")
	check(w.play("a.wav", Vector2(1300, 2000)) and w.played == 1, "the first buffer")
	check(w.get_child_count() == 1 and w.get_child(0).volume_db == -3.0, "300 units east: -3 dB (%f)" % w.get_child(0).volume_db)
	check(w.get_child(0).position == Vector2(1300, 1000), "placed at the screen spot (y / 2)")
	check(w.get_child(0).max_distance >= 1.0e9, "Godot's distance attenuation is out of the way (the old law sets the volume)")
	w.option_volume = 50
	check(w.play("a.wav", Vector2(1000, 2000)) and w.get_child_count() == 1, "an idle buffer is reused (KWavSound::GetFreeBuffer)")
	check(w.get_child(0).volume_db == -50.0, "the option halves the range: -50 dB at the focus (%f)" % w.get_child(0).volume_db)
	var no_provider := WavSound.new()
	check(not no_provider.play("a.wav", Vector2.ZERO), "no stream provider: silent")
	no_provider.free()
	w.queue_free()


# ---- the gold monsters (KNpcGold at KNpc+0x4c; the name painter 0x005F21B0, the filter 0x00642550) -----------------

func test_gold_name() -> void:
	# 0x005F242F: a monster's line is "%s/Lv:%d"; players and the other kinds keep the name (0x005F2283)
	check(NpcGold.name_text("Linh Miêu", 3, 19) == "Linh Miêu/Lv:19", "a monster shows its level")
	check(NpcGold.name_text("Lão Bản", 2, 19) == "Lão Bản", "a townsman does not")
	check(NpcGold.name_text("Hero", 1, 19) == "Hero", "a player does not")
	# 0x005F23E5..0x005F2419: kind 0 -> white; a kind within the client's 17 rows -> 0xFF6365FF; above them (the boss word
	# 16 + 1 of the server's table is NOT above 17: it is drawn like a gold one) -> 0xFFEBB200
	check(NpcGold.name_color(3, 0, 17) == Color(1, 1, 1), "plain: white")
	check(NpcGold.name_color(3, 13, 17).to_html(false) == "6365ff", "gold: 0xFF6365FF (%s)" % NpcGold.name_color(3, 13, 17).to_html(false))
	check(NpcGold.name_color(3, 17, 17).to_html(false) == "6365ff", "the boss word 17 against 17 client rows: still 0xFF6365FF")
	check(NpcGold.name_color(3, 18, 17).to_html(false) == "ebb200", "above the client's table: 0xFFEBB200")
	check(NpcGold.name_color(1, 5, 17) == Color(1, 1, 1), "a player's name is never coloured by it")
	# 0x00642550: the class of the hang-up filter - 1 plain, 2 gold, 3 above the table
	check(NpcGold.npc_class(0, 17) == 1 and NpcGold.npc_class(16, 17) == 2 and NpcGold.npc_class(18, 17) == 3, "the three classes")
	# the show switches: F7 / F8 flip a value 0 <-> 3 (0x0066B4A0: 0 or 1 -> 3, else 0)
	check(NpcGold.toggle_switch(0) == 3 and NpcGold.toggle_switch(1) == 3 and NpcGold.toggle_switch(3) == 0 and NpcGold.toggle_switch(2) == 0, "Switch flips 0 <-> 3")
	# the name block of a monster (0x006702BD..): nothing without bit 1; hovered / targeted -> 14; bit 2 -> 12; else nothing
	check(NpcGold.name_block(3, 0, false) == 0 and NpcGold.name_block(3, 0, true) == 0, "names off: nothing, even hovered (0x006702D2)")
	check(NpcGold.name_block(3, 1, false) == 0 and NpcGold.name_block(3, 1, true) == 14, "bit 1 alone: only the hovered one, size 14")
	check(NpcGold.name_block(3, 3, false) == 12 and NpcGold.name_block(3, 3, true) == 14, "F7 on (3): everyone at 12, the hovered one at 14")
	check(NpcGold.name_block(1, 0, false) == 12 and NpcGold.name_block(2, 0, false) == 12, "players and townsfolk keep their name line")
	# the life bar (0x00670243 .. PaintLife 0x005EACF0): players with bit 1, monsters hovered / targeted or with bit 2, others never
	check(not NpcGold.life_bar(1, 0, true) and NpcGold.life_bar(1, 1, false), "a player's bar needs the life switch")
	check(NpcGold.life_bar(3, 0, true) and not NpcGold.life_bar(3, 0, false) and NpcGold.life_bar(3, 2, false), "a monster's bar: hovered or bit 2")
	check(not NpcGold.life_bar(2, 3, true), "a townsman never (PaintLife refuses kind 3 unless forced)")
	# the life bar's colour by the percent (PaintLife 0x005EAE2F ..): 50 green, 25 yellow, below red
	check(NpcGold.life_bar_color(100) == Color(0, 1, 0) and NpcGold.life_bar_color(50) == Color(0, 1, 0), "green from 50")
	check(NpcGold.life_bar_color(49) == Color(1, 1, 0) and NpcGold.life_bar_color(25) == Color(1, 1, 0), "yellow from 25")
	# PaintLife 0x005EADF4..0x005EAE2D: the PK flag paints red (kill state) or (255, 0, 64); state 2 without the flag pink
	check(NpcGold.life_bar_color(100, 2, true) == Color(1, 0, 0), "kill state with the flag: red")
	check(NpcGold.life_bar_color(100, 1, true).is_equal_approx(Color(1, 0, 64.0 / 255.0)), "fight state with the flag: (255, 0, 64)")
	check(NpcGold.life_bar_color(100, 2, false).is_equal_approx(Color(1, 105.0 / 255.0, 180.0 / 255.0)), "state 2 without the flag: pink")
	check(NpcGold.life_bar_color(100, 0, false) == Color(0, 1, 0), "no PK: the percent colour")
	check(NpcGold.life_bar_color(10, 2, true, true).is_equal_approx(Color(230.0 / 255.0, 190.0 / 255.0, 0.0)), "a team mate: (230, 190, 0) before any PK colour (0x005EADD5)")
	# GetNpcPate 0x005EBCF0: a sitting player's head sinks MulDiv(30, cur, total) once MulDiv(10, cur, total) >= 8 (rounded)
	check(NpcGold.sit_pate_drop(false, 14, 15) == 0 and NpcGold.sit_pate_drop(true, 11, 15) == 0, "no sink standing or before frame 12 of 15")
	check(NpcGold.sit_pate_drop(true, 12, 15) == 24 and NpcGold.sit_pate_drop(true, 13, 15) == 26 and NpcGold.sit_pate_drop(true, 14, 15) == 28, "24 / 26 / 28 over the last three frames")
	check(NpcGold.sit_pate_drop(true, 8, 10) == 24 and NpcGold.sit_pate_drop(true, 7, 10) == 0, "MulDiv(10, 8, 10) = 8 sinks, 7 does not")
	check(NpcGold.life_bar_color(24) == Color(1, 0, 0) and NpcGold.life_bar_color(0) == Color(1, 0, 0), "red below")
	# a player's name by its current camp (0x005F2507, the table 0x5f2d94)
	check(NpcGold.player_name_color(0) == Color(1, 1, 1), "camp_begin: white")
	check(NpcGold.player_name_color(1).to_html(false) == "ffa85e", "camp_justice: 0xFFFFA85E (%s)" % NpcGold.player_name_color(1).to_html(false))
	check(NpcGold.player_name_color(2).to_html(false) == "ff92ff" and NpcGold.player_name_color(3).to_html(false) == "55ff91", "evil / balance")
	check(NpcGold.player_name_color(4) == Color(1, 0, 0) and NpcGold.player_name_color(6).to_html(false) == "ff69b4", "free red, above 4 pink")


# ---- the team (docs/LINUX-SERVER.md §17, CLIENT-2.0.md §21): the protocol round trip and the 0x53 sub-command numbers ----
func test_team() -> void:
	check(Proto.MsgId.C2G_TEAM == 1119 and Proto.MsgId.G2C_TEAM_SELF == 2130 and Proto.MsgId.G2C_TEAM_EVENT == 2131, "team message ids")
	# the sub-commands of the 0x53 packet as jx_linux_y 0x080DCC90 and the 2.0 client (0x005F7070 leave = 6, 0x005F70B0 kick = 7, 0x005F7100 change captain = 8) number them
	check(Proto.TeamCmd.TEAM_CREATE == 2 and Proto.TeamCmd.TEAM_OPEN_CLOSE == 3 and Proto.TeamCmd.TEAM_APPLY_ADD == 4 and Proto.TeamCmd.TEAM_ACCEPT == 5
		and Proto.TeamCmd.TEAM_LEAVE == 6 and Proto.TeamCmd.TEAM_KICK == 7 and Proto.TeamCmd.TEAM_CHANGE_CAPTAIN == 8 and Proto.TeamCmd.TEAM_DISMISS == 9
		and Proto.TeamCmd.TEAM_INVITE == 10 and Proto.TeamCmd.TEAM_REPLY_INVITE == 11, "0x53 sub-command numbers")
	var self_info := Proto.TeamSelf.new()
	self_info.set_in_team(true)
	self_info.set_team_id(3)
	self_info.set_state(1)
	self_info.set_captain(true)
	self_info.set_lead_level(4)
	self_info.set_lead_exp(1234)
	self_info.set_members_max(5)
	var l = self_info.new_leader()
	l.set_entity_id(77)
	l.set_name("Đại Hiệp")
	l.set_level(30)
	var m = self_info.add_members()
	m.set_entity_id(78)
	m.set_name("Tiểu Đệ")
	m.set_level(12)
	var back := Proto.TeamSelf.new()
	check(back.from_bytes(self_info.to_bytes()) == Proto.PB_ERR.NO_ERRORS, "TeamSelf from_bytes")
	check(back.get_in_team() and back.get_team_id() == 3 and back.get_state() == 1 and back.get_captain() and back.get_lead_level() == 4
		and back.get_lead_exp() == 1234 and back.get_members_max() == 5, "TeamSelf scalars")
	check(back.has_leader() and back.get_leader().get_entity_id() == 77 and back.get_leader().get_name() == "Đại Hiệp"
		and back.get_members().size() == 1 and back.get_members()[0].get_name() == "Tiểu Đệ" and back.get_members()[0].get_level() == 12, "TeamSelf leader + members")
	var ev := Proto.TeamEvent.new()
	ev.set_event(Proto.TeamEventKind.TEAM_EV_MSG)
	ev.set_arg(0x28)
	ev.set_entity_id(77)
	ev.set_name("Đại Hiệp")
	var ev2 := Proto.TeamEvent.new()
	check(ev2.from_bytes(ev.to_bytes()) == Proto.PB_ERR.NO_ERRORS and ev2.get_event() == Proto.TeamEventKind.TEAM_EV_MSG and ev2.get_arg() == 0x28
		and ev2.get_entity_id() == 77 and ev2.get_name() == "Đại Hiệp", "TeamEvent round trip")
	# the request the window sends
	var req := Proto.TeamReq.new()
	req.set_cmd(Proto.TeamCmd.TEAM_INVITE)
	req.set_target(78)
	req.set_flag(0)
	req.set_seq(9)
	var req2 := Proto.TeamReq.new()
	check(req2.from_bytes(req.to_bytes()) == Proto.PB_ERR.NO_ERRORS and req2.get_cmd() == Proto.TeamCmd.TEAM_INVITE and req2.get_target() == 78 and req2.get_seq() == 9, "TeamReq round trip")


# ---- the chat channels (UiMsgCentrePad, 消息集合面板_左.ini; KUiPlayerBar::SendChat 0x00475A10) ----------------------
func test_chat_channels() -> void:
	check(Proto.ChatChannel.CH_NEARBY == 0 and Proto.ChatChannel.CH_TEAM == 1 and Proto.ChatChannel.CH_WORLD == 2 and Proto.ChatChannel.CH_FACTION == 3
		and Proto.ChatChannel.CH_SYSTEM == 4 and Proto.ChatChannel.CH_CITY == 5 and Proto.ChatChannel.CH_TONG == 6 and Proto.ChatChannel.CH_WHISPER == 7,
		"ChatChannel = the [Channels] order")
	var req := Proto.ChatReq.new()
	req.set_text("doi oi")
	req.set_channel(Proto.ChatChannel.CH_TEAM)
	var req2 := Proto.ChatReq.new()
	check(req2.from_bytes(req.to_bytes()) == Proto.PB_ERR.NO_ERRORS and req2.get_channel() == Proto.ChatChannel.CH_TEAM and req2.get_text() == "doi oi", "ChatReq round trip")
	var pad_script: GDScript = load("res://ui/uicase/UiMsgCentrePad.gd")
	var pad = pad_script.new()
	# the real file's values (the exported khung-chat may be missing on a bare checkout)
	pad.from_sections(["CH_NEARBY", "CH_TEAM", "CH_WORLD", "CH_FACTION", "CH_SYSTEM", "CH_CITY"], {
		"CH_NEARBY": {"ShortName0": "Ngoạn", "TextColor": "255,255,255", "MenuText": "Lân cận", "SendMsgInterval": "2000", "SendMsgNum": "2"},
		"CH_TEAM": {"ShortName0": "Đội", "ShortName1": "T", "TextColor": "64,190,255", "MenuText": "Đội", "SendMsgInterval": "800", "SendMsgNum": "2"},
		"CH_WORLD": {"ShortName0": "Công", "ShortName1": "SJ", "TextColor": "146,255,143", "MenuText": "Thế Giới", "SendMsgInterval": "60000", "SendMsgNum": "2"},
		"CH_FACTION": {"ShortName0": "Phái", "TextColor": "225,210,165", "MenuText": "Môn Phái"},
		"CH_SYSTEM": {"ShortName0": "GM", "ShortName1": "Hệ thống", "TextColor": "255,0,0", "MenuText": "Hệ Thống"},
		"CH_CITY": {"ShortName0": "C", "ShortName1": "Thành thị", "ShortName2": "CS", "TextColor": "169,255,224", "MenuText": "Thành Thị"},
	})
	check(pad.channels.size() == 6 and pad.index_of("CH_CITY") == 5 and pad.short_name(1) == "Đội", "the channel table")
	check(pad.color_of(1).is_equal_approx(Color(64.0 / 255.0, 190.0 / 255.0, 1.0)), "TextColor of CH_TEAM")
	check(pad.index_by_short("T") == 1 and pad.index_by_short("SJ") == 2 and pad.index_by_short("CS") == 5 and pad.index_by_short("x") == -1, "ShortName lookup")
	# the menu: the speakable channels in order, each with its colour
	var menu: Array = pad.menu_entries()
	check(menu.size() == 5 and menu[0].index == 0 and menu[1].text == "Đội" and menu[4].index == 5 and menu[2].color.is_equal_approx(pad.color_of(2)), "menu entries")
	# the input prefixes of SendChat 0x00475A10
	pad.current = 0
	var r: Dictionary = pad.parse_input("xin chao")
	check(r.channel == 0 and r.text == "xin chao" and r.target == "", "a plain line goes on the current channel")
	r = pad.parse_input("/Auto2 chao ban")
	check(r.channel == 7 and r.target == "Auto2" and r.text == "chao ban", "/name text = a whisper")
	r = pad.parse_input("&T doi oi")
	check(r.channel == 1 and r.text == "doi oi" and r.target == "", "&short text = the channel by its short name")
	r = pad.parse_input("&SJ the gioi")
	check(r.channel == 2 and r.text == "the gioi", "&SJ = CH_WORLD by ShortName1")
	r = pad.parse_input("&zzz khong co")
	check(r.channel == 0 and r.text == "&zzz khong co", "an unknown short name: the line as typed on the current channel")
	r = pad.parse_input("/")
	check(r.channel == 7 and r.target == "" and r.text == "", "a bare slash")
	# SendMsgNum lines every SendMsgInterval ms
	check(pad.throttle(1, 1000) == 0 and pad.throttle(1, 1100) == 0, "two team lines pass")
	check(pad.throttle(1, 1200) == 1, "the third within 800 ms waits (1 s)")
	check(pad.throttle(1, 1900) == 0, "after the interval it passes again")
	check(pad.throttle(3, 0) == 0 and pad.throttle(3, 1) == 0 and pad.throttle(3, 2) == 0, "no interval: never held")
	# a received line: name in NameTextColor, text in the channel colour; a whisper of my own in TextColorSelf
	var line: Dictionary = pad.line({"channel": 1, "name": "Auto2", "text": "doi oi [x]"}, "Me")
	check(str(line.bbcode) == "[color=#dcdcdc]Auto2:[/color] [color=#40beff]doi oi [lb]x][/color]" and line.texture == null, "a team line")
	line = pad.line({"channel": 7, "name": "Me", "text": "bi mat"}, "Me")
	check(str(line.bbcode).begins_with("[color=#dcdcdc]Me:[/color] [color=#ffe2a8]"), "my own whisper in TextColorSelf")
	line = pad.line({"channel": 7, "name": "Someone", "text": "bi mat"}, "Me")
	check(str(line.bbcode).find("[color=#fc97ff]") > 0, "a stranger's whisper in TextColorUnknown")


# ---- the npc dialog (the 0x63 / 0x5f packets, UiMsgSel, UiInformation2) -----------------------------------------------
func test_dialog() -> void:
	check(Proto.MsgId.C2G_NPC_DIALOG == 1121 and Proto.MsgId.C2G_DIALOG_ANSWER == 1122 and Proto.MsgId.G2C_SCRIPT_ACTION == 2139, "dialog message ids")
	var a := Proto.ScriptAction.new()
	a.set_operate(0)
	a.set_ui_id(0)
	a.set_text("Xin chào")
	a.set_interactive(true)
	a.set_param(-1)
	a.add_options("Một")
	a.add_options("Hai")
	var a2 := Proto.ScriptAction.new()
	check(a2.from_bytes(a.to_bytes()) == Proto.PB_ERR.NO_ERRORS and a2.get_text() == "Xin chào" and a2.get_options().size() == 2
		and a2.get_options()[1] == "Hai" and a2.get_param() == -1 and a2.get_interactive(), "ScriptAction round trip")
	var ans := Proto.DialogAnswer.new()
	ans.set_index(1)
	var ans2 := Proto.DialogAnswer.new()
	check(ans2.from_bytes(ans.to_bytes()) == Proto.PB_ERR.NO_ERRORS and ans2.get_index() == 1 and ans2.get_kind() == 0, "DialogAnswer round trip")
	# the lines of the question window: the answers, or the closing line when there are none
	var sel_script: GDScript = load("res://ui/KUiDialogMath.gd")
	var lines: Array = sel_script.lines_for(["Một", "Hai"], "Kết thúc đối thoại")
	check(lines.size() == 2 and lines[1] == "Hai", "the answers as lines")
	lines = sel_script.lines_for([], "Kết thúc đối thoại")
	check(lines.size() == 1 and lines[0] == "Kết thúc đối thoại", "no answer: the closing line (G_UiMsgSel_0)")
	# the button of a page: "Tiếp tục" until the last page, "Hoàn thành" on it (G_PLAYER_14 / G_PLAYER_15)
	var info_script: GDScript = sel_script
	check(info_script.page_label(0, 3, "Tiếp tục", "Hoàn thành") == "Tiếp tục" and info_script.page_label(2, 3, "Tiếp tục", "Hoàn thành") == "Hoàn thành"
		and info_script.page_label(0, 1, "Tiếp tục", "Hoàn thành") == "Hoàn thành", "page labels (G_PLAYER_14 / G_PLAYER_15)")


# ---- the task values (KPlayerTask.gd, docs/LINUX-SERVER.md §21) ----------------------------------------------

func test_task_values() -> void:
	check(Proto.MsgId.G2C_TASK_VALUE == 2140 and Proto.MsgId.G2C_TASK_VALUES == 2141 and Proto.MsgId.C2G_TASK_VALUE == 1123, "task value message ids")
	var v := Proto.TaskValue.new()
	v.set_id(5)
	v.set_value(77)
	var v2 := Proto.TaskValue.new()
	check(v2.from_bytes(v.to_bytes()) == Proto.PB_ERR.NO_ERRORS and v2.get_id() == 5 and v2.get_value() == 77, "TaskValue round trip")
	var b := Proto.TaskValues.new()
	var e := b.add_values()
	e.set_id(1000)
	e.set_value(-3)
	var b2 := Proto.TaskValues.new()
	check(b2.from_bytes(b.to_bytes()) == Proto.PB_ERR.NO_ERRORS and b2.get_values().size() == 1 and b2.get_values()[0].get_value() == -3, "TaskValues round trip")
	var req := Proto.TaskValueReq.new()
	req.set_id(1276)
	req.set_value(4)
	var req2 := Proto.TaskValueReq.new()
	check(req2.from_bytes(req.to_bytes()) == Proto.PB_ERR.NO_ERRORS and req2.get_id() == 1276 and req2.get_value() == 4, "TaskValueReq round trip")
	var math: GDScript = load("res://scenes/KPlayerTask.gd")
	var values := {}
	check(math.set_value(values, 5, 77) and math.value_of(values, 5) == 77 and not math.set_value(values, 5, 77), "SetTaskValue stores and reports a change")
	check(math.set_value(values, 5, 0) and not values.has(5) and math.value_of(values, 5) == 0, "a zero drops the entry")
	check(not math.set_value(values, 0x1770, 1) and not math.set_value(values, -1, 1) and values.is_empty(), "ids outside 0..0x176f are refused")
	var changed: Array = math.apply_batch(values, [{"id": 1000, "value": 1}, {"id": 1001, "value": 0}, {"id": 1002, "value": 2}])
	check(changed == [1000, 1002] and math.value_of(values, 1002) == 2 and values.size() == 2, "a batch applies in order and names what changed")
	check(math.bits(0x53, 4, 3) == 5 and math.bits(0x53, 0, 8) == 0x53 and math.bits(0x53, 30, 3) == 0 and math.bits(-1, 31, 1) == 1, "GetBits")


# ---- Describe and TaskTip (UiNpcDescribe.gd, UiSysMsg.gd; docs/CLIENT-2.0.md §26) ------------------------------------

func test_describe_tip() -> void:
	check(Proto.MsgId.G2C_TASK_TIP == 2142, "task tip message id")
	var t := Proto.TaskTip.new()
	t.set_text("Bạn nhận được một nhiệm vụ")
	var t2 := Proto.TaskTip.new()
	check(t2.from_bytes(t.to_bytes()) == Proto.PB_ERR.NO_ERRORS and t2.get_text() == "Bạn nhận được một nhiệm vụ", "TaskTip round trip")
	var a := Proto.ScriptAction.new()
	a.set_ui_id(12)
	a.set_text("Mô tả")
	a.add_options("Một")
	var a2 := Proto.ScriptAction.new()
	check(a2.from_bytes(a.to_bytes()) == Proto.PB_ERR.NO_ERRORS and a2.get_ui_id() == 12 and a2.get_options().size() == 1, "ScriptAction ui 12 round trip")
	var math: GDScript = load("res://ui/KUiDialogMath.gd")
	# the pane keeps its distance to the right and bottom edges: 240x250 at (790,593) on 1024x768 reaches 6 px past the
	# right edge and 75 px past the bottom one, and does the same on 1280x720
	check(math.bottom_right_anchor(Vector2(790, 593), Vector2(240, 250), Vector2(1024, 768), Vector2(1280, 720)) == Vector2(1046, 545), "the system message pane anchored bottom right")
	check(math.bottom_right_anchor(Vector2(790, 593), Vector2(240, 250), Vector2(1024, 768), Vector2(1024, 768)) == Vector2(790, 593), "the theme's own screen keeps the layout's place")
	var msgs: Array = []
	check(math.sys_msg_add(msgs, {"type": 1, "text": "A", "blink": true, "priority": 3}, 1000, 8), "a message joins")
	check(not math.sys_msg_add(msgs, {"type": 1, "text": "A", "blink": true, "priority": 3}, 1500, 8) and msgs.size() == 1, "the same message again is dropped (0x004C3820)")
	check(math.sys_msg_add(msgs, {"type": 1, "text": "B", "blink": false, "priority": 1}, 2000, 8) and msgs[0].text == "B", "a lower priority goes in front (0x004C3A4D)")
	check(math.sys_msg_add(msgs, {"type": 5, "text": "C", "blink": true, "priority": 3}, 2500, 8) and msgs.size() == 3, "another type joins")
	check(not math.sys_msg_add(msgs, {"type": 9, "text": "D"}, 2600, 8) and not math.sys_msg_add(msgs, {"type": 0, "text": "D"}, 2600, 8), "types outside 1..8 are refused")
	check(math.sys_msg_latest(msgs, 1).text == "B" and math.sys_msg_latest(msgs, 5).text == "C" and math.sys_msg_latest(msgs, 2).is_empty(), "the newest message of a type")
	check(math.sys_msg_blinks(msgs, 1) and math.sys_msg_blinks(msgs, 5) and not math.sys_msg_blinks(msgs, 2), "a type blinks while one of its messages does")
	check(not math.sys_msg_prune(msgs, 30000, 30000) and msgs.size() == 3, "nothing older than the interval yet")
	check(math.sys_msg_prune(msgs, 31000, 30000) and msgs.size() == 2 and math.sys_msg_latest(msgs, 1).text == "B", "a message goes after SysMsgDisappearInterval")
	check(math.sys_msg_prune(msgs, 40000, 30000) and msgs.is_empty(), "all gone")


# ---- the give-item box (UiGiveItem.gd; docs/CLIENT-2.0.md §27) ----------------------------------------------------

func test_give_item() -> void:
	check(Proto.MsgId.C2G_GIVE_ITEMS == 1124 and Proto.MsgId.G2C_GIVE_ITEM_MSG == 2143, "give item message ids")
	var req := Proto.GiveItemsReq.new()
	req.set_kind(1)
	var e1 := req.add_items()
	e1.set_room(0)
	e1.set_x(3)
	e1.set_y(2)
	e1.set_cell_x(5)
	e1.set_cell_y(3)
	var req2 := Proto.GiveItemsReq.new()
	check(req2.from_bytes(req.to_bytes()) == Proto.PB_ERR.NO_ERRORS and req2.get_kind() == 1 and req2.get_items().size() == 1
		and req2.get_items()[0].get_x() == 3 and req2.get_items()[0].get_cell_y() == 3, "GiveItemsReq round trip")
	var m := Proto.GiveItemMsg.new()
	m.set_kind(1)
	m.set_text("Chắc chưa?")
	var m2 := Proto.GiveItemMsg.new()
	check(m2.from_bytes(m.to_bytes()) == Proto.PB_ERR.NO_ERRORS and m2.get_kind() == 1 and m2.get_text() == "Chắc chưa?", "GiveItemMsg round trip")
	var a := Proto.ScriptAction.new()
	a.set_ui_id(11)
	a.set_notify_changes(true)
	a.add_options("Giao nộp")
	var a2 := Proto.ScriptAction.new()
	check(a2.from_bytes(a.to_bytes()) == Proto.PB_ERR.NO_ERRORS and a2.get_ui_id() == 11 and a2.get_notify_changes() and a2.get_options()[0] == "Giao nộp", "ScriptAction ui 11 round trip")
	var math: GDScript = load("res://ui/KUiDialogMath.gd")
	# the 6 x 4 box: a 1 x 3 piece fits at (0,0), not at (0,2) (out of the bottom), not on a 2 x 2 piece at (0,0)
	check(math.give_box_place([], Vector2i(0, 0), 1, 3, 6, 4) == Vector2i(0, 0), "a piece fits in the empty box")
	check(math.give_box_place([], Vector2i(0, 2), 1, 3, 6, 4) == null and math.give_box_place([], Vector2i(6, 0), 1, 1, 6, 4) == null
		and math.give_box_place([], Vector2i(-1, 0), 1, 1, 6, 4) == null, "outside the box")
	var taken := [{"x": 0, "y": 0, "w": 2, "h": 2}]
	check(math.give_box_place(taken, Vector2i(1, 1), 1, 1, 6, 4) == null and math.give_box_place(taken, Vector2i(2, 0), 1, 1, 6, 4) == Vector2i(2, 0), "no two pieces on one cell")
	check(math.give_cell_code(0, 0) == 1 and math.give_cell_code(5, 3) == 24 and math.give_cell_code(2, 1) == 9, "the cell code y * 6 + x + 1")
	var entries: Array = math.give_entries([{"id": 7, "x": 2, "y": 1}, {"id": 9, "x": 0, "y": 0}], {7: {"room": 0, "x": 4, "y": 5}})
	check(entries.size() == 1 and entries[0].room == 0 and entries[0].x == 4 and entries[0].y == 5 and entries[0].cell_x == 2 and entries[0].cell_y == 1, "the 0x89 entries: the bag place and the cell; a piece that is gone is left out")
