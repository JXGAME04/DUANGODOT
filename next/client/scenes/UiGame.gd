# The game screen: the world (drawn by a KWorldView - 2D sprites or 3D models, ADR-008), click to
# move, the 2.0 windows, chat box, debug HUD.  Every position here is in scene units of the zone;
# the view does the projecting.
extends Node2D

const NpcScript := preload("res://scenes/KNpc.gd")
const WorldView2D := preload("res://scenes/KWorldView2D.gd")
const KNpcGold := preload("res://scenes/KNpcGold.gd")
const ACTION_ATTACK := 1
const ENTITY_DROP := 4
const PICK_UP_RANGE := 180.0          # scene units: inside PLAYER_PICKUP_SERVER_DISTANCE (200) with a margin
const KLogin := preload("res://net/KLogin.gd")
const KUiGameWindows := preload("res://ui/KUiGameWindows.gd")
const KUiItemView := preload("res://ui/KUiItemView.gd")
const KUiSkillDesc := preload("res://ui/KUiSkillDesc.gd")

var _entities := {}          # entity_id -> Node (KNpc / KObj)
var _target: Node = null     # the entity the player attacks / selected
var _world: Node             # the KWorldView drawing the map, the entities, the missiles
var _hovered: Node = null    # the npc under the mouse ([core+0xa8c4] of the 2.0 client)
var _hud: Label
var _chat_log: RichTextLabel
var _chat_input: LineEdit
var _spawn_count := 0
var _move_count := 0
var _action_count := 0
var _scene_w := 8192
var _scene_h := 8192
var _windows: KUiGameWindows = null   # the bag, the character window, the tooltip, the item on the cursor
var _pending_pickup := 0              # the ground object the player walks to (0 = none)


func _ready() -> void:
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
	Game.state_icons_changed.connect(_on_state_icons)
	Game.gold_changed.connect(_on_gold)
	Game.entity_camp.connect(_on_entity_camp)
	Game.chat_msg.connect(_on_chat)
	Game.kicked.connect(_on_kicked)
	Game.connection_lost.connect(_on_connection_lost)
	for d in Game.entities.values():
		_add_entity(d)
	_update_camera(true)
	Log.info("ui", "world screen", {"zone": Game.zone_name, "entity": Game.entity_id, "entities": _entities.size(),
		"map": Game.map_id, "bundle": has_map, "view": _world.get_script().resource_path.get_file()})
	_append_chat("[color=gray]Vào %s. Click chuột trái để đi, Enter để chat, F4 túi đồ, F3 nhân vật, F5 kỹ năng, Q..C kỹ năng tắt, 1..9 ô nhanh, Esc để thoát.[/color]" % Game.zone_name)
	for a in OS.get_cmdline_user_args():
		if a.begins_with("--gm="):
			# a development shortcut: one GM script line on entering (zone.gm_chat), e.g. --gm=NewWorld(9053,232,194)
			Game.chat("?gm ds " + a.substr(5))
	if "--auto3d" in OS.get_cmdline_user_args():
		_auto3d_run()
	elif "--auto" in OS.get_cmdline_user_args():
		_auto_run()


# The world view: 3D when the map has a 3D bundle (client/assets3d/maps/<id>) or --3d asks for it, else the 2D one;
# made on entering and again when a map change crosses the 2D / 3D line (every entity is dropped before that)
func _make_world_view() -> Node:
	var want := Game.want_3d()
	if _world != null and _world.is_3d() == want:
		return _world
	if _world != null:
		_world.queue_free()
	var view := Node.new()
	view.set_script(load("res://scenes3d/KWorldView3D.gd") if want else WorldView2D)
	view.name = "WorldView3D" if want else "WorldView2D"
	add_child(view)
	move_child(view, 0)
	if view.get("right_click_handler") != null:
		# the 3D camera turns on a right drag; a plain right click still casts the right mouse skill
		view.right_click_handler = _on_right_click
	return view


# The right mouse skill (m_nRightSkillID): on the entity under the cursor, else on the spot
func _on_right_click(_screen: Vector2) -> void:
	if Game.right_skill > 0 and Game.skills.has(Game.right_skill):
		var cseq := cast_skill_at_mouse(Game.right_skill)
		Log.debug("ui", "right click cast", {"skill": Game.right_skill, "seq": cseq})


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


# Loads the world of Game.map_id through the view; called on entering the world and again after a ChangeMap.
func _setup_map() -> bool:
	_scene_w = Game.scene_w if Game.scene_w > 0 else 8192
	_scene_h = Game.scene_h if Game.scene_h > 0 else 8192
	_world = _make_world_view()
	var has_map: bool = _world.load_map()
	var snd = _world.sounds()
	if snd != null:
		snd.focus_provider = func() -> Vector2:
			var o := _own()
			return o.scene_pos if o != null else Vector2.ZERO
	return has_map


