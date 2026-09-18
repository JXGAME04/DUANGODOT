# KUiGameWindows - the windows of the game screen (the part of Ui\UiBase.cpp that opens and
# closes the item / status windows and keeps the item on the cursor).
#
# A CanvasLayer over the world: the bag (UiItem), the character window (UiStatus), the tooltip
# (UiMouseHover) and the item on the cursor (KUiDraggedObject).  The windows are laid out for the
# theme's screen (1024x768) and drawn at that size on whatever the viewport is; they open at the
# Left/Top of their layout.  Keys: I the bag, C the character window (Alt+ the same), Escape puts
# a lifted item back and closes the window on top.
extends CanvasLayer

const UiItem := preload("res://ui/uicase/UiItem.gd")
const UiStatus := preload("res://ui/uicase/UiStatus.gd")
const UiSkills := preload("res://ui/uicase/UiSkills.gd")
const UiMouseHover := preload("res://ui/uicase/UiMouseHover.gd")
const UiControlBar := preload("res://ui/uicase/UiControlBar.gd")
const UiPlayerBar := preload("res://ui/uicase/UiPlayerBar.gd")
const UiSkillTree := preload("res://ui/uicase/UiSkillTree.gd")
const UiSkillState := preload("res://ui/uicase/UiSkillState.gd")
const KUiShortcut := preload("res://ui/KUiShortcut.gd")
const KUiDraggedObject := preload("res://ui/KUiDraggedObject.gd")
const KUiItemView := preload("res://ui/KUiItemView.gd")
const KUiScheme := preload("res://ui/KUiScheme.gd")

var item_window: UiItem = null
var status_window: UiStatus = null
var skills_window: UiSkills = null
# the bars of the 2.0 screen (docs/CLIENT-2.0.md §7): the top bar (life / mana / stamina / exp / level), the tool bar
# (the buttons that open the windows) and the bottom bar (quick items, the two mouse skills, the chat line)
var top_bar: UiControlBar = null
var tool_bar: UiControlBar = null
var player_bar: UiPlayerBar = null
var skill_tree: UiSkillTree = null   # the mouse-skill tree (Open([[leftskill]]) / Open([[rightskill]]))
var state_window: UiSkillState = null   # the skill state list under the top bar (技能状态列表.ini)
var shortcuts := KUiShortcut.new()      # the nine shortcut skills (Q W E A S D Z X C), kept per character
var hover: UiMouseHover = null
var hand: KUiDraggedObject = null
var ready_ok := false
var screen := Vector2i(1024, 768)
var _canvas: Control = null


func _ready() -> void:
	layer = 10
	_canvas = Control.new()
	_canvas.name = "Canvas"
	_canvas.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_canvas.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(_canvas)
	screen = Vector2i(get_viewport().get_visible_rect().size)
	var probe = KUiScheme.open(UiItem.SCHEME)
	if probe == null:
		Log.warn("ui", "item windows missing", {"hint": "python tools/dev.py assets (jxassets export-ui)"})
		return
	item_window = UiItem.new()
	status_window = UiStatus.new()
	skills_window = UiSkills.new()
	hover = UiMouseHover.new()
	hand = KUiDraggedObject.new()
	_build_bars()
	for w in [item_window, status_window, skills_window]:
		_canvas.add_child(w)
		if not w.load_scheme(screen):
			Log.error("ui", "layout missing", {"window": w.SCHEME})
			return
	skill_tree = UiSkillTree.new()
	_canvas.add_child(skill_tree)
	if not skill_tree.load_scheme(screen):
		Log.warn("ui", "layout missing", {"window": UiSkillTree.SCHEME})
		skill_tree.queue_free()
		skill_tree = null
	else:
		skill_tree.picked.connect(_on_skill_clicked)
		skill_tree.hovered.connect(_on_tree_hovered)
		skill_tree.shortcuts = shortcuts
	_load_shortcuts()
	_canvas.add_child(hover)
	hover.load_scheme(screen)
	_canvas.add_child(hand)
	item_window.open_status.connect(func(): status_window.open_window())
	status_window.open_item.connect(func(): item_window.open_window())
	item_window.item_hovered.connect(_on_item_hovered)
	status_window.item_hovered.connect(_on_item_hovered)
	item_window.item_lift.connect(_lift)
	status_window.item_lift.connect(_lift)
	item_window.item_put.connect(_put_in_bag)
	status_window.item_put_on.connect(_put_on)
	# the skill book: a left click picks the left mouse skill (GOI_SET_IMMDIA_SKILL 0 of the old client), a right
	# click the right one; the tooltip shows the name and level
	skills_window.skill_clicked.connect(_on_skill_clicked.bind(false))
	skills_window.skill_right_clicked.connect(_on_skill_clicked.bind(true))
	skills_window.skill_hovered.connect(_on_skill_hovered)
	Game.player_attrib_changed.connect(func(_a): _refresh_bars())
	Game.skills_changed.connect(_refresh_mouse_skills)
	Game.mouse_skill_changed.connect(_refresh_mouse_skills)
	Game.skill_changed.connect(func(_id): _refresh_mouse_skills())
	_refresh_bars()
	_refresh_mouse_skills()
	Game.item_changed.connect(_on_item_changed)
	Game.item_removed.connect(_on_item_removed)
	Game.item_result.connect(_on_item_result)
	ready_ok = true
	Log.info("ui", "item windows ready", {"screen": "%dx%d" % [screen.x, screen.y]})


