# World view: the exported map (or a grid when the zone has no map bundle), entities as
# circles, click to move, chat box, debug HUD.  Screen = (scene x, scene y / 2).
extends Node2D

const NpcScript := preload("res://scenes/KNpc.gd")
const ScenePlaceScript := preload("res://scenes/KScenePlaceC.gd")
const KLogin := preload("res://net/KLogin.gd")
const GRID_CELL := 512

var _entities := {}          # entity_id -> Node2D
var _target: Node2D = null   # the entity the player attacks / selected
var _camera: Camera2D
var _map: Node2D             # MapView
var _entity_layer: Node2D    # y-sorted parent of entity nodes (MapView.ysort or a local one)
var _grid: Node2D
var _hud: Label
var _chat_log: RichTextLabel
var _chat_input: LineEdit
var _zoom := 1.0
var _spawn_count := 0
var _move_count := 0
var _action_count := 0
var _scene_w := 8192
var _scene_h := 8192


func _ready() -> void:
	_camera = $Camera
	_camera.zoom = Vector2(_zoom, _zoom)
	_map = Node2D.new()
	_map.set_script(ScenePlaceScript)
	_map.name = "Map"
	add_child(_map)
	var has_map := _setup_map()

	_build_hud()
	Game.map_changed.connect(_on_map_changed)
	Game.entity_spawn.connect(_on_spawn)
	Game.entity_despawn.connect(_on_despawn)
	Game.entity_move.connect(_on_move)
	Game.entity_action.connect(_on_action)
	Game.entity_life.connect(_on_life)
	Game.chat_msg.connect(_on_chat)
	Game.kicked.connect(_on_kicked)
	Game.connection_lost.connect(_on_connection_lost)
	for d in Game.entities.values():
		_add_entity(d)
	_update_camera(true)
	Log.info("ui", "world screen", {"zone": Game.zone_name, "entity": Game.entity_id, "entities": _entities.size(),
		"map": Game.map_id, "bundle": has_map})
	_append_chat("[color=gray]Vào %s. Click chuột trái để đi, Enter để chat, Esc để thoát.[/color]" % Game.zone_name)
	if "--auto" in OS.get_cmdline_user_args():
		_auto_run()


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


# Loads the map bundle of Game.map_id (or the plain grid) and sizes the camera; called on entering
# the world and again after a ChangeMap.
func _setup_map() -> bool:
	_scene_w = Game.scene_w if Game.scene_w > 0 else 8192
	_scene_h = Game.scene_h if Game.scene_h > 0 else 8192
	if _grid != null:
		_grid.queue_free()
		_grid = null
	if _entity_layer != null and _entity_layer != _map.objects and is_instance_valid(_entity_layer):
		_entity_layer.queue_free()
	_entity_layer = null
	var has_map: bool = Game.map_id > 0 and Assets.has_map(Game.map_id) and bool(_map.load_map(Game.map_id))
	if has_map:
		_entity_layer = _map.objects   # ordered by the old sorting tree, see KScenePlaceC
	else:
		_map.clear()
		_grid = Node2D.new()
		_grid.name = "Grid"
		_grid.set_script(preload("res://scenes/KSceneGrid.gd"))
		_grid.size = Vector2(_scene_w, _scene_h * 0.5)
		add_child(_grid)
		_entity_layer = Node2D.new()
		_entity_layer.y_sort_enabled = true
		_entity_layer.z_index = 1
		add_child(_entity_layer)
		if Game.map_id > 0:
			Log.warn("map", "map bundle missing, drawing grid", {"map_id": Game.map_id, "dir": Assets.assets_root()})
	_camera.limit_left = 0
	_camera.limit_top = 0
	_camera.limit_right = maxi(_scene_w, 1280)
	_camera.limit_bottom = maxi(int(_scene_h / 2), 720)
	return has_map


# G2C_CHANGE_MAP: drop every entity, load the new bundle; the zone's EntitySpawn (with ourselves)
# follows right after (KNpc::ChangeWorld -> SendSyncData of the old server).
func _on_map_changed(info: Dictionary) -> void:
	_select_target(null)
	for id in _entities.keys():
		var node: Node2D = _entities[id]
		if _map.map_id > 0:
			_map.remove_entity(node)
		node.queue_free()
	_entities.clear()
	var has_map := _setup_map()
	_camera.position = Vector2(float(info.get("x", 0)), float(info.get("y", 0)) * 0.5)
	_append_chat("[color=gray]Sang %s (map %d).[/color]" % [_map.info.get("name", "?") if has_map else "map", Game.map_id])
	Log.info("ui", "map changed", {"map": Game.map_id, "bundle": has_map, "x": info.get("x", 0), "y": info.get("y", 0)})


func _own() -> Node2D:
	return _entities.get(Game.entity_id)


func _update_camera(snap: bool) -> void:
	var own := _own()
	if own:
		# whole pixels only: a fractional camera position makes nearest-filtered tiles shimmer
		var target := own.position if snap else _camera.position.lerp(own.position, 0.3)
		_camera.position = target.round()


