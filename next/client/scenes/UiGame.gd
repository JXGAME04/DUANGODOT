# World view: the exported map (or a grid when the zone has no map bundle), entities as
# circles, click to move, chat box, debug HUD.  Screen = (scene x, scene y / 2).
extends Node2D

const NpcScript := preload("res://scenes/KNpc.gd")
const ObjScript := preload("res://scenes/KObj.gd")
const MissleScript := preload("res://scenes/KMissle.gd")
const MissleEffectScript := preload("res://scenes/KMissleEffect.gd")
const ENTITY_DROP := 4
const PICK_UP_RANGE := 180.0          # scene units: inside PLAYER_PICKUP_SERVER_DISTANCE (200) with a margin
const ScenePlaceScript := preload("res://scenes/KScenePlaceC.gd")
const KLogin := preload("res://net/KLogin.gd")
const KUiGameWindows := preload("res://ui/KUiGameWindows.gd")
const KUiItemView := preload("res://ui/KUiItemView.gd")
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
var _windows: KUiGameWindows = null   # the bag, the character window, the tooltip, the item on the cursor
var _pending_pickup := 0              # the ground object the player walks to (0 = none)


func _ready() -> void:
	_camera = $Camera
	_camera.zoom = Vector2(_zoom, _zoom)
	_map = Node2D.new()
	_map.set_script(ScenePlaceScript)
	_map.name = "Map"
	add_child(_map)
	var has_map := _setup_map()

	_build_hud()
	_windows = KUiGameWindows.new()
	_windows.quick_skill.connect(func(id: int): cast_skill_at_mouse(id))
	_windows.name = "Windows"
	add_child(_windows)
	# the 2.0 bottom bar carries the chat line ([InputEdit] of 玩家信息主界面.ini): the plain one steps aside
	if _windows.player_bar != null and _windows.player_bar.chat_input != null:
		_chat_input.visible = false
		_chat_input = _windows.player_bar.chat_input
		_windows.player_bar.chat_submitted.connect(_on_chat_submitted)
	Game.map_changed.connect(_on_map_changed)
	Game.entity_spawn.connect(_on_spawn)
	Game.entity_despawn.connect(_on_despawn)
	Game.entity_move.connect(_on_move)
	Game.entity_action.connect(_on_action)
	Game.missle_sync.connect(_on_missle)
	Game.entity_life.connect(_on_life)
	Game.chat_msg.connect(_on_chat)
	Game.kicked.connect(_on_kicked)
	Game.connection_lost.connect(_on_connection_lost)
	for d in Game.entities.values():
		_add_entity(d)
	_update_camera(true)
	Log.info("ui", "world screen", {"zone": Game.zone_name, "entity": Game.entity_id, "entities": _entities.size(),
		"map": Game.map_id, "bundle": has_map})
	_append_chat("[color=gray]Vào %s. Click chuột trái để đi, Enter để chat, F4 túi đồ, F3 nhân vật, F5 kỹ năng, Q..C kỹ năng tắt, 1..9 ô nhanh, Esc để thoát.[/color]" % Game.zone_name)
	if "--auto" in OS.get_cmdline_user_args():
		_auto_run()


