# World view: a grid map, entities as circles, click to move, chat box, debug HUD.
extends Node2D

const EntityScene := preload("res://scenes/entity.gd")
const MAP_SIZE := 8192
const CELL := 512

var _entities := {}          # entity_id -> Node2D
var _camera: Camera2D
var _hud: Label
var _chat_log: RichTextLabel
var _chat_input: LineEdit
var _zoom := 1.0
var _spawn_count := 0
var _move_count := 0


func _ready() -> void:
	_camera = $Camera
	_camera.zoom = Vector2(_zoom, _zoom)
	_build_hud()
	Game.entity_spawn.connect(_on_spawn)
	Game.entity_despawn.connect(_on_despawn)
	Game.entity_move.connect(_on_move)
	Game.chat_msg.connect(_on_chat)
	Game.kicked.connect(_on_kicked)
	Game.connection_lost.connect(_on_connection_lost)
	# entities that arrived before this scene existed
	for d in Game.entities.values():
		_add_entity(d)
	Log.info("ui", "world screen", {"zone": Game.zone_name, "entity": Game.entity_id, "entities": _entities.size()})
	_append_chat("[color=gray]Vào %s. Click chuột trái để đi, Enter để chat, Esc để thoát.[/color]" % Game.zone_name)
	if "--auto" in OS.get_cmdline_user_args():
		_auto_run()


# Automated run: move once, chat once, report and quit (used by tools/dev.py and CI).
func _auto_run() -> void:
	await get_tree().create_timer(0.5).timeout
	var own: Node2D = _entities.get(Game.entity_id)
	var from := own.position if own else Vector2.ZERO
	var seq := Game.move_to(int(from.x) + 200, int(from.y))
	Log.info("auto", "auto move", {"seq": seq, "from_x": from.x, "to_x": from.x + 200})
	Game.chat("auto client here")
	var arrived := false
	var waited := 0.0
	while waited < 10.0 and not arrived:
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
		own = _entities.get(Game.entity_id)
		arrived = own != null and own.position == own.target and int(own.position.x) == int(from.x) + 200
	Log.info("auto", "auto result", {"arrived": arrived, "entities": _entities.size(), "spawns": _spawn_count, "moves": _move_count, "rtt_ms": Game.last_rtt_ms})
	print("AUTO_RESULT arrived=%s entities=%d moves=%d" % [arrived, _entities.size(), _move_count])
	await _save_screenshot("user://logs/auto_world.png")
	Game.leave_world()
	await get_tree().create_timer(0.3).timeout
	get_tree().quit(0 if arrived else 1)


func _build_hud() -> void:
	var layer := CanvasLayer.new()
	add_child(layer)
	_hud = Label.new()
	_hud.position = Vector2(8, 8)
	_hud.add_theme_color_override("font_color", Color.WHITE)
	_hud.add_theme_color_override("font_shadow_color", Color.BLACK)
	_hud.add_theme_constant_override("shadow_offset_x", 1)
	_hud.add_theme_constant_override("shadow_offset_y", 1)
	layer.add_child(_hud)

	var chat_box := VBoxContainer.new()
	chat_box.set_anchors_preset(Control.PRESET_BOTTOM_LEFT)
	chat_box.position = Vector2(8, -8)
	chat_box.custom_minimum_size = Vector2(420, 180)
	chat_box.grow_vertical = Control.GROW_DIRECTION_BEGIN
	layer.add_child(chat_box)
	_chat_log = RichTextLabel.new()
	_chat_log.bbcode_enabled = true
	_chat_log.scroll_following = true
	_chat_log.custom_minimum_size = Vector2(420, 150)
	_chat_log.add_theme_color_override("default_color", Color.WHITE)
	chat_box.add_child(_chat_log)
	_chat_input = LineEdit.new()
	_chat_input.placeholder_text = "Chat (Enter)"
	_chat_input.text_submitted.connect(_on_chat_submitted)
	chat_box.add_child(_chat_input)


