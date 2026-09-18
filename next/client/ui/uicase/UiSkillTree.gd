# UiSkillTree - the mouse-skill tree of the VLTK 2.0 client (KUiSkillTree, 技能选择树.ini; gamecl.exe LoadScheme
# 0x00495C20, UpdateData 0x00495AE0, layout 0x00495A40, paint 0x00496170, WndProc 0x00495E10; docs/CLIENT-2.0.md §8).
# Lua Open([[leftskill]]) / Open([[rightskill]]) (0x0042EF50 cases 9 / 10 -> 0x00496400: +0x8b0 = 1 for the left)
# opens it - the two boxes of the bottom bar; while open, Open(...) again closes it (0x004957C0).
#   - [Main] LeftBtnPos (the lowest, leftmost button), RightBtnPos (read, but the layout 0x00495A40 anchors both sides
#     at LeftBtnPos), BtnSize (>= 1), KeyFont (12), KeyColor, MaxBtnCountPerRow.
#   - the entries: GDI 0x3f7 (left) / 0x3f8 (right): entry 0 = the current mouse skill, then every held skill of
#     level >= 1 the side lists (KUiSkillTreeLayout.listed) whose ReqLevel the character reaches, group = index / 8.
#   - a left click on an entry: OperationRequest(0xd, {genre, id}, right) = the skill becomes the mouse skill, the
#     tree hides; a right click or Esc hides it; the mouse over an entry shows the skill's tip (0x0044EBC0).
#   - each button also shows its shortcut key (ShortcutSkill(i) of the [ShortSkill] settings) in KeyFont / KeyColor -
#     not here yet: the F-key table is a per-character setting file.
extends "res://ui/elem/KWndWindow.gd"

const Layout := preload("res://ui/KUiSkillTreeLayout.gd")
const KFont := preload("res://ui/KFont.gd")

const SCHEME := "chon-ky-nang"
const GROUP_SIZE := 8   # 0x00623B0C: group = index / 8

signal picked(skill_id: int, right: bool)
signal hovered(skill_id: int)   # 0 = none

var right_side := false
var anchor := Vector2i(760, 650)
var btn := Vector2i(36, 36)
var max_per_row := 7
var key_font := 12
var key_color := Color.RED
var _entries: Array = []   # [{id, group, icon}]
var _cells: Array = []
var _rows := 0
var _skill_rows := {}      # skill id -> {style, aura, req_level, icon, name} of skills.json
var _tables_loaded := false
var _hover := -1
var shortcuts = null   # KUiShortcut of the window manager: the key letter painted on an entry that has a slot (0x0049627D)