func _build_hud() -> void:
	var layer := CanvasLayer.new()
	add_child(layer)
	_hud = Label.new()
	_hud.position = Vector2(8, 104)   # below the 2.0 top bar (27 px) and the skill state list (146,28 240x72)
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
	_walk_to_pickup()
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
			if hit != null and hit.entity_type == ENTITY_DROP:
				_pick_up(hit)
			elif hit != null and hit.entity_id != Game.entity_id and hit.is_attackable():
				_select_target(hit)
				# the left mouse skill of the old client (KPlayer::m_nLeftSkillID): a skill picked in the book is cast
				# (NpcSkillCommand), else the plain attack of the weapon (the swing the zone picks for it)
				if Game.left_skill > 0 and Game.skills.has(Game.left_skill):
					var cseq := Game.cast_skill(Game.left_skill, hit.entity_id)
					Log.debug("ui", "click cast", {"skill": Game.left_skill, "target": hit.entity_id, "name": hit.display_name, "seq": cseq})
				else:
					var aseq := Game.attack(hit.entity_id)
					Log.debug("ui", "click attack", {"target": hit.entity_id, "name": hit.display_name, "seq": aseq})
			else:
				var x := clampi(int(p.x), 0, _scene_w - 1)
				var y := clampi(int(p.y * 2.0), 0, _scene_h - 1)
				var seq := Game.move_to(x, y)
				Log.debug("ui", "click move", {"x": x, "y": y, "seq": seq})
		elif event.button_index == MOUSE_BUTTON_RIGHT:
			# the right mouse skill (m_nRightSkillID): on the entity under the cursor, else on the spot
			if Game.right_skill > 0 and Game.skills.has(Game.right_skill):
				var cseq := cast_skill_at_mouse(Game.right_skill)
				Log.debug("ui", "right click cast", {"skill": Game.right_skill, "seq": cseq})
		elif event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_set_zoom(_zoom * 1.15)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_set_zoom(_zoom / 1.15)
	elif event is InputEventKey and event.pressed:
		if event.keycode == KEY_ESCAPE:
			if _windows == null or not _windows.any_open():
				_leave()
		elif event.keycode == KEY_ENTER or event.keycode == KEY_KP_ENTER:
			if not _chat_input.has_focus():
				_chat_input.grab_focus()


func _set_zoom(z: float) -> void:
	_zoom = clampf(z, 0.25, 3.0)
	_camera.zoom = Vector2(_zoom, _zoom)


# A skill on the entity under the cursor, else on the spot (the right mouse skill; ShortcutUseItem of a quick cell
# holding a skill casts the same way, 0x005BC500 with the cursor position)
func cast_skill_at_mouse(skill_id: int) -> int:
	var p := get_global_mouse_position()
	var hit := _entity_at(p)
	if hit != null and hit.entity_id != Game.entity_id and hit.is_attackable():
		_select_target(hit)
		return Game.cast_skill(skill_id, hit.entity_id)
	return Game.cast_skill(skill_id, 0, clampi(int(p.x), 0, _scene_w - 1), clampi(int(p.y * 2.0), 0, _scene_h - 1))


# A click on a thing on the ground: picked up at once when near enough, else the character walks
# next to it and asks when it arrives (the old client did the same through KPlayer::PickUpItem).
func _pick_up(node: Node2D) -> void:
	var own := _own()
	if own == null:
		return
	_pending_pickup = 0
	if own.scene_pos.distance_to(node.scene_pos) <= PICK_UP_RANGE:
		Game.pick_up(node.entity_id)
		return
	_pending_pickup = node.entity_id
	var dir: Vector2 = (node.scene_pos - own.scene_pos).normalized()
	var stand: Vector2 = node.scene_pos - dir * 60.0
	Game.move_to(int(stand.x), int(stand.y))


func _walk_to_pickup() -> void:
	if _pending_pickup == 0:
		return
	var node: Node2D = _entities.get(_pending_pickup)
	var own := _own()
	if node == null or own == null:
		_pending_pickup = 0
		return
	if own.scene_pos.distance_to(node.scene_pos) <= PICK_UP_RANGE:
		Game.pick_up(node.entity_id)
		_pending_pickup = 0
	elif not own.is_moving():
		_pending_pickup = 0   # could not get there


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



# ---- the missiles (G2C_MISSLE): KMissle nodes in the entity layer, one per slot while it lives -----------------
var _missles := {}
var _missle_spawns := 0
var _missle_effects := 0


