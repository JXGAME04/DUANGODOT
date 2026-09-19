# The game screen: the world (drawn by a KWorldView - 2D sprites or 3D models, ADR-008), click to
# move, the 2.0 windows, chat box, debug HUD.  Every position here is in scene units of the zone;
# the view does the projecting.
extends Node2D

const NpcScript := preload("res://scenes/KNpc.gd")
const WorldView2D := preload("res://scenes/KWorldView2D.gd")
const KNpcGold := preload("res://scenes/KNpcGold.gd")
const ACTION_ATTACK := 1
const ENTITY_DROP := 4
const ENTITY_PLAYER_KIND := 1
const PICK_UP_RANGE := 180.0          # scene units: inside PLAYER_PICKUP_SERVER_DISTANCE (200) with a margin
const KLogin := preload("res://net/KLogin.gd")
const KUiGameWindows := preload("res://ui/KUiGameWindows.gd")
const Proto := preload("res://proto/jx_pb.gd")
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
	_windows.system_line.connect(func(text: String): _append_chat("[color=#e6be00]%s[/color]" % text))
	_windows.name = "Windows"
	add_child(_windows)
	Game.team_changed.connect(_on_team_changed)
	Game.entity_menu_state.connect(_on_entity_menu_state)
	# the 2.0 bottom bar carries the chat line ([InputEdit] of 玩家信息主界面.ini): the plain one steps aside
	if _windows.player_bar != null and _windows.player_bar.chat_input != null:
		_chat_input.visible = false
		_chat_input = _windows.player_bar.chat_input
		_windows.player_bar.chat_submitted.connect(_on_chat_submitted)
	_bind_minimap()
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
	Game.entity_res.connect(_on_entity_res)
	Game.entity_ride.connect(_on_entity_ride)
	Game.entity_pk.connect(_on_entity_pk)
	Game.pk_changed.connect(_on_pk_changed)
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
	_bind_minimap()


func _own() -> Node:
	return _entities.get(Game.entity_id)