func load_scheme(_screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null:
		return false
	name = "UiSkillTree"
	anchor = ini.get_integer2("Main", "LeftBtnPos", anchor)
	btn = ini.get_integer2("Main", "BtnSize", btn)
	if btn.x < 1:
		btn.x = 1
	if btn.y < 1:
		btn.y = 1
	key_font = ini.get_integer("Main", "KeyFont", 12)
	key_color = ini.get_color("Main", "KeyColor", Color.RED)
	max_per_row = ini.get_integer("Main", "MaxBtnCountPerRow", 7)
	mouse_filter = Control.MOUSE_FILTER_STOP
	visible = false
	return true


func _load_tables() -> void:
	if _tables_loaded:
		return
	_tables_loaded = true
	var tab = Assets.load_json("%s/skills.json" % Assets.assets_root())
	if tab is Dictionary:
		for r in tab.get("rows", []):
			var cells: Dictionary = r.get("cells", {})
			_skill_rows[int(r.get("id", 0))] = {
				"style": int(str(cells.get("SkillStyle", "0"))), "aura": int(str(cells.get("IsAura", "0"))) != 0,
				"req_level": int(str(cells.get("ReqLevel", "0"))), "icon": str(cells.get("SkillIcon", "")), "name": str(cells.get("SkillName", ""))}


# Open([[leftskill]]) / Open([[rightskill]]): opens for that side, or closes when already open (0x0042F0EB)
func toggle_for(right: bool) -> void:
	if visible:
		hide()
		return
	open_for(right)


func open_for(right: bool) -> void:
	_load_tables()
	right_side = right
	_entries.clear()
	var current: int = Game.right_skill if right else Game.left_skill
	_entries.append({"id": current, "group": 0, "icon": _skill_rows.get(current, {}).get("icon", "")})
	var level := int(Game.player_attrib.get("level", 1))
	var i := 1
	for id in Game.skills:
		var sk: Dictionary = Game.skills[id]
		if int(sk.get("level", 0)) < 1:
			continue
		var row: Dictionary = _skill_rows.get(int(id), {})
		if row.is_empty() or not Layout.listed(int(row.style), bool(row.aura), right):
			continue
		if int(row.req_level) > level:
			continue
		@warning_ignore("integer_division")
		_entries.append({"id": int(id), "group": i / GROUP_SIZE, "icon": str(row.icon)})
		i += 1
	var groups: Array = []
	for e in _entries:
		groups.append(e.group)
	var placed := Layout.place(groups, max_per_row)
	_cells = placed.cells
	_rows = int(placed.rows)
	var rect := Layout.window_rect(anchor, btn, _rows, int(placed.cols))
	position = Vector2(rect.position)
	size = Vector2(rect.size)
	_hover = -1
	visible = true
	queue_redraw()
	Log.info("ui", "skill tree", {"side": "right" if right else "left", "entries": _entries.size(), "rows": _rows})


func _draw() -> void:
	for i in _entries.size():
		var e: Dictionary = _entries[i]
		var at := Vector2(Layout.cell_pos(_cells[i], btn, _rows))
		draw_rect(Rect2(at, Vector2(btn)), Color(0, 0, 0, 0.55), true)
		if i == _hover:
			draw_rect(Rect2(at, Vector2(btn)), Color(1, 1, 0.6, 0.9), false, 1.0)
		if int(e.id) <= 0:
			continue
		var img = Assets.item_image(str(e.icon))
		if img != null:
			img.draw(self, at, 0)
		if shortcuts != null:
			var k: int = shortcuts.slot_of(int(e.id), right_side)
			if k >= 0:
				# the key letter in KeyFont / KeyColor (0x0049627D): the game font of that size, else the default font
				var font = KFont.of(key_font)
				if font != null:
					font.draw(self, at + Vector2(2, 1), shortcuts.key_of(k), key_color, Color.BLACK)
				else:
					draw_string(get_theme_default_font(), at + Vector2(2, key_font), shortcuts.key_of(k), HORIZONTAL_ALIGNMENT_LEFT, -1, key_font, key_color)


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.pressed:
		if event.button_index == MOUSE_BUTTON_LEFT:
			var i := Layout.hit(Vector2i(event.position), btn, _rows, _cells)
			if i >= 0 and int(_entries[i].id) > 0:
				picked.emit(int(_entries[i].id), right_side)
			hide()
			hovered.emit(0)
		elif event.button_index == MOUSE_BUTTON_RIGHT:
			hide()
			hovered.emit(0)
		accept_event()
	elif event is InputEventMouseMotion:
		var i := Layout.hit(Vector2i(event.position), btn, _rows, _cells)
		if i != _hover:
			_hover = i
			queue_redraw()
			hovered.emit(int(_entries[i].id) if i >= 0 else 0)


# the entry under the mouse (ShortcutSkill(k) with the tree open takes it): its skill id, 0 for none
func hovered_skill() -> int:
	if _hover < 0 or _hover >= _entries.size():
		return 0
	return int(_entries[_hover].id)


func _unhandled_key_input(event: InputEvent) -> void:
	if visible and event is InputEventKey and event.pressed and event.keycode == KEY_ESCAPE:
		hide()
		hovered.emit(0)
		get_viewport().set_input_as_handled()


func _notification(what: int) -> void:
	if what == NOTIFICATION_MOUSE_EXIT and _hover != -1:
		_hover = -1
		queue_redraw()
		hovered.emit(0)