func _on_missle(d: Dictionary) -> void:
	var idx := int(d.get("index", 0))
	var node = _missles.get(idx)
	if bool(d.get("collided", false)):
		# KMissle::DoCollision: the collision movie (AnimFile4) at the spot; the missile flies on
		var row := Game.missle_row(int(d.get("missle_id", 0)))
		var anims: Array = row.get("anims", [])
		if anims.size() > 3 and anims[3] is Dictionary and str(anims[3].get("sprite", "")) != "":
			var fx = MissleEffectScript.new()
			_entity_layer.add_child(fx)
			fx.setup(anims[3], int(d.get("dir", 0)), Vector2(float(d.get("x", 0)), float(d.get("y", 0))), int(d.get("z", 0)))
			_missle_effects += 1
		if node != null:
			node.apply(d)
		return
	if node == null:
		if bool(d.get("removed", false)):
			return   # a missile this client never saw fly: nothing to end
		node = MissleScript.new()
		_entity_layer.add_child(node)
		_missles[idx] = node
		node.gone.connect(func(i: int): _missles.erase(i))
		node.setup(d, Game.missle_row(int(d.get("missle_id", 0))))
		_missle_spawns += 1
		return
	node.apply(d)


func _clear_missles() -> void:
	for idx in _missles.keys():
		var node = _missles[idx]
		if node != null and is_instance_valid(node):
			node.queue_free()
	_missles.clear()


# ---- world events --------------------------------------------------------------------------

func _add_entity(d: Dictionary) -> void:
	var id := int(d.id)
	var node: Node2D = _entities.get(id)
	if node == null:
		node = Node2D.new()
		node.set_script(ObjScript if int(d.get("type", 0)) == ENTITY_DROP else NpcScript)
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
		if node.entity_type != ENTITY_DROP:
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
	await _auto_items()
	await _auto_skills()
	await _auto_fight()
	await _auto_death()
	print("AUTO_MISSLE packets=%d spawned=%d effects=%d live=%d" % [Game.missle_packets, _missle_spawns, _missle_effects, _missles.size()])
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


# --auto: ask the zone for a sword, a potion and a level-5 sword with six magic levels through the
# GM chat (a development server has zone.gm_chat on), wait for G2C_ITEM_ADD, open the bag and take
# its picture, then the tooltip of the magic sword.  AUTO_ITEMS says how many the character carries
# afterwards: 0 when the zone has no item tables or gm_chat is off; AUTO_MAGIC how many prefixes /
# suffixes the level-5 sword rolled (Gen_MagicAttrib of the zone on the real tables).
func _auto_items() -> void:
	var before := Game.items.size()
	Game.chat("?gm ds AddItem(0,0,0,1,0,0)")
	Game.chat("?gm ds AddItem(1,0,0,1,0,0)")
	Game.chat("?gm ds AddItem(0,0,0,5,0,100,5,5,5,5,5,5)")
	var waited := 0.0
	while waited < 3.0 and Game.items.size() < before + 3:
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
	Log.info("auto", "auto items", {"before": before, "after": Game.items.size()})
	print("AUTO_ITEMS count=%d" % Game.items.size())
	var magic_sword: Dictionary = {}
	for item in Game.items.values():
		if int(item.get("genre", -1)) == 0 and int(item.get("level", 0)) == 5:
			magic_sword = item
	if not magic_sword.is_empty():
		var magic: Array = magic_sword.get("magic", [])
		Log.info("auto", "auto magic sword", {"name": str(magic_sword.get("name", "")), "magic": magic.size()})
		print("AUTO_MAGIC count=%d" % magic.size())
	# throw one on the ground and pick it up again: the ground objects end to end
	if Game.items.size() > 0:
		var had := Game.items.size()
		# the first thing lying in the bag that may be thrown: not a worn piece, not the task item a fresh
		# character starts with (KItemGenre::task 4 - the zone answers RESULT_BAD_REQUEST)
		var drop_id := 0
		for id in Game.items:
			if int(Game.items[id].room) == Game.ROOM_BAG and int(Game.items[id].genre) != 4:
				drop_id = int(id)
				break
		Game.item_drop(drop_id)
		var ground: Node2D = null
		waited = 0.0
		while waited < 3.0 and ground == null:
			await get_tree().create_timer(0.25).timeout
			waited += 0.25
			for node in _entities.values():
				if node.entity_type == ENTITY_DROP:
					ground = node
		var dropped := ground != null and Game.items.size() == had - 1
		if ground != null:
			await _save_screenshot("user://logs/auto_drop.png")
			_pick_up(ground)
			waited = 0.0
			while waited < 3.0 and Game.items.size() < had:
				await get_tree().create_timer(0.25).timeout
				waited += 0.25
		var picked := Game.items.size() == had
		Log.info("auto", "auto drop", {"dropped": dropped, "picked": picked, "ground": ground.display_name if ground else ""})
		print("AUTO_DROP dropped=%s picked=%s" % [dropped, picked])
	if _windows != null and _windows.ready_ok and Game.items.size() > 0:
		_windows.item_window.open_window()
		_windows.status_window.open_window()
		_windows.status_window._on_page_button(true, _windows.status_window.PAGE_EQUIP)
		await _save_screenshot("user://logs/auto_items.png")
		# the attribute page with the zone's numbers (G2C_PLAYER_ATTRIB): level, points, damage...
		_windows.status_window._on_page_button(true, _windows.status_window.PAGE_ATTRIB)
		var a: Dictionary = Game.player_attrib
		Log.info("auto", "auto attributes", {"level": a.get("level", 0), "strength": a.get("strength", 0), "attack_rating": a.get("attack_rating", 0),
			"life_max": a.get("life_max", 0), "points": a.get("attribute_point", 0), "exp": a.get("exp", 0), "next": a.get("next_level_exp", 0)})
		print("AUTO_ATTRIB level=%d strength=%d ar=%d life_max=%d points=%d" % [int(a.get("level", 0)), int(a.get("strength", 0)),
			int(a.get("attack_rating", 0)), int(a.get("life_max", 0)), int(a.get("attribute_point", 0))])
		await _save_screenshot("user://logs/auto_status.png")
		_windows.status_window.hide_window()
		if not magic_sword.is_empty():
			# the tooltip of the magic sword, as if the mouse rested on it in the bag
			_windows.hover.show_text(KUiItemView.describe_text(magic_sword) + "\n", Vector2(560, 120))
			await _save_screenshot("user://logs/auto_item_tip.png")
			_windows.hover.hide_lines()
		_windows.item_window.hide_window()


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


