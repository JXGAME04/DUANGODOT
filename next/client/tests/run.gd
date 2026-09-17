# Headless unit tests for the client (no editor, no window):
#   godot --headless --path client -s tests/run.gd
# Exit code 0 when every check passes.  Keep the checks aligned with the C++/Go contracts.
extends SceneTree

const Proto := preload("res://proto/jx_pb.gd")
const NetScript := preload("res://net/KProtocol.gd")   # pure framing (autoload scripts cannot be preloaded here)
const LogScript := preload("res://autoload/KDebug.gd")
const SceneMath := preload("res://scenes/KSceneMath.gd")
const IpoTree := preload("res://scenes/KIpoTree.gd")
const IpotLeaf := preload("res://scenes/KIpotLeaf.gd")

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
	test_log_format()
	test_proto_round_trip()
	test_scene_math()
	test_ipot_order()
	print("client tests: %d passed, %d failed" % [_passed, _failed])
	quit(0 if _failed == 0 else 1)


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