func _view_rect() -> Rect2:
	var size := get_viewport_rect().size / _camera.zoom
	return Rect2(_camera.get_screen_center_position() - size * 0.5, size)


func _process(delta: float) -> void:
	_update_camera(false)
	if _map.map_id > 0:
		_map.update_view(_view_rect(), delta)
	var own := _own()
	var st: Dictionary = Assets.stats()
	var target_text := ""
	if _target != null and is_instance_valid(_target):
		target_text = "  target %s %d/%d" % [_target.display_name, _target.life, _target.life_max]
	_hud.text = "%s  zone %d  map %d  entity %d  sid %d (%s)\npos %s  hp %d/%d%s\nentities %d  regions %d  sprites %d (%d MB)  rtt %d ms  fps %d" % [
		Game.zone_name, Game.zone_id, Game.map_id, Game.entity_id, Game.sid, Net.transport,
		str(Vector2i(own.scene_pos)) if own else "-", own.life if own else 0, own.life_max if own else 0, target_text,
		_entities.size(), _map.region_count(), st.sprites, st.mb, Game.last_rtt_ms, Engine.get_frames_per_second()]


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_LEFT:
			var p := get_global_mouse_position()
			var hit := _entity_at(p)
			if hit != null and hit.entity_id != Game.entity_id and hit.is_attackable():
				_select_target(hit)
				var aseq := Game.attack(hit.entity_id)
				Log.debug("ui", "click attack", {"target": hit.entity_id, "name": hit.display_name, "seq": aseq})
			else:
				var x := clampi(int(p.x), 0, _scene_w - 1)
				var y := clampi(int(p.y * 2.0), 0, _scene_h - 1)
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


func _set_zoom(z: float) -> void:
	_zoom = clampf(z, 0.25, 3.0)
	_camera.zoom = Vector2(_zoom, _zoom)


# The entity drawn under a world point (the one on top wins).
func _entity_at(world: Vector2) -> Node2D:
	var best: Node2D = null
	for node in _entities.values():
		if node.hit_test(node.to_local(world)) and (best == null or node.z_index > best.z_index):
			best = node
	return best


func _select_target(node: Node2D) -> void:
	if _target != null and is_instance_valid(_target) and _target != node:
		_target.set_target(false)
	_target = node
	if node != null:
		node.set_target(true)


func _leave() -> void:
	Game.leave_world()
	get_tree().change_scene_to_file("res://scenes/UiShell.tscn")


# ---- world events --------------------------------------------------------------------------

func _add_entity(d: Dictionary) -> void:
	var id := int(d.id)
	var node: Node2D = _entities.get(id)
	if node == null:
		node = Node2D.new()
		node.set_script(NpcScript)
		_entity_layer.add_child(node)
		_entities[id] = node
	node.setup(d, id == Game.entity_id)
	if _map.map_id > 0:
		_map.add_entity(node)
	_spawn_count += 1


func _on_spawn(list: Array) -> void:
	for d in list:
		_add_entity(d)


func _on_despawn(ids: Array) -> void:
	for id in ids:
		var node: Node2D = _entities.get(int(id))
		if node:
			if node == _target:
				_target = null
			if _map.map_id > 0:
				_map.remove_entity(node)
			node.queue_free()
			_entities.erase(int(id))


func _on_move(mv: Dictionary) -> void:
	var node: Node2D = _entities.get(int(mv.id))
	if node:
		node.apply_move(mv)
		_move_count += 1
	else:
		Log.trace("world", "move for unknown entity", {"id": mv.id})


func _on_action(a: Dictionary) -> void:
	_action_count += 1
	var node: Node2D = _entities.get(int(a.id))
	if node:
		node.apply_action(a)
		if node == _target and node.is_dead():
			_select_target(null)


func _on_life(l: Dictionary) -> void:
	var node: Node2D = _entities.get(int(l.id))
	if node:
		node.set_life(l)


func _on_chat(msg: Dictionary) -> void:
	_append_chat("[b]%s:[/b] %s" % [msg.name, str(msg.text).replace("[", "[lb]")])


func _append_chat(bbcode: String) -> void:
	_chat_log.append_text(bbcode + "\n")


func _on_chat_submitted(text: String) -> void:
	_chat_input.text = ""
	_chat_input.release_focus()
	if text.strip_edges() != "":
		Game.chat(text)


func _on_kicked(reason: int, text: String) -> void:
	_append_chat("[color=red]Bị ngắt: %s[/color]" % KLogin.result_text(reason, text))
	if KLogin.session_ends(reason):
		# replaced / shutdown / rate limit: the gateway closes the socket, back to the login screen
		Game.logout("kicked")
		get_tree().change_scene_to_file("res://scenes/UiShell.tscn")
	else:
		get_tree().change_scene_to_file("res://scenes/UiShell.tscn")


func _on_connection_lost(reason: String) -> void:
	Log.warn("ui", "connection lost in world", {"reason": reason})
	get_tree().change_scene_to_file("res://scenes/UiShell.tscn")


# ---- automation ----------------------------------------------------------------------------

