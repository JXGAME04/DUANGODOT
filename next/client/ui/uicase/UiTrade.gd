# UiTrade - the trade window of the VLTK 2.0 client (KUiTrade; OpenWindow 0x004C0D69 loads "%s\玩家间交易.ini" at 0x004C02D2,
# Init 0x004BF000 (the widgets), WndProc 0x004C0AB0; docs/CLIENT-2.0.md §22).
#   - [Main] 461 x 404 at (186,100) on 玩家交易底板_JX20.spr; [TakewithItemsBox] the bag (6 x 10) with [TakewithMoney] under it;
#     [SelfItemsBox] my table (8 x 4, the trade room 2 of the zone) with the [SelfMoney] edit and [AddMoney] / [ReduceMoney]
#     (one coin per click, held = repeated, 0x004BFE20); [OtherItemsBox] the partner's table (8 x 4) with [OtherName] and
#     [OtherMoney]; [InfoText] the state line (G_STR_OTHER_NOT_OK "Chờ đối phương khóa" / G_STR_OTHER_OK "Đối phương đã khóa"
#     / G_STR_WAIT_TRADING "Chờ đợi", the colours of the layout); the *BindGold rows are the JX2 room 4 (bound gold) - shown,
#     not wired.
#   - buttons: [OkBtn] "Khoá" a check box -> TradeOperation(0x15, checked) 0x004C0400 = the lock (decision 2); [TradeBtn]
#     "Xác nhận" -> TradeOperation(0x16) 0x004C0020 = the ok (decision 1); [CancelBtn] "Huỷ giao dịch" 0x004C0080 = decision 0.
#   - the item boxes: a click on a bag item puts it on my table (KItemList::ExchangeItem to the trade room), a click on my
#     table takes it back; nothing moves once I locked (MSG_MOVE_ITEM_FAIL_TRADE_LOCKED).
# The zone speaks the trade packets directly (Game.trade_request, Game.item_move); the window shows Game.trade.
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KWndLabeledButton := preload("res://ui/elem/KWndLabeledButton.gd")
const KWndEdit := preload("res://ui/elem/KWndEdit.gd")
const KWndObjContainer := preload("res://ui/elem/KWndObjContainer.gd")
const KUiItemView := preload("res://ui/KUiItemView.gd")
const Proto := preload("res://proto/jx_pb.gd")

const SCHEME := "giao-dich"
const TEXTS := ["TakewithMoney", "TakewithBindGold", "OtherName", "OtherMoney", "OtherBindGold", "InfoText", "MoneyTip0", "MoneyTip1",
	"MoneyTip2", "MoneyTip3", "MoneyTip4", "MoneyTip5", "TitleTip"]
const ROOM_BAG := 0
const ROOM_TRADE := 2
const TRADE_W := 8
const TRADE_H := 4

signal closed
signal item_hovered(item)