# G2C_CHANGE_MAP: drop every entity, load the new bundle; the zone's EntitySpawn (with ourselves)
# follows right after (KNpc::ChangeWorld -> SendSyncData of the old server).
func _on_map_changed(info: Dictionary) -> void:
	_select_target(null)
	for id in _entities.keys():
		var node: Node = _entities[id]
		_world.remove_entity(node)
		node.queue_free()
	_entities.clear()
	var has_map := _setup_map()
	_world.center_on(Vector2(float(info.get("x", 0)), float(info.get("y", 0))))
	_append_chat("[color=gray]Sang %s (map %d).[/color]" % [_world.map_name() if has_map else "map", Game.map_id])
	Log.info("ui", "map changed", {"map": Game.map_id, "bundle": has_map, "x": info.get("x", 0), "y": info.get("y", 0)})


func _own() -> Node:
	return _entities.get(Game.entity_id)


func _update_camera(snap: bool, delta: float = 0.0) -> void:
	var own := _own()
	if own:
		_world.follow(own, snap, delta)


func _process(delta: float) -> void:
	_update_camera(false, delta)
	_walk_to_pickup()
	_world.update(delta)
	var own := _own()
	var st: Dictionary = Assets.stats()
	var target_text := ""
	if _target != null and is_instance_valid(_target):
		target_text = "  target %s %d/%d" % [_target.display_name, _target.life, _target.life_max]
	_hud.text = "%s  zone %d  map %d  entity %d  sid %d (%s)\npos %s  hp %d/%d%s\nentities %d  regions %d  sprites %d (%d MB)  rtt %d ms  fps %d" % [
		Game.zone_name, Game.zone_id, Game.map_id, Game.entity_id, Game.sid, Net.transport,
		str(Vector2i(own.scene_pos)) if own else "-", own.life if own else 0, own.life_max if own else 0, target_text,
		_entities.size(), _world.region_count(), st.sprites, st.mb, Game.last_rtt_ms, Engine.get_frames_per_second()]


# the cursor in viewport pixels (the views project it)
func _mouse() -> Vector2:
	return get_viewport().get_mouse_position()


# a scene point clamped to the map
func _clamp_scene(p: Vector2) -> Vector2i:
	return Vector2i(clampi(int(p.x), 0, _scene_w - 1), clampi(int(p.y), 0, _scene_h - 1))


func _unhandled_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_LEFT:
			var p := _mouse()
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
				var s := _clamp_scene(_world.screen_to_scene(p))
				var seq := Game.move_to(s.x, s.y)
				Log.debug("ui", "click move", {"x": s.x, "y": s.y, "seq": seq})
		elif event.button_index == MOUSE_BUTTON_RIGHT:
			_on_right_click(_mouse())
		elif event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_world.zoom_step(1)
		elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_world.zoom_step(-1)
	elif event is InputEventMouseMotion:
		_set_hovered(_entity_at(_mouse()))
	elif event is InputEventKey and event.pressed:
		if event.keycode == KEY_ESCAPE:
			if _windows == null or not _windows.any_open():
				_leave()
		elif event.keycode == KEY_ENTER or event.keycode == KEY_KP_ENTER:
			if not _chat_input.has_focus():
				_chat_input.grab_focus()
		elif event.keycode == KEY_F7 and not event.echo:
			# autoexec.lua: AddCommand("F7", "", "Switch([[showplayername]])") -> 0x0042FBF7
			set_show_switches(KNpcGold.toggle_switch(NpcScript.name_switch), NpcScript.life_switch)
		elif event.keycode == KEY_F8 and not event.echo:
			# AddCommand("F8", "", "Switch([[showplayerlife]])") -> 0x0042FC2D
			set_show_switches(NpcScript.name_switch, KNpcGold.toggle_switch(NpcScript.life_switch))


# the npc under the mouse (the pate loop 0x0067021A compares [core+0xa8c4], the hovered npc, with each one)
func _set_hovered(node: Node) -> void:
	if node == _hovered:
		return
	if _hovered != null and is_instance_valid(_hovered) and _hovered.has_method("set_hovered"):
		_hovered.set_hovered(false)
	_hovered = node if node != null and node.has_method("set_hovered") else null
	if _hovered != null:
		_hovered.set_hovered(true)


