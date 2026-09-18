# UiSkills - the skill book of the VLTK 2.0 client (KUiSkills, 技能主窗口.ini), laid out the way gamecl.exe 2.0
# lays it out (docs/CLIENT-2.0.md §6, read line by line):
#   - KUiSkills holds THREE fight pages (0x004953C0: three KUiFightSkill), one per branch of the character's
#     faction (0x00494EF0: pages[i].SetBranch(i)) - Shaolin's Quyền / Bổng / Đao - and one common page
#     (KUiCommonSkill, 0x00494540); the three branch buttons all come from [FightBtn] and stand 103 px (0x67)
#     apart (0x00494A65 / 0x00494AAF), [CommonBtn] is the fourth; [LiveBtn] is in the file but never loaded.
#   - KUiSkills::UpdateData 0x004947C0: button i gets the title of branch i of the faction last joined (GDI 0x413:
#     skillui.txt column 3 of the (faction, branch) row) and shows; a branch the faction lacks hides its button and
#     the common button slides into the first free slot; page 0 opens first, the common page when there is no branch 0.
#   - each fight page (KUiFightSkill::LoadScheme 0x00494030 + the box pad 0x004933A0, 战斗技能分页.ini): ten tier
#     columns 51 px apart (0x33) with [ImgColTitle] + [TxtColTitle] ([ColTitle] Title_1..10: Thấp, Cao, Nhập môn,
#     Lv10 .. Lv50, Trấn Phái, Lv60), nine [ImgSplitLine]; three slot rows 58 px apart (0x3a): [ImgSkillBG] at
#     (6 + 51 c, 28 + 58 r), a 36 px box [Skill], an [AddPointBtn] right of the box (bottom aligned) while skill
#     points are left, the level under the box in [SkillText] (ColorAddon above the learned level, ColorCutdown below).
#   - KUiFightSkillPad::UpdateData 0x004938E0: every held skill asks GDI 0x414 with skill x 10 + THE PAGE'S BRANCH
#     (0x0049393D) and lands in box [slot x 10 + tier] when that branch's row of skillui.txt names it - a skill of
#     another branch is not on this page, one named by two branches is on both.
#   - the common page (0x00493A40): the held skills no branch names, into the first empty box column by column
#     (tier outer, slot inner), rows 61 px apart (0x3d, mode 1 of the pad: [ImgCommonSkillBG], [CommonSkill],
#     [AddCommonPointBtn]), no column titles.
# A left click on a skill makes it the left mouse skill of the old client (GOI_SET_IMMDIA_SKILL 0), a right click the
# right one; the add-point button sends C2G_ADD_SKILL_POINT (GOI_TONE_UP_SKILL).
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndImage := preload("res://ui/elem/KWndImage.gd")
const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KWndLabeledButton := preload("res://ui/elem/KWndLabeledButton.gd")
const KWndObjContainer := preload("res://ui/elem/KWndObjContainer.gd")

const Layout := preload("res://ui/uicase/KUiSkillsLayout.gd")   # the numbers of the 2.0 book (tested headless)

const SCHEME := "ky-nang"
const SCHEME_FIGHT := "ky-nang-chien-dau"
const BRANCHES := Layout.BRANCHES            # 0x004953C0: three fight pages, three branch buttons
const PAGE_COMMON := Layout.PAGE_COMMON      # the fourth page
const TIER_COLS := Layout.TIER_COLS          # 0x00494030: ten columns
const SLOT_ROWS := Layout.SLOT_ROWS          # three rows
const COL_PITCH := Layout.COL_PITCH          # 0x33
const ROW_PITCH := Layout.ROW_PITCH          # 0x3a (mode 0: the fight pages)
const ROW_PITCH_COMMON := Layout.ROW_PITCH_COMMON   # 0x3d (mode 1: the common page)
const BASIC_ATTACKS := [1, 2]  # the plain attacks of the weapon: never a mouse skill of their own

signal closed
signal skill_clicked(skill_id: int)
signal skill_right_clicked(skill_id: int)
signal skill_hovered(skill)   # {id, name, level, ...} or null