# The minimap follows the scene: its picture per map, the entity table, our own node once it exists
func _bind_minimap() -> void:
	if _windows == null or _windows.minimap == null:
		return
	_windows.minimap.bind(_entities)
	_windows.minimap.set_map(Game.map_id, _world.map_name(), _world.map_info())
	_windows.minimap.set_own(_own())


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
	_hud.text = "%s  zone %d  map %d  entity %d  sid %d (%s)\npos %s  hp %d/%d%s  pk %s/%d\nentities %d  regions %d  sprites %d (%d MB)  rtt %d ms  fps %d" % [
		Game.zone_name, Game.zone_id, Game.map_id, Game.entity_id, Game.sid, Net.transport,
		str(Vector2i(own.scene_pos)) if own else "-", own.life if own else 0, own.life_max if own else 0, target_text,
		["tu luyện", "chiến đấu", "sát nhân"][clampi(Game.pk_state, 0, 2)], Game.pk_value,
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
			elif hit != null and hit.entity_id != Game.entity_id and hit.has_method("is_dialoger") and hit.is_dialoger():
				# KPlayer::DialogNpc: the 0x6e packet to the zone (within twice the npc's dialog radius, 248 px)
				var dseq := Game.npc_dialog(hit.entity_id)
				Log.debug("ui", "click npc dialog", {"npc": hit.entity_id, "name": hit.display_name, "seq": dseq})
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
		elif event.button_index == MOUSE_BUTTON_RIGHT and event.ctrl_pressed:
			# autoexec.lua: AddCommand("Ctrl+RButton", "", "Mouse_Menu()") - the player menu (gamecl.exe 0x004C2450)
			var hit := _entity_at(_mouse())
			if hit != null and hit.entity_type == ENTITY_PLAYER_KIND and hit.entity_id != Game.entity_id and _windows != null:
				_windows.open_player_menu(hit.entity_id, get_viewport().get_mouse_position())
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
		elif event.keycode == KEY_F9 and not event.echo and not _chat_input.has_focus():
			# AddCommand("F9", "", "Switch([[pk]])") / Ctrl+H: the PK switch - the zone's three states in turn (0x080DBE00 takes a state byte)
			Game.pk_state_request((Game.pk_state + 1) % 3)
		elif event.keycode == KEY_V and not event.echo and not _chat_input.has_focus():
			# AddCommand("V", "", "Switch([[sit]])"): the 0x71 packet - down when standing, up when sitting (0x005C37C2)
			var own: Node = _entities.get(Game.entity_id)
			Game.sit(not (own != null and own.has_method("is_sitting") and own.is_sitting()))


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
			_world.add_missle_effect(anims[3], int(d.get("dir", 0)), Vector2(float(d.get("x", 0)), float(d.get("y", 0))), int(d.get("z", 0)), int(d.get("skill_id", 0)))
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
	if _entities[id].has_method("set_team_mate"):
		_entities[id].set_team_mate(Game.is_team_mate(id))
	_spawn_count += 1
	if id == Game.entity_id and _windows != null and _windows.minimap != null:
		_windows.minimap.set_own(_entities[id])


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
		_world.action(node, a)
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
		if _world.has_method("on_life"):
			_world.on_life(node, l)


# G2C_STATE_ICONS (the 0x7a packet): the pictures of the states an entity holds
func _on_state_icons(entity_id: int) -> void:
	var node: Node = _entities.get(entity_id)
	var d = Game.entities.get(entity_id)
	if node != null and d != null and node.has_method("set_state_icons"):
		node.set_state_icons(d.get("state_icons", []))


# the 0x59 / 0x58 packets: a npc's camps - a player's name colour follows its current camp (0x005F2507)
func _on_entity_camp(c: Dictionary) -> void:
	var node: Node = _entities.get(int(c.id))
	if node != null and node.has_method("set_camp"):
		node.set_camp(int(c.camp), int(c.current_camp))


# the 0xad packet: a player's equipment look changed (KNpc::SetPlayerRes 0x005ED920)
func _on_entity_res(r: Dictionary) -> void:
	var node: Node = _entities.get(int(r.id))
	if node != null and node.has_method("set_equip_rows"):
		node.set_equip_rows(r.res)


# the flag & 3 of the 0x4b sync (G2C_ENTITY_PK): the PK state of another player -> KNpc+0x16e4
func _on_entity_pk(r: Dictionary) -> void:
	var node: Node = _entities.get(int(r.id))
	if node != null and node.has_method("set_pk_state"):
		node.set_pk_state(int(r.pk_state))


# s2c_npcsetmenustate: the sign over a player's head (and its trade sentence)
func _on_entity_menu_state(id: int) -> void:
	var node: Node2D = _entities.get(id)
	var d = Game.entities.get(id)
	if node != null and d != null and node.has_method("set_menu_state"):
		node.set_menu_state(int(d.get("menu_state", 0)), str(d.get("menu_sentence", "")))


# G2C_TEAM_SELF: the life bar of a team mate is (230, 190, 0) (PaintLife 0x005EADB8 when 0x0066D070 == 8) - every player
# node learns whether it is one now
func _on_team_changed() -> void:
	for id in _entities:
		var node: Node2D = _entities[id]
		if node != null and is_instance_valid(node) and node.has_method("set_team_mate"):
			node.set_team_mate(Game.is_team_mate(int(id)))


# the 0x90 / 0x93 packets: one's own PK state and value (G2C_PK_STATE); a refused switch keeps the state
func _on_pk_changed(state: int, _value: int, refused: bool) -> void:
	var own: Node = _entities.get(Game.entity_id)
	if own != null and own.has_method("set_pk_state"):
		own.set_pk_state(state)
	if refused:
		_append_chat("[PK] chưa đủ thời gian để đổi trạng thái PK (NormalPKTimeLong)")


# the ride flag of the 0x4a / 0x4b sync (G2C_ENTITY_RIDE): KNpc::SetRideHorse 0x005EC3E0
func _on_entity_ride(r: Dictionary) -> void:
	var node: Node = _entities.get(int(r.id))
	if node != null and node.has_method("set_riding"):
		node.set_riding(bool(r.riding))


# the 0x9a packet: a monster turned gold - its name takes the gold colour (0x005F23E5)
func _on_gold(entity_id: int) -> void:
	var node: Node = _entities.get(entity_id)
	var d = Game.entities.get(entity_id)
	if node != null and d != null and node.has_method("set_gold_type"):
		node.set_gold_type(int(d.get("gold_type", 0)))


func _on_chat(msg: Dictionary) -> void:
	if _windows == null:
		_append_chat("[b]%s:[/b] %s" % [msg.name, str(msg.text).replace("[", "[lb]")])
		return
	var line: Dictionary = _windows.chat_line(msg)
	if line.get("texture") != null:
		_chat_log.add_image(line.texture)
		_chat_log.add_text(" ")
	_append_chat(str(line.bbcode))


func _append_chat(bbcode: String) -> void:
	_chat_log.append_text(bbcode + "\n")


func _on_chat_submitted(text: String) -> void:
	_chat_input.text = ""
	_chat_input.release_focus()
	if text.strip_edges() != "":
		if _windows != null:
			_windows.send_chat(text)
		else:
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
	await _auto_sit()
	await _auto_ride()
	await _auto_pk()
	await _auto_team()
	await _auto_trade()
	await _auto_dialog()
	await _auto_task()
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
# --auto3d --factions: every faction's skills with a mapped 3D effect (skill_map.json, the reference's skill_main ids
# 100 Wudang, 200 Kunlun, 300 Gaibang, 400 Tianren, 500 Emei, 600 Cuiyan, 700 Wudu, 800 Tangmen, 900 Tianwang, 1000 Shaolin)
# are spawned client-side at our feet, one after another - the pictures only, no zone rules (weapon / level limits) -
# with one screenshot per faction; then one real cast per faction through the zone (SetFaction + add_xx(30) of
# skills_table.lua, a skill any weapon may use).  AUTO3D_FX lines per skill, AUTO3D_FACTIONS at the end.
const FACTION_REF := {"wudang": 100, "kunlun": 200, "gaibang": 300, "tianren": 400, "emei": 500, "cuiyan": 600, "wudu": 700,
	"tangmen": 800, "tianwang": 900, "shaolin": 1000}
const FACTION_ADD := {"shaolin": "add_sl", "tianwang": "add_tw", "tangmen": "add_tm", "wudu": "add_wu", "emei": "add_em",
	"cuiyan": "add_cy", "tianren": "add_tr", "gaibang": "add_gb", "wudang": "add_wd", "kunlun": "add_kl"}


func _auto3d_factions() -> void:
	var me := _own()
	var view = _world.get("_views").get(me) if me != null else null
	if me == null or view == null:
		return
	var by_jx: Dictionary = _world.fx.map.get("by_jx", {})
	# --fxshots: a half-size picture of every skill 0.45 s into its cast (auto3d_fx_<faction>_<skill>.png) - the review sheet
	var shots := "--fxshots" in OS.get_cmdline_user_args()
	view.rotation.y = 0.0   # the caster looks down -Z, where every aim below lies: a directional effect must go that way
	if shots and _world.get("cam_rig") != null:
		# the review pictures: the camera close and behind the caster, looking along the aim
		var rig = _world.cam_rig
		rig.dist = 8.0
		rig.dist_min = minf(rig.dist_min, 8.0)
		rig.pitch = 35.0
		rig.pitch_min = minf(rig.pitch_min, 35.0)
		rig.yaw = 0.0
		rig.call("_apply", true)
	var total := 0
	var with_fx := 0
	var shown := 0
	var n := 0
	var only := ""
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--factions="):
			only = arg.substr(11)
	for fac in FACTION_REF.keys():
		if only != "" and fac != only:
			continue
		var lo: int = FACTION_REF[fac]
		var ids: Array = []
		var seen_ref := {}
		for jid in by_jx.keys():
			var e: Dictionary = by_jx[jid]
			var ref := int(e.get("ref", 0))
			if ref >= lo and ref < lo + 100 and not seen_ref.has(ref):
				seen_ref[ref] = true
				ids.append(int(jid))
		ids.sort()
		var fac_fx := 0
		var best_id := 0
		var best_n := 0
		for sid in ids:
			total += 1
			var e: Dictionary = by_jx[str(sid)]
			if e.get("cast", []).is_empty() and e.get("child", []).is_empty() and e.get("aura", []).is_empty():
				print("AUTO3D_FX faction=%s skill=%d ref=%d name=%s spawned=0 (no picture: %s)" % [fac, sid, int(e.get("ref", 0)), str(e.get("cn", "")), str(e.get("events", {}))])
				continue
			with_fx += 1
			var before: int = _world.fx.spawned
			var pos: Vector3 = view.global_position
			var aim: Vector3 = pos + Vector3(0, 0, -4.0)
			_world.fx.cast(_world.get("_views_root"), sid, 18, pos, float(view.rotation.y), aim, view)
			var f: Dictionary = _world.fx.flying(sid)
			if e.has("ghost"):
				await get_tree().create_timer(0.3).timeout
				var ghosts := 0
				for gn in get_tree().current_scene.get_children():
					if "Ghost" in str(gn.name):   # Godot renames the later holders "@Ghost@n"
						ghosts += 1
				var gnames: Array = []
				for gn in get_tree().current_scene.get_children():
					if "Ghost" in str(gn.name):
						gnames.append(str(gn.name) + ":" + str(gn.get_child_count()))
				print("AUTO3D_GHOST skill=%d ghosts=%d alive=%s names=%s" % [sid, ghosts, str(view.get("_ghosts_alive")), str(gnames)])
			if not f.is_empty():
				_world.fx.hit(_world.get("_views_root"), sid, aim + Vector3(0, 0.9, 0))
				_world.fx.hit_on(view, sid, -1, float(view.rotation.y))   # the hit picture a struck entity would get (here: on the caster)
			var a: Node3D = _world.fx.aura(view, sid)
			if shots:
				await get_tree().create_timer(0.45).timeout
				await _save_screenshot("user://logs/auto3d_fx_%s_%d.png" % [fac, sid], true)
				await get_tree().create_timer(0.75).timeout
			else:
				await get_tree().create_timer(1.2).timeout   # the cast is 18 frames = 1 s: every child object's frame has passed
			var got: int = _world.fx.spawned - before
			if got > 0:
				shown += 1
				fac_fx += 1
			if got > best_n:
				best_n = got
				best_id = sid
			print("AUTO3D_FX faction=%s skill=%d ref=%d name=%s spawned=%d fly=%s" % [fac, sid, int(e.get("ref", 0)), str(e.get("cn", "")), got, not f.is_empty()])
			if a != null and is_instance_valid(a):
				a.queue_free()
		# the faction's fullest skill once more for the picture
		if best_id > 0:
			await get_tree().create_timer(0.8).timeout
			var pos2: Vector3 = view.global_position
			_world.fx.cast(_world.get("_views_root"), best_id, 18, pos2, float(view.rotation.y), pos2 + Vector3(0, 0, -4.0))
			if not _world.fx.flying(best_id).is_empty():
				_world.fx.hit(_world.get("_views_root"), best_id, pos2 + Vector3(0, 0.9, -4.0))
				_world.fx.hit_on(view, best_id, -1, float(view.rotation.y))
			await get_tree().create_timer(0.35).timeout
			await _save_screenshot("user://logs/auto3d_fx_%s.png" % fac)
			await get_tree().create_timer(0.55).timeout
			await _save_screenshot("user://logs/auto3d_fx_%s_b.png" % fac)   # the later child objects (at 0.9 of the cast)
			if only != "":
				# --factions=<one>: what is on screen now (the effect nodes alive, their meshes and materials)
				for fxn in _world.get("_views_root").get_children():
					if str(fxn.name).begins_with("sfx_"):
						for mi in fxn.find_children("*", "MeshInstance3D", true, false):
							var mat = mi.get_surface_override_material(0) if mi.mesh != null and mi.mesh.get_surface_count() > 0 else null
							print("AUTO3D_FXNODE %s/%s scale=%s albedo=%s blend=%s tex=%s aabb=%s" % [fxn.name, mi.name, str(mi.global_transform.basis.get_scale()), str(mat.albedo_color) if mat is StandardMaterial3D else "-", str(mat.blend_mode) if mat is StandardMaterial3D else "-", str(mat.albedo_texture != null) if mat is StandardMaterial3D else "-", str(mi.get_aabb().size)])
		print("AUTO3D_FACTION faction=%s skills=%d shown=%d best=%d" % [fac, ids.size(), fac_fx, best_id])
		n += 1
	print("AUTO3D_FACTIONS factions=%d skills=%d with_fx=%d shown=%d" % [n, total, with_fx, shown])
	# what still hangs on the caster's view after the sweep (a looping effect that should not be there shows up here)
	var names: Array = []
	for c in view.get_children():
		names.append(str(c.name))
	for c in view.find_children("sfx_*", "", true, false):
		names.append("deep:" + str(c.name) + "@" + str(c.get_parent().name))
	print("AUTO3D_VIEW_CHILDREN %s" % str(names))


func _auto3d_run() -> void:
	await get_tree().create_timer(1.0).timeout
	var waited := 0.0
	while waited < 10.0 and not (_world.is_3d() and _own() != null):
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
	var own := _own()
	print("AUTO3D view=%s map=%d entities=%d own=%s" % [_world.name, Game.map_id, _entities.size(), str(own.scene_pos) if own else "-"])
	if _world.is_3d() and _world.get("place") != null and _world.place.has_method("scene_effects_live"):
		await get_tree().create_timer(1.0).timeout
		print("AUTO3D_SCENEFX placed=%d live=%d cam_far=%.0f glow=%s" % [_world.place._effects.size(), _world.place.scene_effects_live(), _world.cam_rig.cam.far, str(_world.place._env.environment.glow_enabled) if _world.place._env != null else "-"])
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
	# the frame rate of the 3D map itself (M3D-6): two seconds at the default camera, vsync off with --nosync
	if "--nosync" in OS.get_cmdline_user_args():
		DisplayServer.window_set_vsync_mode(DisplayServer.VSYNC_DISABLED)
		var t0 := Time.get_ticks_msec()
		var frames := 0
		while Time.get_ticks_msec() - t0 < 2000:
			await get_tree().process_frame
			frames += 1
		print("AUTO3D_FPS map=%d quality=%s fps=%.1f draw_calls=%d primitives=%d vram_mb=%.0f" % [Game.map_id, _world.place.quality,
			frames * 1000.0 / maxf(1.0, float(Time.get_ticks_msec() - t0)), Performance.get_monitor(Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME),
			Performance.get_monitor(Performance.RENDER_TOTAL_PRIMITIVES_IN_FRAME), Performance.get_monitor(Performance.RENDER_VIDEO_MEM_USED) / 1048576.0])
	# the building fade (CameraBuildingFade rules): the camera swung around at its longest arm, the pictures where a
	# building stands between it and the character show the building at FadeAlpha
	if "--fade" in OS.get_cmdline_user_args() and _world.place.get("mode") == "3d":
		_world.cam_rig.dist = _world.cam_rig.dist_max
		for yaw in [0.0, 60.0, 120.0, 180.0, 240.0, 300.0]:
			_world.cam_rig.yaw = yaw
			for i in 30:
				await get_tree().process_frame
			await _save_screenshot("user://logs/auto3d_fade_%d.png" % int(yaw))
			print("AUTO3D_FADE yaw=%d occluders=%d faded=%d" % [int(yaw), _world.place._occluders.size(), _world.place._fade_state.size()])
		_world.cam_rig.dist = 19.0
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
	if _world.has_method("debug_equip_rows"):
		var eq: Dictionary = _world.debug_equip_rows()
		print("AUTO3D_WEAPON bag=%s rows=%s row=%s" % [eq.get("weapon_bag", ""), eq.get("weapon_rows", ""), str(eq.get("rows", {}).get(2, -1))])
	# a horse (AddItem genre 0 detail 10 particular 2 level 1 = Liệt Bạch Mã), worn on part 10, then C2G_RIDE: the 3D view mounts
	# (the reference 白马 under the rider, group 20 / 21)
	var horses_before := Game.items.size()
	Game.chat("?gm ds AddItem(0,10,2,1,0,0)")
	waited = 0.0
	while waited < 3.0 and Game.items.size() < horses_before + 1:
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
	var horse_item := 0
	for id in Game.items:
		if int(Game.items[id].genre) == 0 and int(Game.items[id].detail) == 10 and int(Game.items[id].room) == Game.ROOM_BAG:
			horse_item = int(id)
	if horse_item != 0:
		Game.item_equip(horse_item, 10)
		waited = 0.0
		while waited < 3.0 and Game.item_worn(10) != horse_item:
			await get_tree().create_timer(0.25).timeout
			waited += 0.25
		Game.ride(true)
		waited = 0.0
		while waited < 3.0 and not (own != null and is_instance_valid(own) and bool(own.get("riding"))):
			await get_tree().create_timer(0.25).timeout
			waited += 0.25
		for i in 4:
			await get_tree().process_frame
		await _save_screenshot("user://logs/auto3d_ride.png")
		print("AUTO3D_RIDE item=%d worn=%d riding=%s" % [horse_item, Game.item_worn(10), bool(own.get("riding")) if own != null else false])
		if _world.has_method("debug_equip_rows"):
			var eq: Dictionary = _world.debug_equip_rows()
			var hv = _world.get("_views").get(own) if own != null else null
			var horse_cha: String = str(hv.horse.get("cha")) if hv != null and hv.get("horse") != null and hv.horse.get("cha") != null else "?"
			print("AUTO3D_HORSE rows=%s row=%s item_row=%s shown=%s" % [str(eq.get("horse_rows", -1)), str(eq.get("rows", {}).get(3, -1)), str(eq.get("horse_item_row", -1)), horse_cha])
		Game.ride(false)
		await get_tree().create_timer(0.6).timeout
	print("AUTO3D_WEAPON item=%d worn=%d weapon=%s skill53=%s skill53_level=%d level=%d item_results=%s item=%s" % [sword, Game.item_worn(3), _world._own_weapon, Game.skills.has(53),
		int(Game.skills.get(53, {}).get("level", -1)), int(Game.player_attrib.get("level", 0)), str(results), "room %s x %s y %s name %s" % [str(Game.items.get(sword, {}).get("room")), str(Game.items.get(sword, {}).get("x")), str(Game.items.get(sword, {}).get("y")), str(Game.items.get(sword, {}).get("name"))]])
	Game.chat("?gm ds SetFightState(1)")
	await _auto_fight()
	await _save_screenshot("user://logs/auto3d_%d.png" % n)
	n += 1
	if _world.get("floats") != null:
		print("AUTO3D_FLOATS added=%d live=%d hit_fx=%d" % [_world.floats.added, _world.floats._items.size(), _world.fx.spawned])
		for vn in _world.get("_views").values():
			if vn.get("_select_fx") != null and is_instance_valid(vn._select_fx):
				var sfx: Node3D = vn._select_fx
				var aabb := AABB()
				for mi in sfx.find_children("*", "MeshInstance3D", true, false):
					aabb = aabb.merge(mi.global_transform * mi.get_aabb()) if aabb.size != Vector3.ZERO else mi.global_transform * mi.get_aabb()
				print("AUTO3D_SELECT view=%s fx=%s top=%s visible=%s aabb=%s scale=%s" % [str(vn.global_position), str(sfx.global_position), str(sfx.top_level), str(sfx.visible), str(aabb), str(sfx.scale)])
	# a faction skill with a mapped 3D effect (skill_map.json): Wudang's Nộ Lôi Chỉ (153, any weapon, level 10) - its missile
	# is the reference client's 怒雷指 child object; handed out the way the faction script does (SetFaction + add_wd + AddMagic)
	if _world.is_3d() and _world.get("fx") != null:
		# --skill=<jx id>:<faction code>: another skill / faction instead (auto3d_skill_<id>_<k>.png during the flight)
		var fx_skill := 153
		var fx_fac := "wudang"
		var add_fn := {"shaolin": "add_sl", "tianwang": "add_tw", "tangmen": "add_tm", "wudu": "add_wu", "emei": "add_em",
			"cuiyan": "add_cy", "tianren": "add_tr", "gaibang": "add_gb", "wudang": "add_wd", "kunlun": "add_kl", "huashan": "add_hs"}
		for arg in OS.get_cmdline_user_args():
			if arg.begins_with("--skill="):
				var parts := arg.substr(8).split(":")
				fx_skill = int(parts[0])
				if parts.size() > 1:
					fx_fac = parts[1]
		Game.chat("?gm ds SetFaction(\"%s\")" % fx_fac)
		Game.chat("?gm ds Include(\"\\\\script\\\\global\\\\skills_table.lua\") %s(%d)" % [add_fn.get(fx_fac, "add_wd"), 90 if fx_fac != "wudang" else 30])
		waited = 0.0
		while waited < 4.0 and Game.faction_last < 0:
			await get_tree().create_timer(0.25).timeout
			waited += 0.25
		if fx_skill != 153:
			# KSkillList::can_cast 0x080E4540 wants the character's level >= the skill's ReqLevel (Phi Long Tại Thiên: 80)
			# (KPlayer::add_exp 0x080AFEA0 caps a gain at the next level's need: one level per call)
			for k in 80:
				if int(Game.player_attrib.get("level", 1)) >= 90:
					break
				Game.chat("?gm ds AddExp(2000000000, 0)")
				await get_tree().create_timer(0.05).timeout
			waited = 0.0
			while waited < 6.0 and int(Game.player_attrib.get("level", 1)) < 90:
				await get_tree().create_timer(0.25).timeout
				waited += 0.25
			print("AUTO3D_LEVEL level=%d skill=%d skill_level=%d" % [int(Game.player_attrib.get("level", 1)), fx_skill, int(Game.skills.get(fx_skill, {}).get("level", 0))])
			# a skill bound to a ranged weapon (EqtLimit >= 100 = DetailType 1, particular EqtLimit - 100: KSubWorld::weapon_eqt_limit
			# 0x080E8C05): that weapon in hand first (AddItem genre 0 detail 1)
			var lim := int(Game.skill_row(fx_skill).get("EqtLimit", -2))
			if lim >= 100:
				var nb := Game.items.size()
				Game.chat("?gm ds AddItem(0,1,%d,1,0,0)" % (lim - 100))
				waited = 0.0
				while waited < 3.0 and Game.items.size() < nb + 1:
					await get_tree().create_timer(0.25).timeout
					waited += 0.25
				for id in Game.items:
					if int(Game.items[id].genre) == 0 and int(Game.items[id].detail) == 1 and int(Game.items[id].room) == Game.ROOM_BAG:
						Game.item_equip(int(id), 3)
				await get_tree().create_timer(0.8).timeout
			elif lim >= 0:
				# a melee weapon of that type (EqtLimit = particular of genre 0 detail 0: 0 sword, 1 blade, 2 spear, 3 staff, 4 dual
				# blades, 5 dual hammers, 6 fists) instead of the sword the flow wore
				var nb2 := Game.items.size()
				Game.chat("?gm ds AddItem(0,0,%d,1,0,0)" % lim)
				waited = 0.0
				while waited < 3.0 and Game.items.size() < nb2 + 1:
					await get_tree().create_timer(0.25).timeout
					waited += 0.25
				for id in Game.items:
					if int(Game.items[id].genre) == 0 and int(Game.items[id].detail) == 0 and int(Game.items[id].particular) == lim and int(Game.items[id].room) == Game.ROOM_BAG:
						Game.item_equip(int(id), 3)
				await get_tree().create_timer(0.8).timeout
		if true:
			if int(Game.skills.get(fx_skill, {}).get("level", 0)) <= 0:
				Game.chat("?gm ds AddMagic(%d, 1)" % fx_skill)   # at level 1 directly (AddMagic of the script api)
				waited = 0.0
				while waited < 2.0 and int(Game.skills.get(fx_skill, {}).get("level", 0)) <= 0:
					await get_tree().create_timer(0.25).timeout
					waited += 0.25
			var victim: Node = null
			var best_d := 600.0
			var me := _own()
			for node in _entities.values():
				if me != null and node != me and node.is_attackable() and node.scene_pos.distance_to(me.scene_pos) < best_d:
					best_d = node.scene_pos.distance_to(me.scene_pos)
					victim = node
			if victim == null and fx_skill != 153 and me != null:
				# --skill: no monster within 600 units - step next to the nearest one anywhere in view (SetPos, cells of 32 units)
				var far_d := INF
				for node in _entities.values():
					if node != me and node.is_attackable() and node.scene_pos.distance_to(me.scene_pos) < far_d:
						far_d = node.scene_pos.distance_to(me.scene_pos)
						victim = node
				if victim != null:
					var at: Vector2 = victim.scene_pos + Vector2(-160, 0)
					Game.chat("?gm ds SetPos(%d, %d)" % [int(at.x / 32.0), int(at.y / 32.0)])
					await get_tree().create_timer(1.2).timeout
			Game.chat("?gm ds SetFightState(1)")
			await get_tree().create_timer(0.3).timeout
			var before_fx: int = _world.fx.spawned
			if victim != null:
				Game.cast_skill(fx_skill, victim.entity_id)
			elif me != null:
				Game.cast_skill(fx_skill, 0, int(me.scene_pos.x), int(me.scene_pos.y))
			# the zone answers with the EntityAction of the cast (the character may first walk into reach): wait for the first effect
			waited = 0.0
			while waited < 6.0 and _world.fx.spawned <= before_fx:
				await get_tree().create_timer(0.05).timeout
				waited += 0.05
			if fx_skill != 153:
				# the flight of the chosen skill, four pictures 0.3 s apart, with the missile nodes on screen
				for k in 4:
					await get_tree().create_timer(0.3).timeout
					await _save_screenshot("user://logs/auto3d_skill_%d_%d.png" % [fx_skill, k])
					for mv in _world.get("_views_root").get_children():
						if mv.get("missle") != null and mv.get("_custom") != null:
							print("AUTO3D_MISSLE k=%d node=%s pos=%s yaw=%.1f dir64=%s status=%s" % [k, mv.name, str(mv.global_position), rad_to_deg(mv.rotation.y), str(mv.missle.get("dir64")), str(mv.missle.get("status"))])
							if str(mv._custom.name) == "Orbit":
								# moveType 7: the orbit driver (KSkillOrbit3D) - its angle / radius and where its effect is
								var oc: Node3D = mv._custom
								print("AUTO3D_MISSLE_ORBIT k=%d angle=%.1f t=%.2f radius_end=%.2f pos=%s centre=%s vis=%s children=%d" % [k, float(oc.get("_angle")), float(oc.get("_t")), float(oc.get("born_distance")), str(oc.global_position), str(mv.global_position), str(oc.visible), oc.get_child_count()])
							for ln in mv._custom.get_children():
								if str(ln.name).begins_with("line_") or str(ln.name).begins_with("ptrail_"):
									var im = ln.get("mesh")
									print("AUTO3D_MISSLE_LINE %s surfaces=%d start=%s end=%s vis=%s aabb=%s" % [ln.name, im.get_surface_count() if im != null else -1, str(ln.get("start_pos")), str(ln.get("end_node").global_position) if ln.get("end_node") != null else "-", str(ln.visible), str(ln.get_aabb())])
			await get_tree().create_timer(0.3).timeout
			await _save_screenshot("user://logs/auto3d_%d.png" % n)
			n += 1
			await get_tree().create_timer(0.6).timeout
			await _save_screenshot("user://logs/auto3d_%d.png" % n)
			n += 1
			print("AUTO3D_SKILL skill=%d target=%d fx_spawned=%d map_entries=%d" % [fx_skill, victim.entity_id if victim != null else 0, _world.fx.spawned - before_fx, _world.fx.map.get("by_jx", {}).size()])
			# the aura: Wudang's Thất Tinh Trận (159) switched on (KNpc::SetAura) - its child state comes back in Game.states and the
			# reference halo (Halo/halo_wd_qixingzhen, state_list 100) loops at the feet
			Game.chat("?gm ds AddMagic(159, 1)")
			await get_tree().create_timer(0.5).timeout
			Game.set_aura(159)
			waited = 0.0
			while waited < 4.0 and not (Game.states.has(159) or Game.states.has(211)):
				await get_tree().create_timer(0.25).timeout
				waited += 0.25
			await get_tree().create_timer(0.5).timeout
			await _save_screenshot("user://logs/auto3d_aura.png")
			var auras = _world._auras.get(_own(), {})
			print("AUTO3D_AURA states=%s halos=%d" % [str(Game.states.keys()), auras.size() if auras is Dictionary else 0])
			Game.set_aura(0)
		if "--factions" in OS.get_cmdline_user_args() or Array(OS.get_cmdline_user_args()).any(func(x: String) -> bool: return x.begins_with("--factions=")):
			await _auto3d_factions()
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
	# the test character keeps its bag between runs: drop the junk of earlier runs (bag items that are not quest ones)
	# down to a few, so the AddItems below and the horse of _auto_ride find room
	var junk: Array = []
	for id in Game.items:
		var it: Dictionary = Game.items[id]
		if int(it.get("room", -1)) == Game.ROOM_BAG and int(it.get("genre", -1)) != 4:
			junk.append(int(id))
	if junk.size() > 4:
		var total := Game.items.size()
		var dropping := junk.size() - 4
		for i in range(dropping):
			Game.item_drop(junk[i])
		var cleaned := 0.0
		while cleaned < 3.0 and Game.items.size() > total - dropping:
			await get_tree().create_timer(0.25).timeout
			cleaned += 0.25
		Log.info("auto", "bag cleaned", {"dropped": dropping, "left": Game.items.size()})
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
	var shot_hit := false
	while waited < 8.0 and is_instance_valid(best) and not best.is_dead() and best.life > 0:
		await get_tree().create_timer(0.25).timeout
		waited += 0.25
		if not shot_hit and best.life < life_before:
			# the first loss: the damage number and the hit picture still on screen (auto_fight_hit.png)
			shot_hit = true
			await get_tree().create_timer(0.15).timeout
			await _save_screenshot("user://logs/auto_fight_hit.png")
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
# the sit of the 2.0 tool bar (Switch([[sit]]) -> the 0x71 packet): the wounded character sits, its life climbs by
# SitAddLife every ten frames (0x0808BBE6), the sit animation holds its last frame, then it stands up again
func _auto_sit() -> void:
	var own: Node = _entities.get(Game.entity_id)
	if own == null:
		print("AUTO_SIT none")
		return
	if own.riding:
		Game.ride(false)   # 0x080DC367: no sitting on horseback - down first (a horse worn from an earlier run)
		for i in 20:
			await get_tree().create_timer(0.1).timeout
			own = _entities.get(Game.entity_id)
			if own != null and not own.riding:
				break
	var life_before: int = own.life
	Game.sit(true)
	var sat := false
	for i in 20:
		await get_tree().create_timer(0.1).timeout
		own = _entities.get(Game.entity_id)
		if own != null and own.is_sitting():
			sat = true
			break
	await get_tree().create_timer(1.6).timeout
	own = _entities.get(Game.entity_id)
	var frame_held: int = own.cur_frame if own != null else -1
	await _save_screenshot("user://logs/auto_sit.png")
	var life_after: int = own.life if own != null else -1
	Game.sit(false)
	var stood := false
	for i in 20:
		await get_tree().create_timer(0.1).timeout
		own = _entities.get(Game.entity_id)
		if own != null and not own.is_sitting():
			stood = true
			break
	print("AUTO_SIT sat=%s frame=%d life_before=%d life_after=%d stood=%s" % [sat, frame_held, life_before, life_after, stood])


# a horse from the gm (AddItem genre 0 detail 10 = equip_horse, particular 0, level 1 -> HorseRes row 3 -> horse row 8), worn
# (0x081FE752: worn = ridden), the on-horse pictures (KNpcRes::SetRideHorse -> actions 38..) with the name 38 higher, then off
# the PK switch (F9): the fight state is taken at once (0x080DBF10: forced when not in fight mode - the auto character is,
# so SetPKState 0x080C3740 unforced: still taken for 1 / 2), the way back to 0 needs NormalPKTimeLong seconds in the state
# (0x080C3789) -> refused; the own life bar turns red-ish, AUTO_PK reports both answers
func _auto_pk() -> void:
	Game.pk_state_request(1)
	var on := false
	for i in 20:
		await get_tree().create_timer(0.1).timeout
		if Game.pk_state == 1:
			on = true
			break
	var own: Node = _entities.get(Game.entity_id)
	var bar_state: int = own.pk_state if own != null else -1
	await _save_screenshot("user://logs/auto_pk.png")
	var answer := {"refused": false, "back": false, "done": false}   # a lambda captures locals by value: the answer lives in a dictionary
	var cb := func(state: int, _value: int, r: bool) -> void:
		answer.refused = answer.refused or r
		answer.back = state == 0
		answer.done = true
	Game.pk_changed.connect(cb)
	Game.pk_state_request(0)
	for i in 20:
		await get_tree().create_timer(0.1).timeout
		if answer.done:
			break
	Game.pk_changed.disconnect(cb)
	Log.info("auto", "auto pk", {"on": on, "bar_state": bar_state, "value": Game.pk_value, "back": answer.back, "refused": answer.refused})
	print("AUTO_PK on=%s bar_state=%d value=%d back=%s refused=%s" % [on, bar_state, Game.pk_value, answer.back, answer.refused])


# the team of one character: create (the 0x53 sub 2 -> TeamSelf captain / open), the window, close the team (sub 3), dismiss
# (sub 9 -> TeamSelf out of a team); the invitations need a second player (the [team] tests of the zone cover them)
func _auto_team() -> void:
	var answer := {"changes": 0}
	var cb := func() -> void:
		answer.changes += 1
	Game.team_changed.connect(cb)
	Game.team_request(Proto.TeamCmd.TEAM_CREATE)
	for i in 20:
		await get_tree().create_timer(0.1).timeout
		if bool(Game.team.in_team):
			break
	var created := bool(Game.team.in_team)
	var captain := bool(Game.team.captain)
	var open_state := int(Game.team.state)
	var lead_level := int(Game.team.lead_level)
	var members_max := int(Game.team.members_max)
	# the partner bot (jxbot -partner, dev.py screenshot): invited, it says yes - the list grows, its life bar turns team colour
	var partner_id := _auto_partner_id()
	var invited := false
	var mate_color := false
	if partner_id != 0:
		Game.team_request(Proto.TeamCmd.TEAM_INVITE, partner_id)
		for i in 40:
			await get_tree().create_timer(0.1).timeout
			if Game.team.members.size() >= 1:
				break
		invited = Game.team.members.size() >= 1
		var pn: Node2D = _entities.get(partner_id)
		mate_color = pn != null and "team_mate" in pn and bool(pn.team_mate)
	var window_ok := false
	if _windows != null and _windows.team_window != null:
		_windows.team_window.open_window()
		await get_tree().create_timer(0.3).timeout
		window_ok = _windows.team_window.visible and _windows.team_window.member_count() == (2 if invited else 1)
		await _save_screenshot("user://logs/auto_team.png")
		_windows.team_window.close_window()
	print("AUTO_TEAM_PARTNER partner=%d invited=%s members=%d mate_color=%s" % [partner_id, invited, Game.team.members.size(), mate_color])
	# the channels (docs/LINUX-SERVER.md §19): a team line comes back on CH_TEAM (the team hears it), a world line
	# through the zone's fan-out on CH_WORLD (level 100: the level 30 / 80 % mana of chatcost.ini type 4 are met)
	var heard := {}
	var on_chat := func(m: Dictionary) -> void:
		heard[int(m.get("channel", 0))] = str(m.get("text", ""))
	Game.chat_msg.connect(on_chat)
	# through the bar's rules (KUiPlayerBar::SendChat 0x00475A10): "&T ..." picks the team by its short name, the plain
	# line goes on the current channel = the world one picked from the ChannelBtn menu (0x00475900)
	Game.chat("?gm ds RestoreMana()")   # the casts above drank the mana: the world line needs 80 % of it
	await get_tree().create_timer(0.3).timeout
	if _windows != null:
		_windows.set_channel(Proto.ChatChannel.CH_WORLD)
		if invited:
			_windows.send_chat("&T doi oi")
		_windows.send_chat("ca the gioi")
	else:
		if invited:
			Game.chat("doi oi", Proto.ChatChannel.CH_TEAM)
		Game.chat("ca the gioi", Proto.ChatChannel.CH_WORLD)
	for i in 30:
		await get_tree().create_timer(0.1).timeout
		if heard.has(int(Proto.ChatChannel.CH_WORLD)) and (not invited or heard.has(int(Proto.ChatChannel.CH_TEAM))):
			break
	Game.chat_msg.disconnect(on_chat)
	var button_short := ""
	var menu_open := false
	if _windows != null and _windows.player_bar != null and _windows.player_bar.channel_btn != null:
		button_short = str(_windows.player_bar.channel_btn.label)
		_windows._open_channel_menu(_windows.player_bar.channel_btn.global_position)
		await get_tree().create_timer(0.2).timeout
		menu_open = _windows.channel_menu != null and _windows.channel_menu.visible
		await _save_screenshot("user://logs/auto_chat.png")
		if _windows.channel_menu != null:
			_windows.channel_menu.hide_menu()
		_windows.set_channel(Proto.ChatChannel.CH_NEARBY)
	print("AUTO_CHAT team=%s world=%s button=%s menu=%s" % [heard.get(int(Proto.ChatChannel.CH_TEAM), "-"), heard.get(int(Proto.ChatChannel.CH_WORLD), "-"), button_short, menu_open])
	Game.team_request(Proto.TeamCmd.TEAM_OPEN_CLOSE, 0, 0)
	for i in 20:
		await get_tree().create_timer(0.1).timeout
		if int(Game.team.state) == 0:
			break
	var closed_ok := int(Game.team.state) == 0
	Game.team_request(Proto.TeamCmd.TEAM_DISMISS)
	for i in 20:
		await get_tree().create_timer(0.1).timeout
		if not bool(Game.team.in_team):
			break
	var dismissed := not bool(Game.team.in_team)
	Game.team_changed.disconnect(cb)
	Log.info("auto", "auto team", {"created": created, "captain": captain, "state": open_state, "lead_level": lead_level,
		"members_max": members_max, "window": window_ok, "closed": closed_ok, "dismissed": dismissed, "changes": answer.changes})
	print("AUTO_TEAM created=%s captain=%s open=%d lead_level=%d members_max=%d window=%s closed=%s dismissed=%s changes=%d" % [created, captain,
		open_state, lead_level, members_max, window_ok, closed_ok, dismissed, answer.changes])


# the player of the partner bot around (jxbot -partner): another player with its trade sign up, else any other player
func _auto_partner_id() -> int:
	var any_player := 0
	for id in Game.entities:
		var e: Dictionary = Game.entities[id]
		if int(id) == Game.entity_id or int(e.get("type", 0)) != ENTITY_PLAYER_KIND:
			continue
		if int(e.get("menu_state", 0)) == 2:
			return int(id)
		if any_player == 0:
			any_player = int(id)
	return any_player


# the trade with the partner bot: Ctrl+right click "Giao Dịch" = TradeApplyStart, the bot says yes (both TRADING, the
# window opens), an item and 5 coins go on my table, the lock (the bot locks and confirms after me), a picture of the window
# with both locked, my ok -> the exchange (0x78 {1}): the item left, the money moved.  Without a partner: T puts my own
# sign up, a picture, T takes it down.
func _auto_trade() -> void:
	var answer := {"changes": 0, "end": -1}
	var cb := func() -> void:
		answer.changes += 1
	var cb_end := func(ok: bool) -> void:
		answer.end = 1 if ok else 0
	Game.trade_changed.connect(cb)
	Game.trade_end.connect(cb_end)
	var partner_id := 0
	for i in 30:
		partner_id = _auto_partner_id()
		var e = Game.entities.get(partner_id)
		if e != null and int(e.get("menu_state", 0)) == 2:
			break
		await get_tree().create_timer(0.2).timeout
	var pe = Game.entities.get(partner_id)
	if partner_id != 0 and pe != null and int(pe.get("menu_state", 0)) == 2:
		# 50 coins from the script api (Earn 0x08118970) so 5 of them can go on the table
		Game.chat("?gm ds Earn(50)")
		for i in 20:
			await get_tree().create_timer(0.1).timeout
			if int(Game.money) >= 5:
				break
		var money_before := int(Game.money)
		var item_id := 0
		for id in Game.items:
			var it: Dictionary = Game.items[id]
			if int(it.room) == 0 and int(it.genre) != 4:
				item_id = int(id)
				break
		Game.trade_request(Proto.TradeCmd.TRADE_APPLY_START, partner_id)
		for i in 40:
			await get_tree().create_timer(0.1).timeout
			if int(Game.trade.state) == 2:
				break
		var started := int(Game.trade.state) == 2
		var placed := false
		if started and item_id != 0:
			Game.item_move(item_id, 2, 0, 0)
			for i in 20:
				await get_tree().create_timer(0.1).timeout
				var it2 = Game.items.get(item_id)
				if it2 != null and int(it2.room) == 2:
					placed = true
					break
		if started:
			# 5 coins typed into the window's SelfMoney edit (the 0x6c packet), or the bare request without a window
			if _windows != null and _windows.trade_window != null and _windows.trade_window.visible:
				_windows.trade_window.put_money(5)
			else:
				Game.trade_request(Proto.TradeCmd.TRADE_MONEY, 0, 5)
			await get_tree().create_timer(0.3).timeout
			Game.trade_request(Proto.TradeCmd.TRADE_DECISION, 0, 2)   # the lock: the bot locks and confirms after it
		for i in 40:
			await get_tree().create_timer(0.1).timeout
			if bool(Game.trade.self_lock) and bool(Game.trade.dest_lock) and bool(Game.trade.dest_ok):
				break
		var both_locked := bool(Game.trade.self_lock) and bool(Game.trade.dest_lock)
		var dest_ok := bool(Game.trade.dest_ok)
		var window_open: bool = _windows != null and _windows.trade_window != null and _windows.trade_window.visible
		var my_table: int = _windows.trade_window.my_table_count() if window_open else -1
		await get_tree().create_timer(0.2).timeout
		await _save_screenshot("user://logs/auto_trade.png")
		if started:
			Game.trade_request(Proto.TradeCmd.TRADE_DECISION, 0, 1)   # my ok: the exchange
		for i in 40:
			await get_tree().create_timer(0.1).timeout
			if answer.end >= 0:
				break
		await get_tree().create_timer(0.3).timeout
		var item_gone := item_id != 0 and not Game.items.has(item_id)
		Game.trade_changed.disconnect(cb)
		Game.trade_end.disconnect(cb_end)
		Log.info("auto", "auto trade", {"partner": partner_id, "started": started, "placed": placed, "both_locked": both_locked, "dest_ok": dest_ok,
			"window": window_open, "my_table": my_table, "end": answer.end, "item_gone": item_gone, "money_before": money_before, "money": int(Game.money)})
		print("AUTO_TRADE partner=%d started=%s placed=%s both_locked=%s dest_ok=%s window=%s my_table=%d end=%d item_gone=%s money=%d->%d" % [
			partner_id, started, placed, both_locked, dest_ok, window_open, my_table, answer.end, item_gone, money_before, int(Game.money)])
		return
	if _windows != null:
		_windows.toggle_trade_sign("ban gi cung mua")
	for i in 20:
		await get_tree().create_timer(0.1).timeout
		if int(Game.trade.state) == 1:
			break
	var opened := int(Game.trade.state) == 1
	var own: Node2D = _entities.get(Game.entity_id)
	var sign_state: int = own.menu_state if own != null and "menu_state" in own else -1
	var sign_drawn: bool = own != null and own.get("_sign") != null and own._sign.visible
	await get_tree().create_timer(0.3).timeout
	await _save_screenshot("user://logs/auto_trade.png")
	if _windows != null:
		_windows.toggle_trade_sign()
	for i in 20:
		await get_tree().create_timer(0.1).timeout
		if int(Game.trade.state) == 0:
			break
	var closed := int(Game.trade.state) == 0
	Game.trade_changed.disconnect(cb)
	Game.trade_end.disconnect(cb_end)
	Log.info("auto", "auto trade", {"opened": opened, "sign": sign_state, "drawn": sign_drawn, "closed": closed, "changes": answer.changes})
	print("AUTO_TRADE opened=%s sign=%d drawn=%s closed=%s changes=%d" % [opened, sign_state, sign_drawn, closed, answer.changes])


# the npc dialog: the nearest dialoger within 248 px (the map's placed npcs with scripts; docs §20) is clicked, its
# script's Say arrives as a script action -> the question window (KUiMsgSel) with the sentence and the answers, a picture,
# then the first answer (or the closing line) is clicked
func _auto_dialog() -> void:
	var own := _own()
	var best: Node2D = null
	var best_d := 1.0e9
	for id in _entities:
		var node: Node2D = _entities[id]
		if node == null or not is_instance_valid(node) or not node.has_method("is_dialoger") or not node.is_dialoger():
			continue
		var d: float = own.scene_pos.distance_to(node.scene_pos) if own != null else 1.0e9
		if d < best_d:
			best_d = d
			best = node
	if best == null:
		print("AUTO_DIALOG npc=0")
		return
	# the 2.0 client walks up to a npc that is out of reach before it talks (twice m_DialogRadius = 248 px): so do we
	if own != null and best_d > 240.0:
		Game.move_to(int(best.scene_pos.x) - 120, int(best.scene_pos.y))
		for i in 40:
			await get_tree().create_timer(0.2).timeout
			own = _own()
			if own == null:
				break
			best_d = own.scene_pos.distance_to(best.scene_pos)
			if best_d <= 240.0 and not own.is_moving():
				break
	var got := {"action": {}}
	var on_action := func(a: Dictionary) -> void:
		got.action = a
	Game.script_action.connect(on_action)
	Game.npc_dialog(best.entity_id)
	for i in 30:
		await get_tree().create_timer(0.1).timeout
		if not got.action.is_empty():
			break
	Game.script_action.disconnect(on_action)
	var a: Dictionary = got.action
	var window_open: bool = _windows != null and _windows.msg_sel != null and _windows.msg_sel.visible
	await get_tree().create_timer(0.2).timeout
	await _save_screenshot("user://logs/auto_dialog.png")
	var answered := false
	if window_open:
		_windows.msg_sel._on_click(0)   # the first line: an answer, or the closing line when there is none
		answered = true
		await get_tree().create_timer(0.3).timeout
	Log.info("auto", "auto dialog", {"npc": best.entity_id, "name": best.display_name, "distance": int(best_d), "ui": a.get("ui", -1),
		"text_len": str(a.get("text", "")).length(), "options": a.get("options", []).size(), "window": window_open, "answered": answered})
	print("AUTO_DIALOG npc=%d name=%s distance=%d ui=%d text_len=%d options=%d window=%s answered=%s" % [best.entity_id, best.display_name,
		int(best_d), a.get("ui", -1), str(a.get("text", "")).length(), a.get("options", []).size(), window_open, answered])


func _auto_ride() -> void:
	var own: Node = _entities.get(Game.entity_id)
	if own == null:
		print("AUTO_RIDE none")
		return
	var before := Game.items.size()
	Game.chat("?gm ds AddItem(0,10,0,1,0,0)")
	var waited := 0.0
	while waited < 3.0 and Game.items.size() < before + 1:
		await get_tree().create_timer(0.1).timeout
		waited += 0.1
	var horse_id := 0
	for id in Game.items:
		var it: Dictionary = Game.items[id]
		if int(it.get("genre", -1)) == 0 and int(it.get("detail", -1)) == 10 and int(it.get("room", -1)) == Game.ROOM_BAG:
			horse_id = int(id)
	if horse_id == 0:
		print("AUTO_RIDE horse=false")
		return
	Game.item_equip(horse_id)
	var mounted := false
	for i in 30:
		await get_tree().create_timer(0.1).timeout
		own = _entities.get(Game.entity_id)
		if own != null and own.riding:
			mounted = true
			break
	await get_tree().create_timer(0.8).timeout
	own = _entities.get(Game.entity_id)
	var rows: Dictionary = own.equip_rows if own != null else {}
	var action: int = own._res.action if own != null and own.has_res else -1
	var parts: int = own._res.parts.size() if own != null and own.has_res else 0
	await _save_screenshot("user://logs/auto_ride.png")
	Game.ride(false)
	var down := false
	for i in 30:
		await get_tree().create_timer(0.1).timeout
		own = _entities.get(Game.entity_id)
		if own != null and not own.riding:
			down = true
			break
	var action_down: int = own._res.action if own != null and own.has_res else -1
	var parts_down: int = own._res.parts.size() if own != null and own.has_res else 0
	# the horse comes off again (0x08200311): a character logging in with it worn rides at once (0x080C1F83) and could not sit
	Game.item_unequip(10)
	await get_tree().create_timer(0.4).timeout
	Log.info("auto", "auto ride", {"mounted": mounted, "horse_row": rows.get(3, -1), "action": action, "parts": parts,
		"down": down, "action_down": action_down})
	print("AUTO_RIDE mounted=%s horse_row=%d action=%d parts=%d down=%s action_down=%d parts_down=%d" % [mounted, rows.get(3, -1),
		action, parts, down, action_down, parts_down])


# The task values (docs/LINUX-SERVER.md §21): the SYNC_FLAG values came with the spawn (G2C_TASK_VALUE each, the
# 1000..1070 batch); a CLIENT_FLAG id set by us (1276 - the 0xaa packet) comes back through a script SyncTaskValue, and a
# script SetTask on a SYNC_FLAG id (100) reaches us at once.  Prints AUTO_TASK so tools/dev.py screenshot can check it.
func _auto_task() -> void:
	var synced: int = Game.task_values.size()
	var v: int = int(Time.get_unix_time_from_system()) % 1000 + 1
	var got := {}
	var on_value := func(id: int, value: int) -> void:
		got[id] = value
	Game.task_value_changed.connect(on_value)
	Game.set_task_value(1276, v + 1)
	await get_tree().create_timer(0.3).timeout
	Game.chat("?gm ds SyncTaskValue(1276)")
	Game.chat("?gm ds SetTask(100, %d)" % v)
	for i in 30:
		await get_tree().create_timer(0.1).timeout
		if got.has(1276) and got.has(100):
			break
	# the task system (docs/LINUX-SERVER.md §22): StartTask opens a group among the temp values (2200 = the count, 2201 = the
	# id), SetTaskStatus sets two bits of value 2000 - every changed value reaches us as G2C_TASK_VALUE (0x0820E1E0)
	Game.chat("?gm ds CloseTask(TaskName(101))")
	await get_tree().create_timer(0.3).timeout
	Game.chat("?gm ds StartTask(TaskName(101))")
	Game.chat("?gm ds SetTaskStatus(TaskName(101), 2)")   # two writes: whatever the last run left, one of them changes value 2000
	Game.chat("?gm ds SetTaskStatus(TaskName(101), 1)")
	for i in 30:
		await get_tree().create_timer(0.1).timeout
		if got.has(2201) and got.has(2000):
			break
	Game.task_value_changed.disconnect(on_value)
	await _save_screenshot("user://logs/auto_task.png")
	print("AUTO_TASK packets=%d synced=%d client_set=%s script_set=%s expected=%d stored=%d task_count=%s task_id=%s status_value=%s" % [Game.task_packets, synced, str(got.get(1276, "-")), str(got.get(100, "-")), v, Game.task_value(100), str(got.get(2200, "-")), str(got.get(2201, "-")), str(got.get(2000, "-"))])


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
func _save_screenshot(path: String, half := false) -> void:
	if DisplayServer.get_name() == "headless":
		return
	await RenderingServer.frame_post_draw
	var img := get_viewport().get_texture().get_image()
	if half:
		img.resize(img.get_width() / 2, img.get_height() / 2, Image.INTERPOLATE_BILINEAR)
	var err := img.save_png(path)
	Log.info("auto", "screenshot", {"path": ProjectSettings.globalize_path(path), "error": error_string(err)})
	print("AUTO_SCREENSHOT " + ProjectSettings.globalize_path(path))