# the two show switches of the option word (KNpcGold.gd): every npc draws its block again
func set_show_switches(name_switch: int, life_switch: int) -> void:
	NpcScript.name_switch = name_switch
	NpcScript.life_switch = life_switch
	for node in _entities.values():
		if node != null and is_instance_valid(node) and node.has_method("refresh_info"):
			node.refresh_info()
	Log.info("ui", "show switches", {"names": name_switch, "life": life_switch})


# A skill on the entity under the cursor, else on the spot (the right mouse skill; ShortcutUseItem of a quick cell
# holding a skill casts the same way, 0x005BC500 with the cursor position)
func cast_skill_at_mouse(skill_id: int) -> int:
	var p := _mouse()
	var hit := _entity_at(p)
	if hit != null and hit.entity_id != Game.entity_id and hit.is_attackable():
		_select_target(hit)
		return Game.cast_skill(skill_id, hit.entity_id)
	var s := _clamp_scene(_world.screen_to_scene(p))
	return Game.cast_skill(skill_id, 0, s.x, s.y)


# A click on a thing on the ground: picked up at once when near enough, else the character walks
# next to it and asks when it arrives (the old client did the same through KPlayer::PickUpItem).
func _pick_up(node: Node) -> void:
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
	var node: Node = _entities.get(_pending_pickup)
	var own := _own()
	if node == null or own == null:
		_pending_pickup = 0
		return
	if own.scene_pos.distance_to(node.scene_pos) <= PICK_UP_RANGE:
		Game.pick_up(node.entity_id)
		_pending_pickup = 0
	elif not own.is_moving():
		_pending_pickup = 0   # could not get there


# The entity drawn under a viewport point (the one on top wins).
func _entity_at(screen: Vector2) -> Node:
	return _world.pick(screen, _entities)


func _select_target(node: Node) -> void:
	if _target != null and is_instance_valid(_target) and _target != node:
		_target.set_target(false)
	_target = node
	if node != null:
		node.set_target(true)


func _leave() -> void:
	Game.leave_world()
	get_tree().change_scene_to_file("res://scenes/UiShell.tscn")



# ---- the missiles (G2C_MISSLE): KMissle nodes in the world, one per slot while it lives -----------------------
var _missles := {}
var _missle_spawns := 0
var _missle_effects := 0
var _missle_smooth_max := 0      # the most render-frame moves a missile made inside one logic frame (rule 13)


func _on_missle(d: Dictionary) -> void:
	var idx := int(d.get("index", 0))
	var node = _missles.get(idx)
	if bool(d.get("collided", false)):
		# KMissle::DoCollision: the collision movie (AnimFile4) at the spot; the missile flies on
		var row := Game.missle_row(int(d.get("missle_id", 0)))
		var anims: Array = row.get("anims", [])
		if anims.size() > 3 and anims[3] is Dictionary and str(anims[3].get("sprite", "")) != "":
			_world.add_missle_effect(anims[3], int(d.get("dir", 0)), Vector2(float(d.get("x", 0)), float(d.get("y", 0))), int(d.get("z", 0)))
			_missle_effects += 1
			# CreateSpecialEffect 0x006B1DC0: the status' sound (SndFile4) after the movie is added, only with a movie,
			# and not while the same file still plays (KMissleRes::PlaySound 0x00717ED0)
			var snd = _world.sounds()
			if snd != null:
				snd.play(str(anims[3].get("sound", "")), Vector2(float(d.get("x", 0)), float(d.get("y", 0))), false, true)
		if node != null:
			node.apply(d)
		return
	if node != null and is_instance_valid(node):
		_missle_smooth_max = maxi(_missle_smooth_max, node.max_frame_moves)
	if node == null:
		if bool(d.get("removed", false)):
			return   # a missile this client never saw fly: nothing to end
		node = _world.add_missle(d, Game.missle_row(int(d.get("missle_id", 0))))
		_missles[idx] = node
		node.gone.connect(func(i: int): _missles.erase(i))
		_missle_spawns += 1
		return
	node.apply(d)


# --auto: where the drawn missiles' frames sit against the character (the frame's centre, screen px; 0,0 = on the character)
func _missle_shot_info() -> String:
	var own := _own()
	var parts := []
	for node in _missles.values():
		if node == null or not is_instance_valid(node) or not node.is_drawn():
			continue
		var r: Rect2 = node.drawn_rect()
		var c: Vector2 = r.get_center() - (own.position if own != null else Vector2.ZERO)
		parts.append("m%d:%s@(%d,%d)%dx%d" % [node.missle_id, node.status, int(c.x), int(c.y), int(r.size.x), int(r.size.y)])
	return " ".join(parts)