var page := 0                  # 0..2 = the branch page, 3 = the common page
var _branch_buttons: Array = []
var _common_button: KWndLabeledButton = null
var _common_button_home := Vector2.ZERO   # [CommonBtn] as the file places it (the fourth slot)
var _pages: Array = []         # four dictionaries: root, boxes, levels, add_buttons, box_skill, remain
var _text_color := Color(218 / 255.0, 255 / 255.0, 165 / 255.0)
var _text_border := Color(23 / 255.0, 68 / 255.0, 0)
var _color_addon := Color(50 / 255.0, 50 / 255.0, 1.0)
var _color_cutdown := Color(1.0, 50 / 255.0, 50 / 255.0)
var _text_offset := Vector2i(0, 36)
var _text_font := 12
var _place := {}               # skill id (int) -> {branch (int) -> {"slot", "tier"}}: the map of 0x00607860
var _titles := {}              # faction id (int) -> [title 0, title 1, title 2]
var _rows := {}                # skill id (int) -> {"name", "icon"} of skills.json
var _tables_loaded := false


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiSkills"
	var title := KWndText.new()
	add_child(title)
	title.init_from(ini, "Title")
	# 0x00494A42: the three branch buttons from [FightBtn], each 103 px right of the last
	for i in BRANCHES:
		var b := KWndLabeledButton.new()
		add_child(b)
		b.init_from(ini, "FightBtn")
		b.position.x = Layout.branch_button_x(int(b.position.x), i)
		b.toggled.connect(_on_page_button.bind(i))
		_branch_buttons.append(b)
	_common_button = KWndLabeledButton.new()
	add_child(_common_button)
	_common_button.init_from(ini, "CommonBtn")
	_common_button_home = _common_button.position
	_common_button.toggled.connect(_on_page_button.bind(PAGE_COMMON))
	var close := KWndButton.new()
	add_child(close)
	close.init_from(ini, "CloseBtn")
	close.clicked.connect(close_window)
	for i in BRANCHES:
		var p := _build_page(false)
		if p.is_empty():
			return false
		_pages.append(p)
	var common := _build_page(true)
	if common.is_empty():
		return false
	_pages.append(common)
	Game.skills_changed.connect(refresh)
	Game.skill_changed.connect(func(_id): refresh())
	Game.player_attrib_changed.connect(func(_a): refresh())
	Game.faction_changed.connect(func(): refresh(true))   # the 0x7b handler tells the UI: UpdateData runs again
	_show_page(0)
	return true


# KUiFightSkill::LoadScheme 0x00494030 (common = false) / KUiCommonSkill::LoadScheme 0x00494540 (common = true) with
# the box pad 0x004933A0 (mode 0: "Skill" / "AddPointBtn", rows 58 px; mode 1: "CommonSkill" / "AddCommonPointBtn",
# rows 61 px).  Returns the page's controls, empty when the layout is missing.
func _build_page(common: bool) -> Dictionary:
	var ini: KUiScheme = KUiScheme.open(SCHEME_FIGHT)
	if ini == null:
		return {}
	var root := Control.new()
	root.name = "CommonPage" if common else "FightPage%d" % _pages.size()
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	root.position = Vector2(ini.get_integer("Main", "Left", 0), ini.get_integer("Main", "Top", 0))
	root.size = Vector2(ini.get_integer("Main", "Width", 0), ini.get_integer("Main", "Height", 0))
	add_child(root)
	var row_pitch := ROW_PITCH_COMMON if common else ROW_PITCH
	# 0x00494120 / 0x004945F2: the cell pictures, three rows of ten
	for row in SLOT_ROWS:
		for col in TIER_COLS:
			var bg := KWndImage.new()
			root.add_child(bg)
			bg.init_from(ini, "ImgCommonSkillBG" if common else "ImgSkillBG")
			bg.position += Vector2(COL_PITCH * col, row_pitch * row)
	# 0x00494183 / 0x00494660: nine vertical lines, one per column boundary
	for col in TIER_COLS - 1:
		var line := KWndImage.new()
		root.add_child(line)
		line.init_from(ini, "ImgSplitLine")
		line.position.x += COL_PITCH * col
	# 0x004941F0: the tier label and its text above every column of a fight page ([ColTitle] Title_%d); the common
	# page has none
	if not common:
		for col in TIER_COLS:
			var img := KWndImage.new()
			root.add_child(img)
			img.init_from(ini, "ImgColTitle")
			img.position.x += COL_PITCH * col
			var txt := KWndText.new()
			root.add_child(txt)
			txt.init_from(ini, "TxtColTitle")
			txt.position.x += COL_PITCH * col
			txt.text = ini.get_string("ColTitle", "Title_%d" % (col + 1), "")
	# the skill points left (RemainPointTitle + RemainPoint)
	var remain_title := KWndText.new()
	root.add_child(remain_title)
	remain_title.init_from(ini, "RemainPointTitle")
	var remain := KWndText.new()
	root.add_child(remain)
	remain.init_from(ini, "RemainPoint")
	remain.text = "0"
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
	var page_index := _pages.size()
	var p := {"root": root, "boxes": [], "levels": [], "add_buttons": [], "box_skill": [], "remain": remain}
	for row in SLOT_ROWS:
		for col in TIER_COLS:
			var box := KWndObjContainer.new()
			root.add_child(box)
			box.init_from(ini, "CommonSkill" if common else "Skill")
			box.position += Vector2(COL_PITCH * col, row_pitch * row)
			var index := Layout.box_index(row, col)
			box.object_clicked.connect(_on_box_clicked.bind(page_index, index))
			box.object_right_clicked.connect(_on_box_right_clicked.bind(page_index, index))
			box.hovered.connect(_on_box_hovered)
			p.boxes.append(box)
			p.box_skill.append(0)
			# 0x00493503: the add-point button sits right of the box, its bottom on the box's bottom
			var btn := KWndButton.new()
			root.add_child(btn)
			btn.init_from(ini, "AddCommonPointBtn" if common else "AddPointBtn")
			btn.position = box.position + Vector2(box.size.x, box.size.y - btn.size.y)
			btn.visible = false
			btn.clicked.connect(_on_add_point.bind(page_index, index))
			p.add_buttons.append(btn)
			var lvl := KWndText.new()
			root.add_child(lvl)
			lvl.font_size = _text_font
			lvl.halign = 1
			lvl.text_color = _text_color
			lvl.border_color = _text_border
			lvl.position = box.position + Vector2(_text_offset.x, _text_offset.y)
			lvl.size = Vector2(box.size.x, _text_font + 2)
			lvl.mouse_filter = Control.MOUSE_FILTER_IGNORE
			p.levels.append(lvl)
	return p


