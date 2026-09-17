# Log - structured JSON-line logging for the Godot client (next/docs/LOGGING.md).
# Same schema as jx::log (C++) and pkg/log (Go): ts,lvl,cat,proc,sid,pid,zone,tick,msg,<fields>.
# Writes user://logs/client.log (rotates once at startup) and the console; keeps a ring buffer.
extends Node

enum Level { TRACE, DEBUG, INFO, WARN, ERROR, FATAL, OFF }
const LEVEL_NAMES := ["trace", "debug", "info", "warn", "error", "fatal", "off"]
const RING_CAPACITY := 2000
const LOG_DIR := "user://logs"
const LOG_PATH := "user://logs/client.log"

var process := "client"
var default_level: int = Level.INFO
var console := true
var levels := {}          # category -> Level
var ctx := {"sid": 0, "pid": 0, "zone": 0, "tick": 0}
var ring: Array[String] = []
var _file: FileAccess
var _flush_timer := 0.0


func _process(delta: float) -> void:
	# flush once a second so a killed process loses at most one second of log
	_flush_timer += delta
	if _flush_timer >= 1.0 and _file:
		_flush_timer = 0.0
		_file.flush()


func _ready() -> void:
	_open_file()
	var args := OS.get_cmdline_user_args()
	for a in args:
		if a.begins_with("--log-level="):
			default_level = level_from_name(a.substr(12))
		elif a.begins_with("--log-levels="):
			set_levels(a.substr(13))
	if OS.has_environment("JX_LOG__LEVEL"):
		default_level = level_from_name(OS.get_environment("JX_LOG__LEVEL"))
	if OS.has_environment("JX_LOG__LEVELS"):
		set_levels(OS.get_environment("JX_LOG__LEVELS"))
	info("boot", "log ready", {"path": ProjectSettings.globalize_path(LOG_PATH), "level": LEVEL_NAMES[default_level]})


func _open_file() -> void:
	DirAccess.make_dir_recursive_absolute(ProjectSettings.globalize_path(LOG_DIR))
	if FileAccess.file_exists(LOG_PATH):
		var dir := DirAccess.open(LOG_DIR)
		if dir:
			dir.rename("client.log", "client.1.log")
	_file = FileAccess.open(LOG_PATH, FileAccess.WRITE)


static func level_name(level: int) -> String:
	if level < 0 or level >= LEVEL_NAMES.size():
		return "info"
	return LEVEL_NAMES[level]


static func level_from_name(name: String) -> int:
	var n := name.strip_edges()
	if n == "warning":
		return Level.WARN
	var idx := LEVEL_NAMES.find(n)
	return idx if idx >= 0 else Level.INFO


func set_level(category: String, level: int) -> void:
	if category == "":
		default_level = level
	else:
		levels[category] = level


# "net=trace,zone.tick=debug,=warn"
func set_levels(spec: String) -> void:
	for item in spec.split(","):
		var eq := item.find("=")
		if eq >= 0:
			set_level(item.substr(0, eq).strip_edges(), level_from_name(item.substr(eq + 1)))
		elif item.strip_edges() != "":
			default_level = level_from_name(item)


# longest configured dotted prefix wins
func effective_level(category: String) -> int:
	var key := category
	while true:
		if levels.has(key):
			return levels[key]
		var dot := key.rfind(".")
		if dot < 0:
			break
		key = key.substr(0, dot)
	return default_level


func enabled(category: String, level: int) -> bool:
	if level == Level.OFF:
		return false
	return level >= effective_level(category)


static func timestamp_utc() -> String:
	var unix := Time.get_unix_time_from_system()
	var whole := int(floor(unix))
	var usec := int((unix - whole) * 1000000.0)
	var d := Time.get_datetime_dict_from_unix_time(whole)
	return "%04d-%02d-%02dT%02d:%02d:%02d.%06dZ" % [d.year, d.month, d.day, d.hour, d.minute, d.second, usec]


# Builds the line in the standard field order; values of extra fields are always strings.
func format_line(level: int, category: String, msg: String, fields: Dictionary, context: Dictionary) -> String:
	var parts: Array[String] = []
	parts.append('"ts":' + JSON.stringify(timestamp_utc()))
	parts.append('"lvl":' + JSON.stringify(level_name(level)))
	parts.append('"cat":' + JSON.stringify(category))
	if process != "":
		parts.append('"proc":' + JSON.stringify(process))
	for key in ["sid", "pid", "zone", "tick"]:
		if context.get(key, 0) != 0:
			parts.append('"%s":%d' % [key, int(context[key])])
	parts.append('"msg":' + JSON.stringify(msg))
	for key in fields:
		parts.append(JSON.stringify(str(key)) + ":" + JSON.stringify(str(fields[key])))
	return "{" + ",".join(parts) + "}"


func write(level: int, category: String, msg: String, fields: Dictionary = {}) -> void:
	if not enabled(category, level):
		return
	var line := format_line(level, category, msg, fields, ctx)
	ring.append(line)
	if ring.size() > RING_CAPACITY:
		ring.pop_front()
	if console:
		if level >= Level.ERROR:
			printerr(line)
		else:
			print(line)
	if _file:
		_file.store_line(line)
		if level >= Level.WARN:
			_file.flush()


func trace(category: String, msg: String, fields: Dictionary = {}) -> void:
	write(Level.TRACE, category, msg, fields)


func debug(category: String, msg: String, fields: Dictionary = {}) -> void:
	write(Level.DEBUG, category, msg, fields)


func info(category: String, msg: String, fields: Dictionary = {}) -> void:
	write(Level.INFO, category, msg, fields)


func warn(category: String, msg: String, fields: Dictionary = {}) -> void:
	write(Level.WARN, category, msg, fields)


func error(category: String, msg: String, fields: Dictionary = {}) -> void:
	write(Level.ERROR, category, msg, fields)


func fatal(category: String, msg: String, fields: Dictionary = {}) -> void:
	write(Level.FATAL, category, msg, fields)
	dump_ring("user://logs/client.crash.log")
	if _file:
		_file.flush()


func dump_ring(path: String) -> void:
	var f := FileAccess.open(path, FileAccess.WRITE)
	if f == null:
		return
	for line in ring:
		f.store_line(line)
	f.close()


func _exit_tree() -> void:
	if _file:
		_file.flush()
		_file.close()
