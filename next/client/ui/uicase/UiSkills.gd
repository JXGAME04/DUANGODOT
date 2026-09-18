# UiSkills - the skill book of the VLTK 2.0 client (KUiSkills, 技能主窗口.ini) with its fight page
# (KUiFightSkill, 战斗技能分页.ini), laid out the way gamecl.exe 2.0 lays it out (0x00494030 with the
# box pad 0x004933A0, docs/CLIENT-2.0.md §5):
#   - ten tier columns 51 px apart (0x33): [ImgColTitle] + [TxtColTitle] above each with the text
#     [ColTitle] Title_1..Title_10 (Thấp, Cao, Nhập môn, Lv10 .. Lv50, Trấn Phái, Lv60; the tiers
#     Lv120 / Lv150 of skillui.txt lie beyond the ten columns), nine vertical [ImgSplitLine] between them;
#   - three slot rows 58 px apart (0x3a): the cell picture [ImgSkillBG] at (6 + 51 c, 28 + 58 r), a 36 px
#     object box [Skill] at (8 + 51 c, 30 + 58 r), an [AddPointBtn] to its right (bottom aligned) while
#     skill points are left, the level under the box in [SkillText] (Color; ColorAddon 50,50,255 when
#     the current level is above the learned one, ColorCutdown 255,50,50 below);
#   - a skill's column and row come from settings/skillui/skillui.txt (skill_ui.json: skill -> tier, slot),
#     what the client core answered to GDI 0x414 (0x00607B40); RemainPoint shows the skill points left.
# A left click on a skill makes it the left mouse skill of the old client (GOI_SET_IMMDIA_SKILL 0), a right
# click the right one; the add-point button sends C2G_ADD_SKILL_POINT (GOI_TONE_UP_SKILL).
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndImage := preload("res://ui/elem/KWndImage.gd")
const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KWndLabeledButton := preload("res://ui/elem/KWndLabeledButton.gd")
const KWndObjContainer := preload("res://ui/elem/KWndObjContainer.gd")

const SCHEME := "ky-nang"
const SCHEME_FIGHT := "ky-nang-chien-dau"
const PAGE_BUTTONS := ["FightBtn", "LiveBtn", "CommonBtn"]
const PAGE_FIGHT := 0
const TIER_COLS := 10     # 0x00494030: ten columns
const SLOT_ROWS := 3      # three rows
const COL_PITCH := 51     # 0x33
const ROW_PITCH := 58     # 0x3a
const BASIC_ATTACKS := [1, 2]   # the plain attacks of the weapon: never a mouse skill of their own

signal closed
signal skill_clicked(skill_id: int)
signal skill_right_clicked(skill_id: int)
signal skill_hovered(skill)   # {id, name, level, ...} or null

var page := PAGE_FIGHT
var _page_buttons: Array = []
var _fight_page: Control = null
var _boxes: Array = []          # [slot * 10 + tier] -> KWndObjContainer
var _levels: Array = []         # the level text under each box
var _add_buttons: Array = []    # the add-point button beside each box
var _box_skill: Array = []      # the skill id in each box (0 = none)
var _remain := KWndText.new()
var _text_color := Color(218 / 255.0, 255 / 255.0, 165 / 255.0)
var _text_border := Color(23 / 255.0, 68 / 255.0, 0)
var _color_addon := Color(50 / 255.0, 50 / 255.0, 1.0)
var _color_cutdown := Color(1.0, 50 / 255.0, 50 / 255.0)
var _text_offset := Vector2i(0, 36)
var _text_font := 12
var _place := {}                # skill id (int) -> {"tier", "slot"}
var _rows := {}                 # skill id (int) -> {"name", "icon"} of skills.json
var _tables_loaded := false


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiSkills"
	var title := KWndText.new()
	add_child(title)
	title.init_from(ini, "Title")
	for i in PAGE_BUTTONS.size():
		var b := KWndLabeledButton.new()
		add_child(b)
		b.init_from(ini, PAGE_BUTTONS[i])
		b.toggled.connect(_on_page_button.bind(i))
		_page_buttons.append(b)
	var close := KWndButton.new()
	add_child(close)
	close.init_from(ini, "CloseBtn")
	close.clicked.connect(close_window)
	if not _build_fight_page():
		return false
	Game.skills_changed.connect(refresh)
	Game.skill_changed.connect(func(_id): refresh())
	Game.player_attrib_changed.connect(func(_a): refresh())
	_show_page(PAGE_FIGHT)
	return true