func _load_tables() -> void:
	if _tables_loaded:
		return
	_tables_loaded = true
	var ui = Assets.load_json("%s/skill_ui.json" % Assets.assets_root())
	if ui is Dictionary:
		for key in ui.get("place", {}):
			var branches: Dictionary = ui.place[key]
			var by_branch := {}
			for b in branches:
				var p: Dictionary = branches[b]
				by_branch[int(b)] = {"slot": int(p.get("slot", 0)), "tier": int(p.get("tier", 0))}
			_place[int(key)] = by_branch
		for key in ui.get("titles", {}):
			var t: Array = ui.titles[key]
			_titles[int(key)] = [str(t[0]) if t.size() > 0 else "", str(t[1]) if t.size() > 1 else "", str(t[2]) if t.size() > 2 else ""]
	var tab = Assets.load_json("%s/skills.json" % Assets.assets_root())
	if tab is Dictionary:
		for r in tab.get("rows", []):
			var cells: Dictionary = r.get("cells", {})
			_rows[int(r.get("id", 0))] = {"name": str(cells.get("SkillName", "")), "icon": str(cells.get("SkillIcon", ""))}
	Log.info("ui", "skill book tables", {"places": _place.size(), "factions": _titles.size(), "rows": _rows.size()})


func skill_info(skill_id: int) -> Dictionary:
	_load_tables()
	return _rows.get(skill_id, {"name": str(skill_id), "icon": ""})


# GDI 0x414 for one page: where a skill sits on the page of `branch` (null when that branch's row lacks it)
func place_of(skill_id: int, branch: int):
	_load_tables()
	var by_branch = _place.get(skill_id)
	if by_branch == null:
		return null
	return by_branch.get(branch)


# the branches (0..2) whose page shows the skill; empty for a common skill
func branches_of(skill_id: int) -> Array:
	_load_tables()
	var out := []
	var by_branch = _place.get(skill_id)
	if by_branch != null:
		for b in by_branch:
			out.append(int(b))
	out.sort()
	return out


# GDI 0x413: the branch titles of the faction the character joined last (PlayerData+0x12080), "" for a missing one
func branch_titles() -> Array:
	_load_tables()
	return _titles.get(int(Game.faction_last), ["", "", ""])