func _process(_delta: float) -> void:
	var own: Node2D = _entities.get(Game.entity_id)
	if own:
		_camera.position = own.position
	_hud.text = "%s  zone %d  entity %d  sid %d\npos %s  entities %d  spawns %d  moves %d\nrtt %d ms  fps %d" % [
		Game.zone_name, Game.zone_id, Game.entity_id, Game.sid,
		str(Vector2i(own.position)) if own else "-", _entities.size(), _spawn_count, _move_count,
		Game.last_rtt_ms, Engine.get_frames_per_second()]


func _draw() -> void:
	var grid := Color(1, 1, 1, 0.12)
	for i in range(0, MAP_SIZE + 1, CELL):
		draw_line(Vector2(i, 0), Vector2(i, MAP_SIZE), grid, 1.0)
		draw_line(Vector2(0, i), Vector2(MAP_SIZE, i), grid, 1.0)
	draw_rect(Rect2(0, 0, MAP_SIZE, MAP_SIZE), Color(1, 1, 1, 0.4), false, 3.0)


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_LEFT:
			var p := get_global_mouse_position()
			var x := clampi(int(p.x), 0, MAP_SIZE - 1)
			var y := clampi(int(p.y), 0, MAP_SIZE - 1)
			var seq := Game.move_to(x, y)
			Log.debug("ui", "click move", {"x": x, "y": y, "seq": seq})
		elif event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_set_zoom(_zoom * 1.15)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_set_zoom(_zoom / 1.15)
	elif event is InputEventKey and event.pressed:
		if event.keycode == KEY_ESCAPE:
			_leave()
		elif event.keycode == KEY_ENTER or event.keycode == KEY_KP_ENTER:
			if not _chat_input.has_focus():
				_chat_input.grab_focus()


# Saves the rendered frame (no-op in headless mode); used by tools/dev.py screenshot.
func _save_screenshot(path: String) -> void:
	if DisplayServer.get_name() == "headless":
		return
	await RenderingServer.frame_post_draw
	var img := get_viewport().get_texture().get_image()
	var err := img.save_png(path)
	Log.info("auto", "screenshot", {"path": ProjectSettings.globalize_path(path), "error": error_string(err)})
	print("AUTO_SCREENSHOT " + ProjectSettings.globalize_path(path))


func _set_zoom(z: float) -> void:
	_zoom = clampf(z, 0.15, 3.0)
	_camera.zoom = Vector2(_zoom, _zoom)


func _leave() -> void:
	Game.leave_world()
	get_tree().change_scene_to_file("res://scenes/char_select.tscn")


# ---- world events --------------------------------------------------------------------------

func _add_entity(d: Dictionary) -> void:
	var id := int(d.id)
	var node: Node2D = _entities.get(id)
	if node == null:
		node = Node2D.new()
		node.set_script(EntityScene)
		add_child(node)
		_entities[id] = node
	node.setup(d, id == Game.entity_id)
	_spawn_count += 1


func _on_spawn(list: Array) -> void:
	for d in list:
		_add_entity(d)


func _on_despawn(ids: Array) -> void:
	for id in ids:
		var node: Node2D = _entities.get(int(id))
		if node:
			node.queue_free()
			_entities.erase(int(id))


func _on_move(mv: Dictionary) -> void:
	var node: Node2D = _entities.get(int(mv.id))
	if node:
		node.apply_move(mv)
		_move_count += 1
	else:
		Log.trace("world", "move for unknown entity", {"id": mv.id})


func _on_chat(msg: Dictionary) -> void:
	_append_chat("[b]%s:[/b] %s" % [msg.name, str(msg.text).replace("[", "[lb]")])


func _append_chat(bbcode: String) -> void:
	_chat_log.append_text(bbcode + "\n")


func _on_chat_submitted(text: String) -> void:
	_chat_input.text = ""
	_chat_input.release_focus()
	if text.strip_edges() != "":
		Game.chat(text)


func _on_kicked(_reason: int, text: String) -> void:
	_append_chat("[color=red]Bị ngắt: %s[/color]" % text)
	get_tree().change_scene_to_file("res://scenes/char_select.tscn")


func _on_connection_lost(reason: String) -> void:
	Log.warn("ui", "connection lost in world", {"reason": reason})
	get_tree().change_scene_to_file("res://scenes/login.tscn")
