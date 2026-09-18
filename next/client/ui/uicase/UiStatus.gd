# KUiStatus - the character window (Ui\UiCase\UiStatus.cpp; layout <theme>\玩家装备与人物状态.ini
# with one file per page: _装备.ini the equipment, _属性.ini the attributes).
#
# The frame has the page buttons (check boxes: one down at a time), [Close] and [Item] (opens the
# bag).  The equipment page is fifteen KWndObjectBox slots, one per ITEM_PART, over the [Male] /
# [Female] backdrop of the character's sex, with the labels [TitleText_N] of the layout.  A click
# on a worn piece lifts it onto the cursor (it can then be put in the bag), a click with a piece
# on the cursor asks the zone to wear it there (C2G_ITEM_EQUIP), a double click or a right click
# takes it off (C2G_ITEM_UNEQUIP).  The attribute page shows what the client knows of its
# character so far (name, level, life); the numbers of the attribute system come with M12.
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndImage := preload("res://ui/elem/KWndImage.gd")
const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KWndLabeledButton := preload("res://ui/elem/KWndLabeledButton.gd")
const KWndObjContainer := preload("res://ui/elem/KWndObjContainer.gd")
const KUiItemView := preload("res://ui/KUiItemView.gd")

const SCHEME := "thong-tin-nhan-vat"
const SCHEME_EQUIP := "thong-tin-nhan-vat-trang-bi"
const SCHEME_ATTRIB := "thong-tin-nhan-vat-thuoc-tinh"
const PAGE_BUTTONS := ["BtnAttribPage", "BtnEquipPage", "BtnJudgePage", "BtnMeridianPage"]
const PAGE_ATTRIB := 0
const PAGE_EQUIP := 1
# the slot sections of the equipment page and the ITEM_PART each one holds (GameDataDef.h)
const SLOT_PARTS := {
	"Cap": 0, "Cloth": 1, "Sash": 2, "Weapon": 3, "Shoes": 4, "Bangle": 5, "Necklace": 6,
	"Ring1": 7, "Ring2": 8, "Pendant": 9, "Horse": 10, "Mask": 11, "Mantle": 12, "Signet": 13, "Shipin": 14,
	"Seal": -1, "FactionPendant": -1,   # JX2 slots beyond the fifteen parts: shown, never filled
}
const ATTRIB_VALUES := ["Name", "Title", "Luck", "Level", "Prestige", "WorldRank", "Life", "Mana", "Stamina", "Status",
	"Strength", "Vitality", "Dexterity", "Energy", "Exp", "LeftDamage", "RightDamage", "Attack", "Defense",
	"MoveSpeed", "AttackSpeed", "RemainPoint", "ResistPhy", "ResistCold", "ResistLighting", "ResistFire", "ResistPoison"]

signal open_item
signal closed
signal item_hovered(item)
signal item_lift(item: Dictionary)
signal item_put_on(part: int)      # a piece on the cursor was clicked onto a slot

var page := PAGE_ATTRIB
var slots := {}                    # section -> KWndObjContainer
var _page_buttons: Array = []
var _equip_page: Control = null
var _attrib_page: Control = null
var _male: KWndImage = null
var _female: KWndImage = null
var _equip_texts := {}
var _attrib_texts := {}
var _screen := Vector2i(1024, 768)


func load_scheme(screen: Vector2i) -> bool:
	_screen = screen
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiStatus"
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
	close.init_from(ini, "Close")
	close.clicked.connect(close_window)
	var item := KWndButton.new()
	add_child(item)
	item.init_from(ini, "Item")
	item.clicked.connect(func(): open_item.emit())
	if not _build_equip_page() or not _build_attrib_page():
		return false
	Game.items_changed.connect(refresh)
	Game.item_changed.connect(func(_item): refresh())
	Game.item_removed.connect(func(_id): refresh())
	Game.entity_life.connect(func(_l): _fill_attribs())
	_show_page(ini.get_integer("Init", "InitPage", 0))
	return true