func _unhandled_input(event: InputEvent) -> void:
	if not ready_ok or not (event is InputEventKey) or not event.pressed or event.echo:
		return
	# the keys of autoexec.lua of the 2.0 client: F3 status, F4 items, F5 skills, M horse; Q W E A S D Z X C the shortcut
	# skills (ShortcutSkill(0..8)); I and K stay as the plain client had them
	var k := KUiShortcut.slot_of_key(event.keycode)
	if k >= 0:
		_shortcut_key(k)
		get_viewport().set_input_as_handled()
		return
	match event.keycode:
		KEY_I, KEY_F4:
			item_window.toggle_window()
			get_viewport().set_input_as_handled()
		KEY_F3:
			status_window.toggle_window()
			get_viewport().set_input_as_handled()
		KEY_K, KEY_F5:
			skills_window.toggle_window()
			get_viewport().set_input_as_handled()
		KEY_M:
			Game.ride(not bool(Game.entities.get(Game.entity_id, {}).get("riding", false)))
			get_viewport().set_input_as_handled()
		KEY_ESCAPE:
			if hand.holding():
				_drop_hand()
				get_viewport().set_input_as_handled()
			elif item_window.visible or status_window.visible or skills_window.visible:
				item_window.hide_window()
				status_window.hide_window()
				skills_window.hide_window()
				get_viewport().set_input_as_handled()


func any_open() -> bool:
	return ready_ok and (item_window.visible or status_window.visible or skills_window.visible or (skill_tree != null and skill_tree.visible))


func _on_item_hovered(item) -> void:
	if item == null or hand.holding():
		hover.hide_lines()
	else:
		# KItem::GetDesc, then the "\n" the CoreShell wrapper (0x006b66a0) adds after it
		hover.show_text(KUiItemView.describe_text(item, KUiItemView.equip_enhance(item)) + "\n", _canvas.get_local_mouse_position())


# KUiPlayerBar::LoadScheme 0x004750E0 (the bottom bar, then [Main] ToolBoxSchema = the tool bar 0x00474B20) and the
# top bar 顶部控制条.ini: the bars sit under the windows; a missing layout leaves that bar out
func _build_bars() -> void:
	player_bar = UiPlayerBar.new()
	_canvas.add_child(player_bar)
	if not player_bar.load_scheme(screen):
		Log.warn("ui", "layout missing", {"window": UiPlayerBar.SCHEME})
		player_bar.queue_free()
		player_bar = null
	else:
		player_bar.mouse_skill_clicked.connect(func(right): if skill_tree != null: skill_tree.toggle_for(right))
	top_bar = UiControlBar.new()
	_canvas.add_child(top_bar)
	if not top_bar.load_scheme("thanh-dieu-khien-tren", screen):
		Log.warn("ui", "layout missing", {"window": "thanh-dieu-khien-tren"})
		top_bar.queue_free()
		top_bar = null
	tool_bar = UiControlBar.new()
	_canvas.add_child(tool_bar)
	if not tool_bar.load_scheme("thanh-cong-cu", screen, true):
		Log.warn("ui", "layout missing", {"window": "thanh-cong-cu"})
		tool_bar.queue_free()
		tool_bar = null
	else:
		tool_bar.command.connect(_on_bar_command)
	state_window = UiSkillState.new()
	_canvas.add_child(state_window)
	if not state_window.load_scheme(screen):
		Log.warn("ui", "layout missing", {"window": UiSkillState.SCHEME})
		state_window.queue_free()
		state_window = null
	else:
		Game.state_changed.connect(func(_id): state_window.refresh())
		state_window.state_hovered.connect(_on_state_hovered)


# the Lua Open([[x]]) / Switch([[x]]) a tool bar button runs (0x0044B250..): the windows this client has
func _on_bar_command(cmd: String) -> void:
	match cmd:
		"status":
			status_window.toggle_window()
		"items":
			item_window.toggle_window()
		"skills":
			skills_window.toggle_window()
		_:
			Log.info("ui", "window not built yet", {"command": cmd})


func _refresh_bars() -> void:
	if top_bar != null:
		top_bar.refresh()
	if tool_bar != null:
		tool_bar.refresh()


