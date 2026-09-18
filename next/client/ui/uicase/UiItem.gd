# KUiItem - the bag (Ui\UiCase\UiItem.cpp; layout <theme>\随身物品.ini).
#
# [ItemBox] is a KWndObjectMatrix of HUnits x VUnits cells over the bag room of the character
# (room 0 of the zone).  What the window does with a click is what KUiItem::WndProc did with
# WND_N_ITEM_PICKDROP / WND_N_OBJ_DB_CLICK: a click lifts the item onto the cursor, the next click
# asks the zone to put it there (C2G_ITEM_MOVE - the zone answers with where it really went), a
# double click or a right click uses the item (a medicine is eaten, a piece of equipment is worn:
# KItemList::UseItem).  [OpenStatus] opens the character window, [CloseBtn] closes this one.
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KWndLabeledButton := preload("res://ui/elem/KWndLabeledButton.gd")
const KWndObjContainer := preload("res://ui/elem/KWndObjContainer.gd")
const KUiItemView := preload("res://ui/KUiItemView.gd")

const SCHEME := "tui-do"
const LABELED_BUTTONS := ["OpenCurrency", "GetMoneyBtn", "OpenStatus", "DecomposeEquip", "MakeAdvBtn", "MarkPriceBtn", "MakeStallBtn"]

signal open_status
signal closed
signal item_hovered(item)          # the item under the mouse (Dictionary) or null
signal item_lift(item: Dictionary) # the player clicked an item with nothing on the cursor
signal item_put(x: int, y: int)    # the player clicked a cell with an item on the cursor

var box: KWndObjContainer = null
var _title := KWndText.new()
var _money := KWndText.new()
var _gold_coin := KWndText.new()
var _close := KWndButton.new()
var _buttons := {}
var _texts: Array = []


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiItem"
	add_child(_title)
	_title.init_from(ini, "Title")
	add_child(_close)
	_close.init_from(ini, "CloseBtn")
	_close.clicked.connect(close_window)
	for section in ["MoneyTitle", "GoldCoinTitle", "BindingGoldTitle"]:
		var t := KWndText.new()
		add_child(t)
		t.init_from(ini, section)
		_texts.append(t)
	add_child(_money)
	_money.init_from(ini, "Money")
	_money.text = "0"
	add_child(_gold_coin)
	_gold_coin.init_from(ini, "GoldCoin")
	_gold_coin.text = "0"
	# the binding-gold line shares the row of the score line in the 2.0 layout; the score is unused
	var binding := KWndText.new()
	add_child(binding)
	binding.init_from(ini, "BindingGold")
	binding.text = "0"
	_texts.append(binding)
	for section in LABELED_BUTTONS:
		var b := KWndLabeledButton.new()
		add_child(b)
		b.init_from(ini, section)
		_buttons[section] = b
	_buttons["OpenStatus"].clicked.connect(func(): open_status.emit())
	box = KWndObjContainer.new()
	add_child(box)
	box.init_from(ini, "ItemBox")
	box.object_clicked.connect(_on_object_clicked)
	box.object_double_clicked.connect(_on_object_used)
	box.object_right_clicked.connect(_on_object_used)
	box.put_requested.connect(func(x: int, y: int): item_put.emit(x, y))
	box.hovered.connect(func(o): item_hovered.emit(Game.items.get(int(o.id)) if o != null else null))
	Game.items_changed.connect(refresh)
	Game.item_changed.connect(func(_item): refresh())
	Game.item_removed.connect(func(_id): refresh())
	Game.money_changed.connect(_on_money)
	refresh()
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


# Rebuilds the box from Game.items: everything in the bag room, at its own cells.
func refresh() -> void:
	if box == null:
		return
	var list: Array = []
	for id in Game.items:
		var it: Dictionary = Game.items[id]
		if int(it.room) == Game.ROOM_BAG:
			list.append(KUiItemView.object_of(it))
	box.set_objects(list)
	_on_money(Game.money, Game.bank_money)


func _on_money(money: int, _bank: int) -> void:
	_money.text = str(money)


# The size of the item on the cursor, so the box can show where it would land (zero = none).
func set_hand(cells: Vector2i) -> void:
	if box != null:
		box.hand_size = cells
		box.queue_redraw()


func _on_object_clicked(o: Dictionary) -> void:
	var it = Game.items.get(int(o.id))
	if it != null:
		item_lift.emit(it)


# KItemList::UseItem: equipment is worn, a medicine / portal / script item is used
func _on_object_used(o: Dictionary) -> void:
	var it = Game.items.get(int(o.id))
	if it == null:
		return
	match int(it.genre):
		KUiItemView.GENRE_EQUIP:
			Game.item_equip(int(it.id), -1)
		KUiItemView.GENRE_MEDICINE, KUiItemView.GENRE_TOWN_PORTAL, KUiItemView.GENRE_SCRIPT:
			Game.item_use(int(it.id))
		_:
			pass
