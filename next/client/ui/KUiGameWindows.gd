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
const KUiDraggedObject := preload("res://ui/KUiDraggedObject.gd")
const KUiItemView := preload("res://ui/KUiItemView.gd")
const KUiScheme := preload("res://ui/KUiScheme.gd")

var item_window: UiItem = null
var status_window: UiStatus = null
var skills_window: UiSkills = null
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
	for w in [item_window, status_window, skills_window]:
		_canvas.add_child(w)
		if not w.load_scheme(screen):
			Log.error("ui", "layout missing", {"window": w.SCHEME})
			return
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
	Game.item_changed.connect(_on_item_changed)
	Game.item_removed.connect(_on_item_removed)
	Game.item_result.connect(_on_item_result)
	ready_ok = true
	Log.info("ui", "item windows ready", {"screen": "%dx%d" % [screen.x, screen.y]})


func _unhandled_input(event: InputEvent) -> void:
	if not ready_ok or not (event is InputEventKey) or not event.pressed or event.echo:
		return
	match event.keycode:
		KEY_I:
			item_window.toggle_window()
			get_viewport().set_input_as_handled()
		KEY_C:
			status_window.toggle_window()
			get_viewport().set_input_as_handled()
		KEY_K:
			skills_window.toggle_window()
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
	return ready_ok and (item_window.visible or status_window.visible or skills_window.visible)


func _on_item_hovered(item) -> void:
	if item == null or hand.holding():
		hover.hide_lines()
	else:
		# KItem::GetDesc, then the "\n" the CoreShell wrapper (0x006b66a0) adds after it
		hover.show_text(KUiItemView.describe_text(item, KUiItemView.equip_enhance(item)) + "\n", _canvas.get_local_mouse_position())


func _on_skill_clicked(skill_id: int, right: bool) -> void:
	if right:
		Game.right_skill = skill_id
	else:
		Game.left_skill = skill_id
	Log.info("ui", "mouse skill", {"skill": skill_id, "button": "right" if right else "left"})


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