var _take := KWndObjContainer.new()
var _self_box := KWndObjContainer.new()
var _other_box := KWndObjContainer.new()
var _self_money := KWndEdit.new()
var _self_bind := KWndEdit.new()
var _ok := KWndLabeledButton.new()
var _trade := KWndLabeledButton.new()
var _cancel := KWndLabeledButton.new()
var _add := KWndButton.new()
var _reduce := KWndButton.new()
var _add_bind := KWndButton.new()
var _reduce_bind := KWndButton.new()
var _texts := {}
var _info_colors := {}
var _money := 0          # the money on my table as the window knows it (the edit / the buttons)


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiTrade"
	for section in TEXTS:
		var t := KWndText.new()
		add_child(t)
		t.init_from(ini, section)
		t.text = ini.get_string(section, "Text", "")
		_texts[section] = t
	_info_colors = {
		"wait": [ini.get_color("InfoText", "WaitTradeMsgColor", Color.WHITE), ini.get_color("InfoText", "WaitTradeMsgBorderColor", Color.BLACK)],
		"lock": [ini.get_color("InfoText", "LockMsgColor", Color.WHITE), ini.get_color("InfoText", "LockMsgBorderColor", Color.BLACK)],
		"unlock": [ini.get_color("InfoText", "UnlockMsgColor", Color.GRAY), ini.get_color("InfoText", "UnlockMsgBorderColor", Color.BLACK)],
	}
	for pair in [[_take, "TakewithItemsBox"], [_self_box, "SelfItemsBox"], [_other_box, "OtherItemsBox"]]:
		add_child(pair[0])
		pair[0].init_from(ini, pair[1])
		pair[0].hovered.connect(_on_box_hovered.bind(pair[0]))
	_take.object_clicked.connect(_on_bag_clicked)
	_self_box.object_clicked.connect(_on_table_clicked)
	add_child(_self_money)
	_self_money.init_from(ini, "SelfMoney")
	_self_money.submitted.connect(func(text: String): _set_money(int(text)))
	add_child(_self_bind)
	_self_bind.init_from(ini, "SelfBindGold")
	_self_bind.set_text("0")
	for pair in [[_ok, "OkBtn"], [_trade, "TradeBtn"], [_cancel, "CancelBtn"]]:
		add_child(pair[0])
		pair[0].init_from(ini, pair[1])
	_ok.toggled.connect(_on_lock)
	_ok.clicked.connect(func(): _on_lock(true))
	_trade.clicked.connect(_on_confirm)
	_trade.toggled.connect(func(_c: bool): _on_confirm())
	_cancel.clicked.connect(_on_cancel)
	for pair in [[_add, "AddMoney"], [_reduce, "ReduceMoney"], [_add_bind, "AddBindGold"], [_reduce_bind, "ReduceBindGold"]]:
		add_child(pair[0])
		pair[0].init_from(ini, pair[1])
	_add.clicked.connect(func(): _set_money(_money + 1))
	_reduce.clicked.connect(func(): _set_money(_money - 1))
	Game.trade_changed.connect(_on_trade_changed)
	Game.items_changed.connect(refresh)
	Game.item_changed.connect(func(_item): refresh())
	Game.item_removed.connect(func(_id): refresh())
	Game.money_changed.connect(func(_m: int, _b: int): refresh())
	return true


func open_window() -> void:
	_money = int(Game.trade.self_money)
	_ok.check(false)
	_ok.enable(true)
	_trade.check(false)
	refresh()
	show_window()


func close_window() -> void:
	hide_window()
	closed.emit()


# a trade that ended (the 0x78 packet) or a state back to normal closes the window (KUiTrade::OpenWindow / 0x004C0E90)
func _on_trade_changed() -> void:
	if int(Game.trade.state) != 2:
		if visible:
			close_window()
		return
	if not visible:
		open_window()
	else:
		refresh()


func refresh() -> void:
	var t: Dictionary = Game.trade
	_texts["OtherName"].text = str(t.get("partner_name", ""))
	_texts["OtherMoney"].text = str(int(t.get("dest_money", 0)))
	_texts["TakewithMoney"].text = str(int(Game.money) - _money)
	_texts["TakewithBindGold"].text = "0"
	_texts["OtherBindGold"].text = "0"
	if not _self_money.input.has_focus():
		_self_money.set_text(str(_money))
	var bag: Array = []
	var mine: Array = []
	for id in Game.items:
		var it: Dictionary = Game.items[id]
		if int(it.room) == ROOM_BAG:
			bag.append(KUiItemView.object_of(it))
		elif int(it.room) == ROOM_TRADE:
			mine.append(KUiItemView.object_of(it))
	_take.set_objects(bag)
	_self_box.set_objects(mine)
	var theirs: Array = []
	for id in t.get("other_items", {}):
		theirs.append(KUiItemView.object_of(t.other_items[id]))
	_other_box.set_objects(theirs)
	# the state line and the buttons (0x004BFA60: the partner's lock, then the wait for the ok)
	var self_lock := bool(t.get("self_lock", false))
	var dest_lock := bool(t.get("dest_lock", false))
	var info: KWndText = _texts["InfoText"]
	if dest_lock and self_lock:
		info.text = KUiItemView.client_string("G_STR_WAIT_TRADING")
		info.set_text_color(_info_colors.wait[0])
	elif dest_lock:
		info.text = KUiItemView.client_string("G_STR_OTHER_OK")
		info.set_text_color(_info_colors.lock[0])
	else:
		info.text = KUiItemView.client_string("G_STR_OTHER_NOT_OK")
		info.set_text_color(_info_colors.unlock[0])
	if self_lock:
		_ok.check(true)
		_ok.enable(false)   # the lock never opens again (the 0x6d decision 2 only sets it)
	_trade.enable(self_lock and dest_lock)
	_trade.check(bool(t.get("self_ok", false)))
	_add.enable(not self_lock)
	_reduce.enable(not self_lock)