# --auto: the most render-frame moves any live missile made within one logic frame (rule 13: > 1 at 144 fps)
func _missle_smooth() -> int:
	for node in _missles.values():
		if node != null and is_instance_valid(node):
			_missle_smooth_max = maxi(_missle_smooth_max, node.max_frame_moves)
	return _missle_smooth_max


# --auto: is any missile showing a frame right now (its AnimFile2 while it flies, AnimFile3 while it vanishes)?
func _missle_drawn() -> bool:
	for node in _missles.values():
		if node != null and is_instance_valid(node) and node.is_drawn():
			return true
	return false


func _clear_missles() -> void:
	for idx in _missles.keys():
		var node = _missles[idx]
		if node != null and is_instance_valid(node):
			node.queue_free()
	_missles.clear()


# ---- world events --------------------------------------------------------------------------

func _add_entity(d: Dictionary) -> void:
	var id := int(d.id)
	_entities[id] = _world.add_entity(d, id == Game.entity_id, _entities.get(id))
	_spawn_count += 1


func _on_spawn(list: Array) -> void:
	for d in list:
		_add_entity(d)


func _on_despawn(ids: Array) -> void:
	for id in ids:
		var node: Node = _entities.get(int(id))
		if node:
			if node == _target:
				_target = null
			_world.remove_entity(node)
			node.queue_free()
			_entities.erase(int(id))


func _on_move(mv: Dictionary) -> void:
	var node: Node = _entities.get(int(mv.id))
	if node:
		node.apply_move(mv)
		if node.entity_type != ENTITY_DROP:
			_move_count += 1
	else:
		Log.trace("world", "move for unknown entity", {"id": mv.id})


func _on_action(a: Dictionary) -> void:
	_action_count += 1
	var node: Node = _entities.get(int(a.id))
	if node:
		node.apply_action(a)
		if node == _target and node.is_dead():
			_select_target(null)
		# KNpc::DoSkill of the 2.0 client (0x005EF90F for a synced cast, 0x005F1E26 for the local player's): a player's
		# cast plays the skill's ManCastSnd / FMCastSnd by sex at the caster (KSkill::PlayCastSound 0x006F6D90); npcs
		# have their own sounds (KNpcRes::PlaySound, B4f-2)
		var snd = _world.sounds()
		if int(a.action) == ACTION_ATTACK and int(a.get("skill", 0)) > 0 and node.has_method("apply_action") \
				and node.entity_type == NpcScript.ENTITY_PLAYER and snd != null:
			snd.play(cast_sound(int(a.skill), node.sex), node.scene_pos)


# the cast sound of a skill for a sex (0 male ManCastSnd, otherwise FMCastSnd), "" when the row has none
static func cast_sound(skill_id: int, sex: int) -> String:
	var cells: Dictionary = Game.skill_row(skill_id)   # the cells of skills.json
	var s := str(cells.get("FMCastSnd" if sex != 0 else "ManCastSnd", "")).strip_edges()
	return "" if s == "0" else s


func _on_life(l: Dictionary) -> void:
	var node: Node = _entities.get(int(l.id))
	if node:
		node.set_life(l)


# G2C_STATE_ICONS (the 0x7a packet): the pictures of the states an entity holds
func _on_state_icons(entity_id: int) -> void:
	var node: Node = _entities.get(entity_id)
	var d = Game.entities.get(entity_id)
	if node != null and d != null and node.has_method("set_state_icons"):
		node.set_state_icons(d.get("state_icons", []))


# the 0x59 / 0x58 packets: a npc's camps - a player's name colour follows its current camp (0x005F2507)
func _on_entity_camp(c: Dictionary) -> void:
	var node: Node2D = _entities.get(int(c.id))
	if node != null and node.has_method("set_camp"):
		node.set_camp(int(c.camp), int(c.current_camp))