# --auto: "?gm ds KillPlayer()" makes the zone run the script function KillPlayer (0x08117BC0: an
# unblockable hit of 200 000 000 from oneself) - the character falls (ACTION_DEATH, the exp / money loss
# of KNpc::OnDeath), its picture is taken, then C2G_REVIVE stands it up at its revive point
# (KPlayer::Revive(0)) and another picture follows; AUTO_DEATH sums it up for tools/dev.py.
func _auto_death() -> void:
	var own := _own()
	if own == null:
		print("AUTO_DEATH none")
		return
	var life_before: int = own.life
	Game.chat("?gm ds KillPlayer()")
	var waited := 0.0
	while waited < 5.0 and is_instance_valid(own) and not own.is_dead():
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
	var dead: bool = is_instance_valid(own) and own.is_dead()
	await get_tree().create_timer(0.8).timeout   # the death animation runs to its last frame
	await _save_screenshot("user://logs/auto_death.png")
	Game.revive()
	waited = 0.0
	while waited < 5.0:
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
		own = _own()
		if own != null and not own.is_dead() and own.life > 0:
			break
	own = _own()
	var revived: bool = own != null and not own.is_dead() and own.life > 0
	Log.info("auto", "auto death", {"life_before": life_before, "dead": dead, "revived": revived,
		"life": own.life if own != null else 0, "doing": own.doing if own != null else -1})
	print("AUTO_DEATH dead=%s revived=%s life_before=%d life=%d" % [dead, revived, life_before, own.life if own != null else 0])
	await get_tree().create_timer(0.5).timeout
	await _save_screenshot("user://logs/auto_revive.png")