# the two mouse skill boxes of the bottom bar (ImediaLeftSkill / ImediaRightSkill)
func _refresh_mouse_skills() -> void:
	if player_bar == null:
		return
	var left := {}
	var right := {}
	if Game.left_skill > 0 and Game.skills.has(Game.left_skill):
		left = skills_window.skill_info(Game.left_skill)
		left["id"] = Game.left_skill
	if Game.right_skill > 0 and Game.skills.has(Game.right_skill):
		right = skills_window.skill_info(Game.right_skill)
		right["id"] = Game.right_skill
	player_bar.set_mouse_skills(left, right)


func _on_skill_clicked(skill_id: int, right: bool) -> void:
	if right:
		Game.right_skill = skill_id
	else:
		Game.left_skill = skill_id
	Log.info("ui", "mouse skill", {"skill": skill_id, "button": "right" if right else "left"})
	_refresh_mouse_skills()


# the tip of a state icon: "name\ndesc\ntime" (0x0041ECE4)
# ShortcutSkill(k) 0x00495F70: with the tree open the hovered entry takes slot k; closed, slot k becomes the mouse skill
# of its side (OperationRequest(0xd, entry, side))
func _shortcut_key(k: int) -> void:
	if skill_tree != null and skill_tree.visible:
		var id := skill_tree.hovered_skill()
		if id > 0:
			assign_shortcut(k, id, skill_tree.right_side)
		return
	var s: Dictionary = shortcuts.slot(k)
	if int(s.id) <= 0 or not Game.skills.has(int(s.id)):
		return
	_on_skill_clicked(int(s.id), bool(s.right))


func assign_shortcut(k: int, skill_id: int, right: bool) -> void:
	if shortcuts.assign(k, skill_id, right):
		Log.info("ui", "shortcut skill", {"slot": k, "key": KUiShortcut.key_of(k), "skill": skill_id, "side": "right" if right else "left"})
		_save_shortcuts()
		if skill_tree != null:
			skill_tree.queue_redraw()


# [ShortSkill] ShortcutSkill_%d of the character's settings (0x0052D720): here user://shortcuts_<player id>.json
func _shortcut_path() -> String:
	return "user://shortcuts_%d.json" % int(Game.player_id)


func _load_shortcuts() -> void:
	var path := _shortcut_path()
	if not FileAccess.file_exists(path):
		return
	var f := FileAccess.open(path, FileAccess.READ)
	if f == null:
		return
	var parsed = JSON.parse_string(f.get_as_text())
	f.close()
	shortcuts.from_json(parsed)


func _save_shortcuts() -> void:
	var f := FileAccess.open(_shortcut_path(), FileAccess.WRITE)
	if f == null:
		return
	f.store_string(JSON.stringify(shortcuts.to_json()))
	f.close()


func _on_state_hovered(text: String) -> void:
	if text == "":
		hover.hide_lines()
	else:
		hover.show_text(text + "\n", _canvas.get_local_mouse_position())


func _on_tree_hovered(skill_id: int) -> void:
	if skill_id <= 0 or not Game.skills.has(skill_id):
		hover.hide_lines()
		return
	var sk = Game.skills[skill_id]
	var info := skills_window.skill_info(skill_id)
	hover.show_text("%s  %d/%d\n" % [info.name, int(sk.current_level), int(sk.max_level)], _canvas.get_local_mouse_position())


func _on_skill_hovered(skill) -> void:
	if skill == null:
		hover.hide_lines()
	else:
		hover.show_text("%s  %d/%d\n" % [skill.name, int(skill.current_level), int(skill.max_level)], _canvas.get_local_mouse_position())


# A click on an item with nothing on the cursor lifts it (Wnd_SetDragObj).
func _lift(item: Dictionary) -> void:
	hand.lift(item)
	hover.hide_lines()
	_tell_hand()
	Log.debug("item", "item lifted", {"id": item.id, "name": item.name})


func _drop_hand() -> void:
	hand.drop()
	_tell_hand()


func _tell_hand() -> void:
	item_window.set_hand(hand.cells)
	status_window.set_hand(hand.cells)


# A click in the bag with an item on the cursor: the zone decides (C2G_ITEM_MOVE); the item
# stays on the cursor until G2C_ITEM_MOVE says where it went or G2C_ITEM_RESULT says no.
func _put_in_bag(x: int, y: int) -> void:
	if not hand.holding():
		return
	hand.pending_seq = Game.item_move(hand.item_id, Game.ROOM_BAG, x, y)


func _put_on(part: int) -> void:
	if not hand.holding():
		return
	hand.pending_seq = Game.item_equip(hand.item_id, part)


func _on_item_changed(item: Dictionary) -> void:
	if hand.holding() and int(item.id) == hand.item_id:
		_drop_hand()


func _on_item_removed(id: int) -> void:
	if hand.holding() and id == hand.item_id:
		_drop_hand()


func _on_item_result(seq: int, result: int) -> void:
	if hand.holding() and seq == hand.pending_seq:
		hand.pending_seq = 0   # still on the cursor: the player may try elsewhere
	Log.debug("item", "item request answered", {"seq": seq, "result": result})