# KUiSkills::UpdateData 0x004947C0 + KUiFightSkillPad::UpdateData 0x004938E0 for every page + the common page 0x00493A40.
# reset_page: UpdateData ends by activating the default page (0x004948DC) - on opening and when the faction moves; a
# skill's level change (0x00494B40) leaves the page alone
func refresh(reset_page: bool = false) -> void:
	_load_tables()
	var titles := branch_titles()
	# the buttons: a titled branch shows its button; the common button takes the first free slot (0x00494878)
	var common_x := _common_button_home.x
	var first_page := -1
	for i in BRANCHES:
		var has: bool = str(titles[i]) != ""
		_branch_buttons[i].visible = has
		if has:
			_branch_buttons[i].set_label(titles[i])
			if first_page < 0 and i == 0:
				first_page = 0
		elif common_x == _common_button_home.x:
			common_x = _branch_buttons[i].position.x
	_common_button.position.x = common_x
	if first_page < 0:
		first_page = PAGE_COMMON
	var points := int(Game.player_attrib.get("skill_point", 0))
	for p in _pages:
		p.remain.text = str(points)
		for i in p.boxes.size():
			p.boxes[i].set_objects([])
			p.box_skill[i] = 0
			p.levels[i].text = ""
			p.add_buttons[i].visible = false
	var common_n := 0
	for id in Game.skills:
		var sk: Dictionary = Game.skills[id]
		var on_page := false
		for b in BRANCHES:
			var place = place_of(int(id), b)
			if place == null or int(place.tier) >= TIER_COLS or int(place.slot) >= SLOT_ROWS:
				continue
			on_page = true
			_put(_pages[b], Layout.box_index(int(place.slot), int(place.tier)), int(id), sk, points)
		if not on_page and common_n < SLOT_ROWS * TIER_COLS:
			_put(_pages[PAGE_COMMON], Layout.common_fill_index(common_n), int(id), sk, points)
			common_n += 1
	# the page shown: the default one (0x004948DC) on an update, else the current one while its button is up
	if reset_page or not (page == PAGE_COMMON or (page < BRANCHES and _branch_buttons[page].visible)):
		_show_page(first_page)
	else:
		_show_page(page)


func _put(p: Dictionary, index: int, id: int, sk: Dictionary, points: int) -> void:
	var info := skill_info(id)
	p.boxes[index].set_objects([{"id": id, "x": 0, "y": 0, "w": 1, "h": 1, "image": Assets.item_image(info.icon),
		"ex_type": 0, "usable": false, "name": info.name, "count": 0}])
	p.box_skill[index] = id
	var level: int = int(sk.get("level", 0))
	var current: int = int(sk.get("current_level", level))
	p.levels[index].text = str(current) if current > 0 else ""
	p.levels[index].text_color = _color_addon if current > level else (_color_cutdown if current < level else _text_color)
	p.add_buttons[index].visible = points > 0 and level < int(sk.get("max_level", 0)) and not sk.get("only_inc", false)


func open_window() -> void:
	refresh(true)
	show_window()


func close_window() -> void:
	hide_window()
	closed.emit()


func toggle_window() -> void:
	if visible:
		close_window()
	else:
		open_window()


# KUiSkills::WndProc 0x00494D10: a branch button opens its page, the common button the common page
func _on_page_button(_checked: bool, index: int) -> void:
	if index < BRANCHES and not _branch_buttons[index].visible:
		_show_page(page)
		return
	_show_page(index)


func _show_page(index: int) -> void:
	page = index
	for i in BRANCHES:
		_branch_buttons[i].check(i == index)
	_common_button.check(index == PAGE_COMMON)
	for i in _pages.size():
		_pages[i].root.visible = i == index


func _on_box_clicked(_o: Dictionary, page_index: int, index: int) -> void:
	var id: int = _pages[page_index].box_skill[index]
	if id != 0:
		skill_clicked.emit(id)


func _on_box_right_clicked(_o: Dictionary, page_index: int, index: int) -> void:
	var id: int = _pages[page_index].box_skill[index]
	if id != 0:
		skill_right_clicked.emit(id)


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


func _on_add_point(page_index: int, index: int) -> void:
	var id: int = _pages[page_index].box_skill[index]
	if id == 0:
		return
	Game.add_skill_point(id)
	Log.info("ui", "skill point", {"skill": id})