# the 0x9a packet: a monster turned gold - its name takes the gold colour (0x005F23E5)
func _on_gold(entity_id: int) -> void:
	var node: Node2D = _entities.get(entity_id)
	var d = Game.entities.get(entity_id)
	if node != null and d != null and node.has_method("set_gold_type"):
		node.set_gold_type(int(d.get("gold_type", 0)))


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
		"rtt_ms": Game.last_rtt_ms, "regions": _world.region_count(), "sprites": Assets.stats().sprites})
	print("AUTO_RESULT arrived=%s entities=%d moves=%d regions=%d transport=%s" % [arrived, _entities.size(), _move_count, _world.region_count(), Net.transport])
	await _save_screenshot("user://logs/auto_world.png")
	await _auto_items()
	await _auto_skills()
	await _auto_fight()
	await _auto_death()
	var _sounds = _world.sounds()
	print("AUTO_MISSLE packets=%d spawned=%d effects=%d live=%d sounds=%d dropped=%d files=%d smooth=%d fps=%d" % [Game.missle_packets, _missle_spawns, _missle_effects, _missles.size(),
		_sounds.played if _sounds != null else 0, _sounds.dropped if _sounds != null else 0, _sounds.get_child_count() if _sounds != null else 0, _missle_smooth(), int(Engine.get_frames_per_second())])
	print("AUTO_SOUNDS %s" % str(_sounds.history if _sounds != null else []))
	# the state pictures every npc around carries (B4d-2: a template's aura in cell 5 casts its child every ten frames)
	var npc_states: PackedStringArray = []
	for node in _entities.values():
		if node != null and is_instance_valid(node) and node.has_method("state_spr_info") and not node.is_own:
			var icons: Array = node.state_icons
			if not icons.is_empty() and icons.any(func(v): return int(v) != 0):
				npc_states.append("%s=%s pics=%s" % [node.display_name, str(icons), _state_pics_text(node.state_spr_info())])
	print("AUTO_NPC_STATES %s" % ", ".join(npc_states))
	# the gold monsters around (B5a: the kind of the 0x4c / 0x9a packets, the name colour of 0x005F23E5)
	var golds: PackedStringArray = []
	for node in _entities.values():
		if node != null and is_instance_valid(node) and "gold_type" in node and int(node.gold_type) != 0:
			var row: Dictionary = NpcResList.gold_row(int(node.gold_type))
			golds.append("%s kind=%d row=%s class=%d color=%s life=%d/%d" % [node.display_name, int(node.gold_type), str(row.get("name", "?")), node.npc_class(),
				node.name_color().to_html(false), node.life, node.life_max])
	if not golds.is_empty():
		await _save_screenshot("user://logs/auto_gold.png")
	print("AUTO_GOLD rows=%d %s" % [NpcResList.gold_rows(), ", ".join(golds)])
	# stability probe: two frames half a second apart while idle must be (almost) identical
	if DisplayServer.get_name() != "headless":
		await get_tree().create_timer(1.0).timeout
		var cam_a: String = _world.camera_state()
		await RenderingServer.frame_post_draw
		var img_a := get_viewport().get_texture().get_image()
		await get_tree().create_timer(0.5).timeout
		var cam_b: String = _world.camera_state()
		await RenderingServer.frame_post_draw
		var img_b := get_viewport().get_texture().get_image()
		var diff := 0
		var total := 0
		for y in range(0, img_a.get_height(), 4):
			for x in range(0, img_a.get_width(), 4):
				total += 1
				if img_a.get_pixel(x, y) != img_b.get_pixel(x, y):
					diff += 1
		Log.info("auto", "idle frame diff", {"diff": diff, "sampled": total, "cam_a": cam_a, "cam_b": cam_b, "fps": Engine.get_frames_per_second(),
			"anims": _world.anim_count(), "regions": _world.region_count()})
		print("AUTO_IDLE_DIFF diff=%d of %d cam_a=%s cam_b=%s anims=%d" % [diff, total, cam_a, cam_b, _world.anim_count()])
		img_a.save_png("user://logs/auto_world_a.png")
		img_b.save_png("user://logs/auto_world_b.png")
	Game.leave_world()
	await get_tree().create_timer(0.3).timeout
	get_tree().quit(0 if arrived else 1)