# KUiFightSkill::LoadScheme 0x00494030 + the box pad 0x004933A0 (mode 0: "Skill" / "AddPointBtn")
func _build_fight_page() -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME_FIGHT)
	if ini == null:
		return false
	_fight_page = Control.new()
	_fight_page.name = "FightPage"
	_fight_page.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_fight_page.position = Vector2(ini.get_integer("Main", "Left", 0), ini.get_integer("Main", "Top", 0))
	_fight_page.size = Vector2(ini.get_integer("Main", "Width", 0), ini.get_integer("Main", "Height", 0))
	add_child(_fight_page)
	# 0x00494120: the cell pictures, three rows of ten
	for row in SLOT_ROWS:
		for col in TIER_COLS:
			var bg := KWndImage.new()
			_fight_page.add_child(bg)
			bg.init_from(ini, "ImgSkillBG")
			bg.position += Vector2(COL_PITCH * col, ROW_PITCH * row)
	# 0x00494183: nine vertical lines, one per column boundary
	for col in TIER_COLS - 1:
		var line := KWndImage.new()
		_fight_page.add_child(line)
		line.init_from(ini, "ImgSplitLine")
		line.position.x += COL_PITCH * col
	# 0x004941F0: the tier label and its text above every column ([ColTitle] Title_%d, Tip_%d)
	for col in TIER_COLS:
		var img := KWndImage.new()
		_fight_page.add_child(img)
		img.init_from(ini, "ImgColTitle")
		img.position.x += COL_PITCH * col
		var txt := KWndText.new()
		_fight_page.add_child(txt)
		txt.init_from(ini, "TxtColTitle")
		txt.position.x += COL_PITCH * col
		txt.text = ini.get_string("ColTitle", "Title_%d" % (col + 1), "")
	# the skill points left (RemainPointTitle + RemainPoint)
	var remain_title := KWndText.new()
	_fight_page.add_child(remain_title)
	remain_title.init_from(ini, "RemainPointTitle")
	_fight_page.add_child(_remain)
	_remain.init_from(ini, "RemainPoint")
	_remain.text = "0"
	# [SkillText]: the level under the box
	_text_font = ini.get_integer("SkillText", "Font", 12)
	var off := ini.get_string("SkillText", "Offset", "0,36").split(",")
	if off.size() == 2:
		_text_offset = Vector2i(int(off[0]), int(off[1]))
	_text_color = ini.get_color("SkillText", "Color", _text_color)
	_text_border = ini.get_color("SkillText", "BorderColor", _text_border)
	_color_addon = ini.get_color("SkillText", "ColorAddon", _color_addon)
	_color_cutdown = ini.get_color("SkillText", "ColorCutdown", _color_cutdown)
	# 0x00493480: the boxes, row by row (box index = row * 10 + col), each with its add-point button and level
	for row in SLOT_ROWS:
		for col in TIER_COLS:
			var box := KWndObjContainer.new()
			_fight_page.add_child(box)
			box.init_from(ini, "Skill")
			box.position += Vector2(COL_PITCH * col, ROW_PITCH * row)
			var index := row * TIER_COLS + col
			box.object_clicked.connect(_on_box_clicked.bind(index))
			box.object_right_clicked.connect(_on_box_right_clicked.bind(index))
			box.hovered.connect(_on_box_hovered)
			_boxes.append(box)
			_box_skill.append(0)
			# 0x00493503: the add-point button sits right of the box, its bottom on the box's bottom
			var btn := KWndButton.new()
			_fight_page.add_child(btn)
			btn.init_from(ini, "AddPointBtn")
			btn.position = box.position + Vector2(box.size.x, box.size.y - btn.size.y)
			btn.visible = false
			btn.clicked.connect(_on_add_point.bind(index))
			_add_buttons.append(btn)
			var lvl := KWndText.new()
			_fight_page.add_child(lvl)
			lvl.font_size = _text_font
			lvl.halign = 1
			lvl.text_color = _text_color
			lvl.border_color = _text_border
			lvl.position = box.position + Vector2(_text_offset.x, _text_offset.y)
			lvl.size = Vector2(box.size.x, _text_font + 2)
			lvl.mouse_filter = Control.MOUSE_FILTER_IGNORE
			_levels.append(lvl)
	return true