func _build_equip_page() -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME_EQUIP)
	if ini == null:
		return false
	_equip_page = Control.new()
	_equip_page.name = "EquipPage"
	_equip_page.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_equip_page)
	_male = KWndImage.new()
	_equip_page.add_child(_male)
	_male.init_from(ini, "Male")
	_female = KWndImage.new()
	_equip_page.add_child(_female)
	_female.init_from(ini, "Female")
	# [Male] / [Female] are the page window itself (Left/Top from the frame's corner, 4,49); the
	# parts of the page are placed from the page's corner, so the labels sit in their slots
	_equip_page.position = _male.position
	_equip_page.size = _male.size
	_male.position = Vector2.ZERO
	_female.position = Vector2.ZERO
	var count := ini.get_integer("Main", "TitleTextCount", 20)
	for i in count:
		var t := KWndText.new()
		_equip_page.add_child(t)
		t.init_from(ini, "TitleText_%d" % i)
	for section in ["Name", "PKValue", "TransLife", "Title"]:
		var t := KWndText.new()
		_equip_page.add_child(t)
		t.init_from(ini, section)
		_equip_texts[section] = t
	for section in SLOT_PARTS:
		var box := KWndObjContainer.new()
		_equip_page.add_child(box)
		if not box.init_from(ini, section):
			continue
		box.accept_free = true
		box.object_clicked.connect(_on_slot_clicked.bind(section))
		box.object_double_clicked.connect(_on_slot_used.bind(section))
		box.object_right_clicked.connect(_on_slot_used.bind(section))
		box.put_requested.connect(func(_x: int, _y: int): _on_slot_put(section))
		box.hovered.connect(func(o): item_hovered.emit(Game.items.get(int(o.id)) if o != null else null))
		slots[section] = box
	for section in ["BtnLock", "BtnBindItem", "BtnLockItem"]:
		var b := KWndLabeledButton.new()
		_equip_page.add_child(b)
		b.init_from(ini, section)
	for section in ["BtnHorseManage", "BtnTitleManage"]:
		var b := KWndButton.new()
		_equip_page.add_child(b)
		b.init_from(ini, section)
	return true


func _build_attrib_page() -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME_ATTRIB)
	if ini == null:
		return false
	_attrib_page = Control.new()
	_attrib_page.name = "AttribPage"
	_attrib_page.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(_attrib_page)
	var bg := KWndImage.new()
	_attrib_page.add_child(bg)
	bg.init_from(ini, "Main")
	_attrib_page.position = bg.position   # [Main] of the page: its window inside the frame
	_attrib_page.size = bg.size
	bg.position = Vector2.ZERO
	var count := ini.get_integer("Main", "TextTitleCount", 27)
	for i in count:
		var t := KWndText.new()
		_attrib_page.add_child(t)
		t.init_from(ini, "TextTitle_%d" % i)
	for section in ATTRIB_VALUES:
		var t := KWndText.new()
		_attrib_page.add_child(t)
		t.init_from(ini, section)
		_attrib_texts[section] = t
	for section in ["AddStrength", "AddVitality", "AddDexterity", "AddEnergy"]:
		var b := KWndButton.new()
		_attrib_page.add_child(b)
		b.init_from(ini, section)
		b.enable(false)
	return true


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
	# only the two pages that exist can be shown; the others stay as they were
	if index == PAGE_ATTRIB or index == PAGE_EQUIP:
		_show_page(index)
	else:
		_page_buttons[index].check(false)
		_page_buttons[page].check(true)


func _show_page(index: int) -> void:
	page = index
	for i in _page_buttons.size():
		_page_buttons[i].check(i == index)
	_equip_page.visible = index == PAGE_EQUIP
	_attrib_page.visible = index == PAGE_ATTRIB
	refresh()


# What is worn, slot by slot, and the character's numbers.
func refresh() -> void:
	if _equip_page == null:
		return
	var me = Game.entities.get(Game.entity_id)
	var female: bool = me != null and int(me.get("sex", 0)) == 1
	_male.visible = not female
	_female.visible = female
	for section in slots:
		var part: int = SLOT_PARTS[section]
		var list: Array = []
		if part >= 0:
			var id := Game.item_worn(part)
			if id != 0:
				var o := KUiItemView.object_of(Game.items[id])
				o.x = 0
				o.y = 0
				o.w = 1
				o.h = 1
				list.append(o)
		slots[section].set_objects(list)
	if _equip_texts.has("Name"):
		_equip_texts["Name"].text = str(me.get("name", "")) if me != null else ""
	_fill_attribs()


func _fill_attribs() -> void:
	var me = Game.entities.get(Game.entity_id)
	var v := {}
	if me != null:
		v["Name"] = str(me.get("name", ""))
		v["Level"] = str(int(me.get("level", 1)))
		v["Life"] = "%d/%d" % [int(me.get("life", 0)), int(me.get("life_max", 0))]
		v["MoveSpeed"] = str(int(me.get("speed", 0)))
	for section in _attrib_texts:
		_attrib_texts[section].text = str(v.get(section, "-"))


func set_hand(cells: Vector2i) -> void:
	for section in slots:
		slots[section].hand_size = cells
		slots[section].queue_redraw()


func _on_slot_clicked(o: Dictionary, _section: String) -> void:
	var it = Game.items.get(int(o.id))
	if it != null:
		item_lift.emit(it)


func _on_slot_used(_o: Dictionary, section: String) -> void:
	var part: int = SLOT_PARTS[section]
	if part >= 0:
		Game.item_unequip(part)


func _on_slot_put(section: String) -> void:
	var part: int = SLOT_PARTS[section]
	if part >= 0:
		item_put_on.emit(part)