# --auto3d (with --gm=NewWorld(9053,232,194) or a 3D map): waits for the 3D view, takes the world from three camera
# angles, walks, fights the nearest monster, and prints AUTO3D_OK - the proof of M3D-1 (docs/LO-TRINH-3D.md).
func _auto3d_run() -> void:
	await get_tree().create_timer(1.0).timeout
	var waited := 0.0
	while waited < 10.0 and not (_world.is_3d() and _own() != null):
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
	var own := _own()
	print("AUTO3D view=%s map=%d entities=%d own=%s" % [_world.name, Game.map_id, _entities.size(), str(own.scene_pos) if own else "-"])
	if not _world.is_3d():
		print("AUTO3D_FAIL no 3D view")
		get_tree().quit(1)
		return
	await get_tree().create_timer(1.5).timeout
	var n := 0
	for yaw in [0.0, 90.0, 200.0]:
		_world.cam_rig.yaw = yaw
		for i in 4:
			await get_tree().process_frame
		await _save_screenshot("user://logs/auto3d_%d.png" % n)
		n += 1
	_world.cam_rig.yaw = 0.0
	# a walk of 200 units east (4 m), like --auto
	var from: Vector2 = own.scene_pos
	var seq := Game.move_to(int(from.x) + 200, int(from.y))
	waited = 0.0
	var arrived := false
	while waited < 10.0 and not arrived:
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
		own = _own()
		arrived = own != null and _move_count > 0 and not own.is_moving() and own.scene_pos.distance_to(from) > 8.0
	await _save_screenshot("user://logs/auto3d_%d.png" % n)
	n += 1
	print("AUTO3D_MOVE seq=%d arrived=%s from=%s to=%s moves=%d" % [seq, arrived, str(from), str(own.scene_pos) if own else "-", _move_count])
	# a fresh character holds only skills 1 / 2: the plain attack 53 (weapon_skill.json: bare hands / swords) is handed out and
	# given a point the way the faction scripts do (AddMagic), levels for the sword's requirements (AddExp, one level a call)
	Game.chat("?gm ds AddMagic(53, 10)")
	for _i in 20:
		Game.chat("?gm ds AddExp(2000000, 60)")
	waited = 0.0
	while waited < 3.0 and not Game.skills.has(53):
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
	if Game.skills.has(53) and int(Game.skills[53].get("level", 0)) <= 0:
		Game.add_skill_point(53)
		waited = 0.0
		while waited < 2.0 and int(Game.skills[53].get("level", 0)) <= 0:
			await get_tree().create_timer(0.25).timeout
			waited += 0.25
	var results: Array = []
	Game.item_result.connect(func(seq: int, result: int): results.append([seq, result]))
	# a sword in hand (AddItem genre 0 detail 0 particular 0 level 1 = Thiết Trủy thủ: strength 20), worn: the 3D weapon follows the item
	var before := Game.items.size()
	Game.chat("?gm ds AddItem(0,0,0,1,0,0)")
	waited = 0.0
	while waited < 3.0 and Game.items.size() < before + 1:
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
	var sword := 0
	for id in Game.items:
		if int(Game.items[id].genre) == 0 and int(Game.items[id].detail) == 0 and int(Game.items[id].room) == Game.ROOM_BAG:
			sword = int(id)
	if sword != 0:
		Game.item_equip(sword, 3)   # ITEMPART_WEAPON; part -1 (the zone picks) is a negative int32 the Godot proto lib does not encode
		waited = 0.0
		while waited < 3.0 and Game.item_worn(3) != sword:
			await get_tree().create_timer(0.25).timeout
			waited += 0.25
	await get_tree().create_timer(0.4).timeout
	_world.cam_rig.yaw = 30.0
	_world.cam_rig.pitch = 45.0
	_world.cam_rig.dist = 10.0
	for i in 4:
		await get_tree().process_frame
	await _save_screenshot("user://logs/auto3d_%d.png" % n)
	n += 1
	print("AUTO3D_WEAPON item=%d worn=%d weapon=%s skill53=%s skill53_level=%d level=%d item_results=%s item=%s" % [sword, Game.item_worn(3), _world._own_weapon, Game.skills.has(53),
		int(Game.skills.get(53, {}).get("level", -1)), int(Game.player_attrib.get("level", 0)), str(results), "room %s x %s y %s name %s" % [str(Game.items.get(sword, {}).get("room")), str(Game.items.get(sword, {}).get("x")), str(Game.items.get(sword, {}).get("y")), str(Game.items.get(sword, {}).get("name"))]])
	Game.chat("?gm ds SetFightState(1)")
	await _auto_fight()
	await _save_screenshot("user://logs/auto3d_%d.png" % n)
	n += 1
	# a trap (the reference map's EnterPoint_wld became one, make_map3d.py): stand next to it, walk in, the zone's NewWorld
	# takes us to Phuong Tuong - a 2D map - and the view swaps back to 2D (KNpc::ChangeWorld -> G2C_CHANGE_MAP)
	var map_before := Game.map_id
	Game.chat("?gm ds SetPos(52, 77)")
	await get_tree().create_timer(0.8).timeout
	Game.move_to(47 * 32 + 16, 77 * 32 + 16)
	waited = 0.0
	while waited < 12.0 and Game.map_id == map_before:
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
	await get_tree().create_timer(1.5).timeout
	await _save_screenshot("user://logs/auto3d_%d.png" % n)
	n += 1
	print("AUTO3D_TRAP from=%d to=%d view3d=%s entities=%d" % [map_before, Game.map_id, _world.is_3d(), _entities.size()])
	var models := 0
	var markers := 0
	var views = _world.get("_views")   # the 3D view's table; the 2D view (after the trap) has none
	for node in _entities.values():
		var v = views.get(node) if views is Dictionary else null
		if v != null and v.model != null:
			models += 1
		else:
			markers += 1
	print("AUTO3D_OK map=%d entities=%d models=%d markers=%d fps=%d camera=%s" % [Game.map_id, _entities.size(), models, markers, Engine.get_frames_per_second(), _world.camera_state()])
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
		var ground: Node = null
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
	var best: Node = null
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
	if not is_instance_valid(best):
		print("AUTO_FIGHT none (the target left while walking)")
		return
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
			print("AUTO_SKILL_TIP skill=%d answered=%s cur_attribs=%d next=%s lines=%d held=%d inc=%d enhance=%d related=%d" % [pick, tip_desc != null, (tip_desc.cur.attribs.size() if tip_desc != null and tip_desc.get("has_cur", false) else -1), (tip_desc.get("has_next", false) if tip_desc != null else false), tip_text.split("\n").size(),
				(int(tip_desc.get("held_level", 0)) if tip_desc != null else -1), (int(tip_desc.get("level_inc", 0)) if tip_desc != null else -1), (int(tip_desc.get("enhance", 0)) if tip_desc != null else -1),
				(tip_desc.cur.get("related", []).size() if tip_desc != null and tip_desc.get("has_cur", false) else -1)])
			_windows._show_skill_tip(0)
		var own := _own()
		var best: Node = null
		var best_d := 400.0
		if own != null:
			for node in _entities.values():
				if node == own or not node.is_attackable():
					continue
				var d: float = node.scene_pos.distance_to(own.scene_pos)
				if d < best_d:
					best = node
					best_d = d
		if own != null and pick > 0:
			# the cast movie in the open (the owner: "move to an empty area so it shows"): up the street from the spawn point,
			# away from the roof whose sort hides a frame; the skill at the character's own spot needs the fight stance
			# (PeaceCanUse 0); the shot when a missile draws
			var open_spot: Vector2 = own.scene_pos + Vector2(-90, -190)
			Game.move_to(int(open_spot.x), int(open_spot.y))
			var walked_open := 0.0
			while walked_open < 4.0 and (own.is_moving() or own.scene_pos.distance_to(open_spot) > 24.0):
				await get_tree().create_timer(0.25).timeout
				walked_open += 0.25
			await _auto_aura()
			Game.chat("?gm ds SetFightState(1)")
			await get_tree().create_timer(0.3).timeout
			Game.cast_skill(pick, 0, int(own.scene_pos.x), int(own.scene_pos.y))
			var open_wait := 0.0
			while open_wait < 1.5 and not _missle_drawn():
				await get_tree().create_timer(0.05).timeout
				open_wait += 0.05
			await _save_screenshot("user://logs/auto_cast_open.png")
			print("AUTO_CAST_OPEN after=%.2f drawn=%s %s" % [open_wait, _missle_drawn(), _missle_shot_info()])
			await get_tree().create_timer(maxf(1.2 - open_wait, 0.1)).timeout
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
			# the shot while the cast movie plays: the request queues a command (up to 18 frames), the missile waits the skill's
			# WaitTime unseen, then its AnimFile2 shows - wait for the first drawn missile (1.5 s at most), shoot, then wait the rest
			var shown := 0.0
			while shown < 1.5 and not _missle_drawn():
				await get_tree().create_timer(0.05).timeout
				shown += 0.05
			await _save_screenshot("user://logs/auto_cast.png")
			print("AUTO_CAST_SHOT after=%.2f drawn=%s %s sounds=%d smooth=%d" % [shown, _missle_drawn(), _missle_shot_info(), _world.sounds().played if _world.sounds() != null else 0, _missle_smooth()])
			await get_tree().create_timer(maxf(1.0 - shown, 0.1)).timeout
			cast_told = _action_count > actions_before
	var titles: Array = _windows.skills_window.branch_titles() if _windows != null and _windows.ready_ok else ["", "", ""]
	Log.info("auto", "auto skills", {"held": Game.skills.size(), "placed": placed, "pick": pick, "cast": cast_told, "faction": Game.faction_last, "branches": titles})
	print("AUTO_SKILLS held=%d placed=%d pick=%d cast=%s faction=%d branches=%s" % [Game.skills.size(), placed, pick, cast_told, Game.faction_last, "|".join(titles)])