func _load_tables() -> void:
	if _tables_loaded:
		return
	_tables_loaded = true
	var ui = Assets.load_json("%s/skill_ui.json" % Assets.assets_root())
	if ui is Dictionary:
		for key in ui.get("place", {}):
			var p: Dictionary = ui.place[key]
			_place[int(key)] = {"tier": int(p.get("tier", 0)), "slot": int(p.get("slot", 0))}
	var tab = Assets.load_json("%s/skills.json" % Assets.assets_root())
	if tab is Dictionary:
		for r in tab.get("rows", []):
			var cells: Dictionary = r.get("cells", {})
			_rows[int(r.get("id", 0))] = {"name": str(cells.get("SkillName", "")), "icon": str(cells.get("SkillIcon", ""))}
	Log.info("ui", "skill book tables", {"places": _place.size(), "rows": _rows.size()})


func skill_info(skill_id: int) -> Dictionary:
	_load_tables()
	return _rows.get(skill_id, {"name": str(skill_id), "icon": ""})


# KUiFightSkillPad::UpdateData 0x004938E0: every box cleared, then every held skill put where
# skillui.txt says (a skill the table does not know stays out of the book, like the old client)
func refresh() -> void:
	_load_tables()
	var points := int(Game.player_attrib.get("skill_point", 0))
	_remain.text = str(points)
	for i in _boxes.size():
		_boxes[i].set_objects([])
		_box_skill[i] = 0
		_levels[i].text = ""
		_add_buttons[i].visible = false
	for id in Game.skills:
		var sk: Dictionary = Game.skills[id]
		var place = _place.get(int(id))
		if place == null or int(place.tier) >= TIER_COLS or int(place.slot) >= SLOT_ROWS:
			continue
		var index: int = int(place.slot) * TIER_COLS + int(place.tier)
		var info := skill_info(int(id))
		_boxes[index].set_objects([{"id": int(id), "x": 0, "y": 0, "w": 1, "h": 1, "image": Assets.item_image(info.icon),
			"ex_type": 0, "usable": false, "name": info.name, "count": 0}])
		_box_skill[index] = int(id)
		var level: int = int(sk.get("level", 0))
		var current: int = int(sk.get("current_level", level))
		_levels[index].text = str(current) if current > 0 else ""
		_levels[index].text_color = _color_addon if current > level else (_color_cutdown if current < level else _text_color)
		_add_buttons[index].visible = points > 0 and level < int(sk.get("max_level", 0)) and not sk.get("only_inc", false)


func open_window() -> void:
	refresh()
	show_window()


func close_window() -> void:
	hide_window()
	closed.emit()


func toggle_window() -> void:
	if visible:
		close_window()
	else:
		open_window()


func _on_page_button(_checked: bool, index: int) -> void:
	# only the fight page exists so far: the other buttons spring back
	if index == PAGE_FIGHT:
		_show_page(index)
	else:
		_page_buttons[index].check(false)
		_page_buttons[page].check(true)


func _show_page(index: int) -> void:
	page = index
	for i in _page_buttons.size():
		_page_buttons[i].check(i == index)
	_fight_page.visible = index == PAGE_FIGHT


func _on_box_clicked(_o: Dictionary, index: int) -> void:
	if _box_skill[index] != 0:
		skill_clicked.emit(_box_skill[index])


func _on_box_right_clicked(_o: Dictionary, index: int) -> void:
	if _box_skill[index] != 0:
		skill_right_clicked.emit(_box_skill[index])


func _on_box_hovered(o) -> void:
	if o == null:
		skill_hovered.emit(null)
		return
	var sk = Game.skills.get(int(o.id))
	if sk == null:
		skill_hovered.emit(null)
		return
	var info := skill_info(int(o.id))
	skill_hovered.emit({"id": int(o.id), "name": info.name, "level": sk.level, "current_level": sk.current_level, "max_level": sk.max_level})


func _on_add_point(index: int) -> void:
	if _box_skill[index] == 0:
		return
	Game.add_skill_point(_box_skill[index])
	Log.info("ui", "skill point", {"skill": _box_skill[index]})