# what typing a sum into the SelfMoney edit does (the --auto run uses it instead of the keyboard)
func put_money(value: int) -> void:
	_set_money(value)


# the money on my table: 0..the bag's; the zone clamps it too (the 0x6c packet)
func _set_money(value: int) -> void:
	if bool(Game.trade.self_lock):
		return
	value = clampi(value, 0, int(Game.money))
	if value == _money:
		refresh()
		return
	_money = value
	Game.trade_request(Proto.TradeCmd.TRADE_MONEY, 0, _money)
	refresh()


# a bag item onto my table: the first free spot of the 8 x 4 table (the zone's exchange checks it again)
func _on_bag_clicked(o: Dictionary) -> void:
	if bool(Game.trade.self_lock):
		return
	var spot := _free_spot(_self_box.objects, TRADE_W, TRADE_H, int(o.w), int(o.h))
	if spot.x < 0:
		return
	Game.item_move(int(o.id), ROOM_TRADE, spot.x, spot.y)


# an item of my table back into the bag: the first free spot there
func _on_table_clicked(o: Dictionary) -> void:
	if bool(Game.trade.self_lock):
		return
	var spot := _free_spot(_take.objects, _take.h_units, _take.v_units, int(o.w), int(o.h))
	if spot.x < 0:
		return
	Game.item_move(int(o.id), ROOM_BAG, spot.x, spot.y)


# KInventory::FindRoom: column by column, top to bottom, the first place a w x h item fits
static func _free_spot(objects: Array, width: int, height: int, w: int, h: int) -> Vector2i:
	for x in range(0, width - w + 1):
		for y in range(0, height - h + 1):
			var free := true
			for o in objects:
				if x < int(o.x) + int(o.w) and x + w > int(o.x) and y < int(o.y) + int(o.h) and y + h > int(o.y):
					free = false
					break
			if free:
				return Vector2i(x, y)
	return Vector2i(-1, -1)


func _on_lock(checked: bool) -> void:
	if checked and not bool(Game.trade.self_lock):
		Game.trade_request(Proto.TradeCmd.TRADE_DECISION, 0, 2)
	else:
		_ok.check(bool(Game.trade.self_lock))


func _on_confirm() -> void:
	if bool(Game.trade.self_lock) and bool(Game.trade.dest_lock):
		Game.trade_request(Proto.TradeCmd.TRADE_DECISION, 0, 1)
	else:
		_trade.check(false)


func _on_cancel() -> void:
	Game.trade_request(Proto.TradeCmd.TRADE_DECISION, 0, 0)


func _on_box_hovered(o, box: KWndObjContainer) -> void:
	if o == null:
		item_hovered.emit(null)
		return
	if box == _other_box:
		item_hovered.emit(Game.trade.other_items.get(int(o.id)))
	else:
		item_hovered.emit(Game.items.get(int(o.id)))


func my_table_count() -> int:
	return _self_box.objects.size()


func other_table_count() -> int:
	return _other_box.objects.size()