# --auto: the aura, switched on where the character stands (an open spot: the ring of La Hán Trận shows)
func _auto_aura() -> void:
	# an aura held (IsAura, LRSkill 2 - La Hán Trận 16 for a Shaolin): put on the right button it is switched on at the zone
	# (KNpc::SetAura 0x005EA870 -> the 0x6f packet); every ten frames the zone casts its child at the character (0x080873B0)
	# and its StateSpecialId icon comes back in the 0x7a packet (G2C_STATE_ICONS)
	var aura_id := _held_aura()
	if aura_id == 0 and Game.faction_last == 0:
		# the first stages of add_sl hand out no aura: La Hán Trận (16) from the GM chat, the way the faction script would later
		Game.chat("?gm ds AddMagic(16)")
		var given := 0.0
		while given < 2.0 and not Game.skills.has(16):
			await get_tree().create_timer(0.25).timeout
			given += 0.25
		aura_id = _held_aura()
	if aura_id > 0 and _windows != null:
		# La Hán Trận asks for character level 30 (ReqLevel): the levels still missing through AddExp, one a call
		var need := KUiSkillDesc._cell_int(Game.skill_row(aura_id), "ReqLevel", 0) - int(Game.player_attrib.get("level", 1))
		for _i in maxi(need, 0):
			Game.chat("?gm ds AddExp(2000000, 60)")
		if need > 0:
			await get_tree().create_timer(0.6).timeout
		if int(Game.skills[aura_id].get("level", 0)) <= 0:
			Game.add_skill_point(aura_id)   # handed out at level 0 (AddMagic): a point first, SetAura wants a level 1..63
			var pointed := 0.0
			while pointed < 2.0 and int(Game.skills[aura_id].get("level", 0)) <= 0:
				await get_tree().create_timer(0.25).timeout
				pointed += 0.25
		var packets_before: int = Game.missle_packets
		var spawned_before := _missle_spawns
		var child := KUiSkillDesc._cell_int(Game.skill_row(aura_id), "ChildSkillId", 0)
		_windows._on_skill_clicked(aura_id, true)
		# the shot while the aura's child movie plays (the ring of La Hán Trận): wait for a drawn missile, at most 1.6 s
		var aura_wait := 0.0
		while aura_wait < 1.6 and not _missle_drawn():
			await get_tree().create_timer(0.05).timeout
			aura_wait += 0.05
		await _save_screenshot("user://logs/auto_aura.png")
		await get_tree().create_timer(maxf(1.6 - aura_wait, 0.1)).timeout
		var own_d = Game.entities.get(Game.entity_id)
		var icons: Array = own_d.get("state_icons", []) if own_d != null else []
		var special := KUiSkillDesc._cell_int(Game.skill_row(aura_id), "StateSpecialId", 0)
		# the state pictures on the body (B4e): the aura's StateSpecialId ring at the feet and the child's state
		var own_node: Node = _entities.get(Game.entity_id)
		var pics: Array = own_node.state_spr_info() if own_node != null and own_node.has_method("state_spr_info") else []
		await _save_screenshot("user://logs/auto_aura_state.png")
		print("AUTO_AURA skill=%d child=%d packets=%d spawned=%d icons=%s icon_ok=%s child_state=%s pictures=%s" % [aura_id, child, Game.missle_packets - packets_before, _missle_spawns - spawned_before, str(icons), icons.has(special), Game.states.has(child), _state_pics_text(pics)])
		_windows._on_skill_clicked(0, true)   # off again: SetRightSkill of a non-aura clears it (0x005EA8B4)
		await get_tree().create_timer(0.2).timeout
	else:
		print("AUTO_AURA skill=0 (no aura held) skills=%s" % str(Game.skills.keys()))