# Automated run: move once, chat once, report, screenshot (windowed) and quit.
func _auto_run() -> void:
	await get_tree().create_timer(0.8).timeout
	var own := _own()
	var from: Vector2 = own.scene_pos if own else Vector2.ZERO
	var seq := Game.move_to(int(from.x) + 200, int(from.y))
	Log.info("auto", "auto move", {"seq": seq, "from_x": from.x, "to_x": from.x + 200})
	Game.chat("auto client here")
	# the zone may shorten the walk to the nearest walkable cell: success = stopped away from start
	var arrived := false
	var waited := 0.0
	while waited < 10.0 and not arrived:
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
		own = _own()
		arrived = own != null and _move_count > 0 and not own.is_moving() and own.scene_pos.distance_to(from) > 8.0
	Log.info("auto", "auto result", {"arrived": arrived, "entities": _entities.size(), "spawns": _spawn_count, "moves": _move_count,
		"rtt_ms": Game.last_rtt_ms, "regions": _map.region_count(), "sprites": Assets.stats().sprites})
	print("AUTO_RESULT arrived=%s entities=%d moves=%d regions=%d transport=%s" % [arrived, _entities.size(), _move_count, _map.region_count(), Net.transport])
	await _save_screenshot("user://logs/auto_world.png")
	await _auto_fight()
	# stability probe: two frames half a second apart while idle must be (almost) identical
	if DisplayServer.get_name() != "headless":
		await get_tree().create_timer(1.0).timeout
		var cam_a := _camera.position
		await RenderingServer.frame_post_draw
		var img_a := get_viewport().get_texture().get_image()
		await get_tree().create_timer(0.5).timeout
		var cam_b := _camera.position
		await RenderingServer.frame_post_draw
		var img_b := get_viewport().get_texture().get_image()
		var diff := 0
		var total := 0
		for y in range(0, img_a.get_height(), 4):
			for x in range(0, img_a.get_width(), 4):
				total += 1
				if img_a.get_pixel(x, y) != img_b.get_pixel(x, y):
					diff += 1
		Log.info("auto", "idle frame diff", {"diff": diff, "sampled": total, "cam_a": str(cam_a), "cam_b": str(cam_b), "fps": Engine.get_frames_per_second(),
			"anims": _map.anim_count(), "regions": _map.region_count()})
		print("AUTO_IDLE_DIFF diff=%d of %d cam_a=%s cam_b=%s anims=%d" % [diff, total, str(cam_a), str(cam_b), _map.anim_count()])
		img_a.save_png("user://logs/auto_world_a.png")
		img_b.save_png("user://logs/auto_world_b.png")
	Game.leave_world()
	await get_tree().create_timer(0.3).timeout
	get_tree().quit(0 if arrived else 1)


# --auto: walk up to the nearest monster in sight and attack it until it dies (or 8 s pass);
# prints AUTO_FIGHT so tools/dev.py e2e / screenshot can check the combat path end to end.
func _auto_fight() -> void:
	var own := _own()
	if own == null:
		print("AUTO_FIGHT none")
		return
	var best: Node2D = null
	var best_d := 400.0
	for node in _entities.values():
		if node == own or not node.is_attackable():
			continue
		var d: float = node.scene_pos.distance_to(own.scene_pos)
		if d < best_d:
			best = node
			best_d = d
	if best == null:
		print("AUTO_FIGHT none")
		return
	# stand next to it, then swing
	var dir: Vector2 = (best.scene_pos - own.scene_pos).normalized()
	var stand: Vector2 = best.scene_pos - dir * 48.0
	Game.move_to(int(stand.x), int(stand.y))
	var waited := 0.0
	while waited < 6.0 and (own.is_moving() or own.scene_pos.distance_to(stand) > 24.0):
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
	var life_before: int = best.life
	var actions_before := _action_count
	_select_target(best)
	Game.attack(best.entity_id)
	await get_tree().create_timer(0.7).timeout
	await _save_screenshot("user://logs/auto_fight.png")
	waited = 0.0
	while waited < 8.0 and is_instance_valid(best) and not best.is_dead() and best.life > 0:
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
	var alive: bool = is_instance_valid(best)
	var life_after: int = best.life if alive else 0
	var dead: bool = (not alive) or best.is_dead()
	Log.info("auto", "auto fight", {"target": best.entity_id if alive else 0, "life_before": life_before, "life_after": life_after,
		"actions": _action_count - actions_before, "dead": dead, "own_doing": own.doing})
	print("AUTO_FIGHT target=%d name=%s life_before=%d life_after=%d actions=%d dead=%s" % [best.entity_id if alive else 0,
		best.display_name if alive else "?", life_before, life_after, _action_count - actions_before, dead])
	await _save_screenshot("user://logs/auto_fight_end.png")


# Saves the rendered frame (no-op in headless mode); used by tools/dev.py screenshot.
func _save_screenshot(path: String) -> void:
	if DisplayServer.get_name() == "headless":
		return
	await RenderingServer.frame_post_draw
	var img := get_viewport().get_texture().get_image()
	var err := img.save_png(path)
	Log.info("auto", "screenshot", {"path": ProjectSettings.globalize_path(path), "error": error_string(err)})
	print("AUTO_SCREENSHOT " + ProjectSettings.globalize_path(path))
