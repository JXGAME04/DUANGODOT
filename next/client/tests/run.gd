# Headless unit tests for the client (no editor, no window):
#   godot --headless --path client -s tests/run.gd
# Exit code 0 when every check passes.  Keep the checks aligned with the C++/Go contracts.
extends SceneTree

const Proto := preload("res://proto/jx_pb.gd")
const NetScript := preload("res://net/KProtocol.gd")   # pure framing (autoload scripts cannot be preloaded here)
const LogScript := preload("res://autoload/KDebug.gd")

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
	print("client tests: %d passed, %d failed" % [_passed, _failed])
	quit(0 if _failed == 0 else 1)


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