# --auto: the state pictures of KNpc.state_spr_info() as "id:type@x,y wxh behind" words
func _state_pics_text(pics: Array) -> String:
	var words: PackedStringArray = []
	for p in pics:
		var r: Rect2 = p.rect
		words.append("%d:%d@%d,%d %dx%d%s" % [int(p.id), int(p.type), int(r.position.x), int(r.position.y), int(r.size.x), int(r.size.y), " behind" if bool(p.behind) else ""])
	return "[%s]" % ",".join(words)


# --auto: the first aura (IsAura) among the skills held, 0 = none
func _held_aura() -> int:
	for sid in Game.skills:
		if KUiSkillDesc._cell_int(Game.skill_row(int(sid)), "IsAura", 0) != 0:
			return int(sid)
	return 0


# Saves the rendered frame (no-op in headless mode); used by tools/dev.py screenshot.
func _save_screenshot(path: String) -> void:
	if DisplayServer.get_name() == "headless":
		return
	await RenderingServer.frame_post_draw
	var img := get_viewport().get_texture().get_image()
	var err := img.save_png(path)
	Log.info("auto", "screenshot", {"path": ProjectSettings.globalize_path(path), "error": error_string(err)})
	print("AUTO_SCREENSHOT " + ProjectSettings.globalize_path(path))