# --auto: the skill book (K) with what the character holds, its picture, then the first skill the book places
# (not the plain attacks) becomes the left mouse skill and is cast at the nearest monster; AUTO_SKILLS sums it up.
func _auto_skills() -> void:
	var placed := 0
	var pick := 0
	# a fresh character holds only the plain attacks and the common skills, which the book does not place: the
	# development server makes it a Shaolin disciple (SetFaction of the script api, like the faction npc's script) and
	# hands it every Shaolin skill at level 0 (AddMagic, the way faction_def.lua AddFacSkill does) - the three branch
	# pages of the book (Quyền / Bổng / Đao) then have their skills
	var before := Game.skills.size()
	# the fist skills ask for level 10 (ReqLevel): AddExp of the script api lifts the character there first - one
	# level per call (KPlayer::AddExp caps the gain at the next level's need), so nine calls from level 1
	# AddExp(exp, npcLevel): the level-difference rule of a kill shrinks the gain of a high character over a level-1 npc, so a
	# high npc level and a large sum - one level per call (capped at the next level's need), enough calls for level 20
	for _i in 22:
		Game.chat("?gm ds AddExp(2000000, 60)")
	# the faction of the character's own series (0x08060BB0 refuses any other): Shaolin for the metal characters --auto makes
	var own_series: int = int(Game.chars[0].series) if not Game.chars.is_empty() else 0
	var fac_name := "shaolin"
	var fac_id := 0
	var ftab = Assets.load_json("%s/faction.json" % Assets.assets_root())
	if ftab is Dictionary:
		for e in ftab.get("factions", []):
			if int(e.get("series", -1)) == own_series and str(e.get("name", "")) != "":
				fac_name = str(e.name)
				fac_id = int(e.get("index", 0))
				break
	# the skills come the way the faction npc's script hands them out (script/global/pgaming/npc/chuongmoncacphai/
	# thieulam.lua: SetFaction + AddMagic(10) on joining, then add_sl(stage) of script/global/skills_table.lua after
	# each rank quest: stage 10 = the entry skills, 20 = the level-10 quest, ... 90 = the high secrets): a level-10
	# character has done the first quest, so stage 20; nothing of the flat factionskill.txt list (that is the leave list)
	var add_functions := {"shaolin": "add_sl", "tianwang": "add_tw", "tangmen": "add_tm", "wudu": "add_wu", "emei": "add_em",
		"cuiyan": "add_cy", "tianren": "add_tr", "gaibang": "add_gb", "wudang": "add_wd", "kunlun": "add_kl", "huashan": "add_hs"}
	Game.chat("?gm ds SetFaction(\"%s\")" % fac_name)
	# the nine AddExp above land one at a time: level 10 first, then the stage of a disciple who did the level-10 quest
	var lifted := 0.0
	while lifted < 4.0 and int(Game.player_attrib.get("level", 1)) < 20:
		await get_tree().create_timer(0.25).timeout
		lifted += 0.25
	# the rank quests come every ten levels (thieulam.lua): stage 10 + 10 per ten levels, up to 60
	@warning_ignore("integer_division")
	var stage: int = clampi(10 + 10 * (int(Game.player_attrib.get("level", 1)) / 10), 10, 60)
	Game.chat("?gm ds Include(\"\\\\script\\\\global\\\\skills_table.lua\") %s(%d)" % [add_functions.get(fac_name, "add_sl"), stage])
	var waited := 0.0
	while waited < 4.0 and (Game.skills.size() < before + 1 or int(Game.player_attrib.get("level", 1)) < 10 or Game.faction_last != fac_id):
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
	if _windows != null and _windows.ready_ok:
		_windows.skills_window.open_window()
		await get_tree().create_timer(0.4).timeout
		for id in Game.skills:
			if int(id) <= 2:
				continue
			var branches: Array = _windows.skills_window.branches_of(int(id))
			if not branches.is_empty():
				placed += 1
				if pick == 0 and branches.has(0):
					pick = int(id)
		await _save_screenshot("user://logs/auto_skills.png")
		# the second branch page (Bổng Pháp for a Shaolin), then back
		if _windows.skills_window.branch_titles()[1] != "":
			_windows.skills_window._on_page_button(true, 1)
			await get_tree().create_timer(0.3).timeout
			await _save_screenshot("user://logs/auto_skills_2.png")
			_windows.skills_window._on_page_button(true, 0)
		_windows.skills_window.hide_window()
		# the mouse-skill tree of the left button (Open([[leftskill]]) of the bottom bar's box)
		if _windows.skill_tree != null:
			var tree = _windows.skill_tree
			tree.open_for(false)
			# ShortcutSkill(k) with the tree open (0x00495F70): the entry under the mouse takes key k - Q to the first listed
			# entry, W to the second (the tree lists skills of level 1..64 only, 0x006239F0: the faction skills of the fresh
			# disciple are level 0 until a point is spent, so they are not here yet)
			var tree_ids := []
			for e in tree._entries:
				tree_ids.append(int(e.id))
			var tree_key := 0
			for i in tree._entries.size():
				if i > 0 and int(tree._entries[i].id) > 0 and tree_key < 2:
					tree._hover = i
					_windows._shortcut_key(tree_key)
					tree_key += 1
			await get_tree().create_timer(0.3).timeout
			await _save_screenshot("user://logs/auto_skill_tree.png")
			print("AUTO_SKILL_TREE entries=%d rows=%d ids=%s key_q=%d key_w=%d mouse=%d/%d weapon=%d" % [tree._entries.size(), tree._rows, str(tree_ids), int(_windows.shortcuts.slot(0).id), int(_windows.shortcuts.slot(1).id), Game.left_skill, Game.right_skill, Game.weapon_attack_skill()])
			tree.hide()
		# the tip of the left mouse skill box (the mouse over ImediaLeftSkill -> ShowObjectTip): asked, then photographed
		if _windows.player_bar != null and Game.left_skill > 0:
			_windows.player_bar.mouse_skill_hovered.emit(false, {"id": Game.left_skill})
			await get_tree().create_timer(0.6).timeout
			_windows.player_bar.mouse_skill_hovered.emit(false, {"id": Game.left_skill})
			await get_tree().create_timer(0.2).timeout
			await _save_screenshot("user://logs/auto_bar_tip.png")
			print("AUTO_BAR_TIP skill=%d lines=%d" % [Game.left_skill, _windows.skill_tip_text(Game.left_skill).split("\n").size()])
			_windows.player_bar.mouse_skill_hovered.emit(false, null)
		# the quick slots: the first medicine of the bag goes to cell 0 (a drop from the cursor), key 1 uses it
		# (ShortcutUseItem(0) -> UseItem), the bar is photographed with the cell filled
		var quick_item := {}
		for id in Game.items:
			var it: Dictionary = Game.items[id]
			if int(it.room) == Game.ROOM_BAG and int(it.genre) == KUiItemView.GENRE_MEDICINE:
				quick_item = it
				break
		var quick_ok := false
		var quick_count := 0
		if not quick_item.is_empty():
			quick_count = int(quick_item.count)
			quick_ok = _windows.assign_quick(0, quick_item)
			await get_tree().create_timer(0.3).timeout
			await _save_screenshot("user://logs/auto_quick.png")
			_windows._quick_key(0)
			await get_tree().create_timer(0.6).timeout
		var after = Game.items.get(int(quick_item.get("id", 0)))
		print("AUTO_QUICK item=%d name=%s count=%d assigned=%s count_after=%d slot0=%d" % [int(quick_item.get("id", 0)), str(quick_item.get("name", "")), quick_count, quick_ok, int(after.count) if after != null else -1, int(_windows.quick.slot(0).id)])
		# a state on the character: Bất Động Minh Vương (15, stage 30) cast on oneself puts its icon in the state list (the
		# 0x87 packet); the fight stance first, like the gate trap does
		if Game.skills.has(15) and _windows.state_window != null:
			Game.chat("?gm ds SetFightState(1)")
			Game.add_skill_point(15)
			await get_tree().create_timer(0.4).timeout
			Game.cast_skill(15, Game.entity_id)
			var buffed := 0.0
			while buffed < 2.0 and not Game.states.has(15):
				await get_tree().create_timer(0.25).timeout
				buffed += 0.25
			await _save_screenshot("user://logs/auto_state.png")
			print("AUTO_STATE held=%d has15=%s" % [Game.states.size(), Game.states.has(15)])
	var cast_told := false
	if pick != 0:
		Game.left_skill = pick
		# the faction hands its skills out at level 0 (AddMagic): a point from the book's add button (C2G_ADD_SKILL_POINT)
		# makes the pick castable, the way a player does it
		Game.add_skill_point(pick)
		await get_tree().create_timer(0.5).timeout
		# the tip before the cast at the monster: it fights back, and a dead character shows no tip (npc3 kills a level 20 in a second)
		# the tip of the picked skill (KSkill::GetDesc 0x006FBC90): shown, the zone's numbers asked, shown again with them
		if _windows != null and pick > 0:
			_windows._show_skill_tip(pick)
			await get_tree().create_timer(0.6).timeout
			_windows._show_skill_tip(pick)
			await get_tree().create_timer(0.2).timeout
			await _save_screenshot("user://logs/auto_skill_tip.png")
			var tip_desc = Game.skill_desc(pick, int(Game.skills.get(pick, {}).get("level", 0)))
			var tip_text := _windows.skill_tip_text(pick)
			print("AUTO_SKILL_TIP skill=%d answered=%s cur_attribs=%d next=%s lines=%d" % [pick, tip_desc != null, (tip_desc.cur.attribs.size() if tip_desc != null and tip_desc.get("has_cur", false) else -1), (tip_desc.get("has_next", false) if tip_desc != null else false), tip_text.split("\n").size()])
			_windows._show_skill_tip(0)
		var own := _own()
		var best: Node2D = null
		var best_d := 400.0
		if own != null:
			for node in _entities.values():
				if node == own or not node.is_attackable():
					continue
				var d: float = node.scene_pos.distance_to(own.scene_pos)
				if d < best_d:
					best = node
					best_d = d
		if best != null:
			# the 2.0 client walks into the skill's reach before it sends the command: stand next to the target first
			var dir: Vector2 = (best.scene_pos - own.scene_pos).normalized()
			var stand: Vector2 = best.scene_pos - dir * 40.0
			Game.move_to(int(stand.x), int(stand.y))
			var walked := 0.0
			while walked < 6.0 and (own.is_moving() or own.scene_pos.distance_to(stand) > 24.0):
				await get_tree().create_timer(0.25).timeout
				walked += 0.25
			# a skill without PeaceCanUse needs the fight stance (KNpc+0x168c): the 2.0 client sets it with its own packet
			# before the first blow - the Godot client has no toggle yet, the script api stands in (B4b)
			Game.chat("?gm ds SetFightState(1)")
			await get_tree().create_timer(0.3).timeout
			var actions_before := _action_count
			_select_target(best)
			Game.cast_skill(pick, best.entity_id)
			await get_tree().create_timer(1.0).timeout
			cast_told = _action_count > actions_before
			await _save_screenshot("user://logs/auto_cast.png")
	var titles: Array = _windows.skills_window.branch_titles() if _windows != null and _windows.ready_ok else ["", "", ""]
	Log.info("auto", "auto skills", {"held": Game.skills.size(), "placed": placed, "pick": pick, "cast": cast_told, "faction": Game.faction_last, "branches": titles})
	print("AUTO_SKILLS held=%d placed=%d pick=%d cast=%s faction=%d branches=%s" % [Game.skills.size(), placed, pick, cast_told, Game.faction_last, "|".join(titles)])


# Saves the rendered frame (no-op in headless mode); used by tools/dev.py screenshot.
func _save_screenshot(path: String) -> void:
	if DisplayServer.get_name() == "headless":
		return
	await RenderingServer.frame_post_draw
	var img := get_viewport().get_texture().get_image()
	var err := img.save_png(path)
	Log.info("auto", "screenshot", {"path": ProjectSettings.globalize_path(path), "error": error_string(err)})
	print("AUTO_SCREENSHOT " + ProjectSettings.globalize_path(path))
