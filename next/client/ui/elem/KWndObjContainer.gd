# KWndObjContainer - a grid of cells holding items, or one slot holding one item
# (Ui\Elem\WndObjContainer.cpp: KWndObjectMatrix and KWndObjectBox).
#
# The numbers are the old ones (KWndObjectMatrix::Init / PaintWindow / DropObject):
#   HUnits= VUnits=   cells across and down (a slot is 1 x 1); a cell is Width / HUnits pixels wide
#   UnitBorder=       the picture sits that many pixels inside its cells
#   HaveBgColor=      1 (default) shades the cells behind an item: [ObjContColor] of the theme's
#                     公共.ini - NormalColor, NotUseableColor (a piece this character cannot wear),
#                     MouseOverColor, PutdownColor (where the item on the cursor would land)
#   a gold / platina / purple item gets its border (BorderColorMap), pulsing every 60 ms
#   an item held on the cursor lands CENTRED on the cell under it: left = x + (w+1)/2 - w
#
# The container knows nothing about the protocol: it shows what `set_objects` gave it and reports
# clicks; the window decides what a click means (pick up, put down, wear, use).
extends "res://ui/elem/KWndWindow.gd"

const KFont := preload("res://ui/KFont.gd")
const KText := preload("res://ui/KText.gd")

signal object_clicked(obj: Dictionary)                 # left click on an item (nothing on the cursor)
signal object_right_clicked(obj: Dictionary)           # right click on an item
signal object_double_clicked(obj: Dictionary)
signal put_requested(x: int, y: int)                   # left click while an item is on the cursor: the top-left cell it would take
signal hovered(obj)                                    # the item under the mouse, or null

const NO_PUT_POS := -1
const BORDER_DELAY_MS := 60        # defBorderDelayTime
const BORDER_STEP := 0.05          # m_fBdAddPerFrame
# BorderColorMap of WndObjContainer.cpp: {border, colour, light} for ex_type 1 (gold), 2 (platina), 3 (purple)
const BORDER_COLORS := [
	[Color8(100, 80, 30), Color8(243, 194, 70), Color8(255, 255, 170)],
	[Color8(110, 110, 110), Color8(240, 240, 240), Color8(255, 255, 255)],
	[Color8(77, 30, 100), Color8(188, 80, 255), Color8(255, 170, 255)],
]

# [ObjContColor] as WndObjContainerInit read it; the defaults are the l_BgColors of the source
# (0x0a001e13...: the shadow alpha of the old renderer is five bits, 0x0a of 31)
static var bg_colors := {
	"normal": Color8(0, 30, 19, 82), "not_useable": Color8(44, 0, 0, 82), "special": Color8(101, 73, 21, 82),
	"mouse_over": Color8(0, 6, 54, 82), "putdown": Color8(0, 0, 255, 82), "price_marked": Color8(173, 133, 121, 82),
}
static var _bg_loaded := false

var h_units := 1
var v_units := 1
var unit_w := 1
var unit_h := 1
var unit_border := 0
var have_bg_color := true
var accept_free := false          # OBJCONT_S_ACCEPT_FREE: anything dropped is taken at (0,0) (a slot)
var objects: Array = []           # [{id, x, y, w, h, image (KUiImage or null), ex_type, usable, name, count}]
var hand_size := Vector2i.ZERO    # size in cells of the item on the cursor (zero = none)
var _mouse_over := -1
var _put_pos := Vector2i(NO_PUT_POS, NO_PUT_POS)
var _border_percent := 0.0
var _border_step := BORDER_STEP
var _border_ms := 0
var _last_click_ms := 0
var _last_click_obj := -1


static func load_colors() -> void:
	if _bg_loaded:
		return
	_bg_loaded = true
	var table = Assets.ui_data("cong-cong")
	if table == null:
		return
	for sec in table.get("sections", []):
		if str(sec.get("name", "")).to_lower() != "objcontcolor":
			continue
		var values: Dictionary = sec.get("values", {})
		var alpha := clampi(KUiScheme.leading_int(str(values.get("alpha", "120")), 120), 0, 255)
		# nAlpha = (nAlpha << 21) & 0xff000000 puts alpha >> 3 in the top byte: a five-bit alpha (0..31)
		var a := int(round(float(alpha >> 3) * 255.0 / 31.0))
		for key in [["normal", "normalcolor"], ["not_useable", "notuseablecolor"], ["special", "specialcolor"],
				["mouse_over", "mouseovercolor"], ["putdown", "putdowncolor"], ["price_marked", "pricemarkedcolor"]]:
			var v := str(values.get(key[1], ""))
			if v != "":
				var c := KText.color_of(v, bg_colors[key[0]])
				c.a8 = a
				bg_colors[key[0]] = c


func init_from(ini: KUiScheme, section: String) -> bool:
	if not super.init_from(ini, section):
		return false
	load_colors()
	h_units = maxi(1, ini.get_integer(section, "HUnits", 1))
	v_units = maxi(1, ini.get_integer(section, "VUnits", 1))
	unit_w = maxi(1, int(size.x) / h_units)
	unit_h = maxi(1, int(size.y) / v_units)
	have_bg_color = ini.get_bool(section, "HaveBgColor", true)
	accept_free = ini.get_bool(section, "AcceptFree", false)
	unit_border = clampi(ini.get_integer(section, "UnitBorder", 0), 0, mini(unit_w, unit_h) - 1)
	mouse_filter = Control.MOUSE_FILTER_STOP
	return true


func set_objects(list: Array) -> void:
	objects = list
	_mouse_over = -1
	queue_redraw()


# The item whose cells cover (x, y), or null.
func object_at(x: int, y: int):
	for o in objects:
		if x >= o.x and y >= o.y and x < o.x + o.w and y < o.y + o.h:
			return o
	return null


func _cell_of(local: Vector2) -> Vector2i:
	return Vector2i(int(local.x) / unit_w, int(local.y) / unit_h)


# KWndObjectMatrix::DropObject: where an item of hand_size lands when the cursor is on `cell`.
func put_pos_for(cell: Vector2i) -> Vector2i:
	var w := hand_size.x
	var h := hand_size.y
	if w > h_units or h > v_units:
		return Vector2i(NO_PUT_POS, NO_PUT_POS)
	var right := mini(cell.x + (w + 1) / 2, h_units)
	var bottom := mini(cell.y + (h + 1) / 2, v_units)
	var left := right - w if right >= w else 0
	var top := bottom - h if bottom >= h else 0
	return Vector2i(left, top)


func _gui_input(event: InputEvent) -> void:
	if event is InputEventMouseMotion:
		var cell := _cell_of(event.position)
		var o = object_at(cell.x, cell.y)
		var idx := objects.find(o) if o != null else -1
		var put := put_pos_for(cell) if hand_size != Vector2i.ZERO else Vector2i(NO_PUT_POS, NO_PUT_POS)
		if idx != _mouse_over or put != _put_pos:
			_mouse_over = idx
			_put_pos = put
			hovered.emit(o)
			queue_redraw()
	elif event is InputEventMouseButton and event.pressed:
		var cell := _cell_of(event.position)
		if event.button_index == MOUSE_BUTTON_LEFT:
			if hand_size != Vector2i.ZERO:
				var put := Vector2i.ZERO if accept_free else put_pos_for(cell)
				if put.x != NO_PUT_POS:
					put_requested.emit(put.x, put.y)
			else:
				var o = object_at(cell.x, cell.y)
				if o != null:
					var now := Time.get_ticks_msec()
					var idx := objects.find(o)
					if idx == _last_click_obj and now - _last_click_ms < 400:
						_last_click_obj = -1
						object_double_clicked.emit(o)
					else:
						_last_click_obj = idx
						_last_click_ms = now
						object_clicked.emit(o)
			accept_event()
		elif event.button_index == MOUSE_BUTTON_RIGHT:
			var o = object_at(cell.x, cell.y)
			if o != null:
				object_right_clicked.emit(o)
			accept_event()


func _notification(what: int) -> void:
	if what == NOTIFICATION_MOUSE_EXIT:
		if _mouse_over != -1 or _put_pos.x != NO_PUT_POS:
			_mouse_over = -1
			_put_pos = Vector2i(NO_PUT_POS, NO_PUT_POS)
			hovered.emit(null)
			queue_redraw()


func _process(_delta: float) -> void:
	# the gold border breathes: 0 -> 0.6 -> 0 in steps of 0.05 every 60 ms
	var now := Time.get_ticks_msec()
	if now - _border_ms >= BORDER_DELAY_MS:
		_border_ms = now
		_border_percent += _border_step
		if _border_percent > 0.6:
			_border_step = -BORDER_STEP
			_border_percent = 0.6 + _border_step
		elif _border_percent < 0.0:
			_border_step = BORDER_STEP
			_border_percent = _border_step
		var any_border := false
		for o in objects:
			if int(o.get("ex_type", 0)) > 0:
				any_border = true
				break
		if any_border:
			queue_redraw()


func _draw() -> void:
	for i in objects.size():
		var o: Dictionary = objects[i]
		var cells := Rect2(o.x * unit_w, o.y * unit_h, o.w * unit_w, o.h * unit_h)
		var inner := Rect2(cells.position + Vector2(unit_border, unit_border), cells.size - Vector2(unit_border * 2, unit_border * 2))
		var shade := Color(0, 0, 0, 0)
		if _put_pos.x != NO_PUT_POS and o == object_at(_put_pos.x, _put_pos.y):
			shade = bg_colors.putdown
		elif i == _mouse_over:
			shade = bg_colors.mouse_over
		elif have_bg_color:
			shade = bg_colors.normal if bool(o.get("usable", true)) else bg_colors.not_useable
		if shade.a > 0.0:
			draw_rect(inner, shade, true)
		var img = o.get("image", null)
		if img != null:
			# the sprite's box is centred in the item's cells (DrawGameObj gets the rectangle)
			var box := Vector2(img.box)
			img.draw(self, (inner.position + (inner.size - box) * 0.5).floor(), 0)
		else:
			var font = KFont.of(12)
			if font != null:
				font.draw(self, inner.position + Vector2(2, 2), str(o.get("name", "?")).left(maxi(1, int(inner.size.x) / 6)), Color.WHITE, Color.BLACK)
		var count := int(o.get("count", 1))
		if count > 1:
			var font = KFont.of(12)
			if font != null:
				var s := str(count)
				font.draw(self, Vector2(inner.end.x - font.width_of(s) - 1, inner.end.y - 13), s, Color(1, 1, 0.6), Color.BLACK)
		var ex := int(o.get("ex_type", 0))
		if ex > 0 and ex <= BORDER_COLORS.size():
			var pair: Array = BORDER_COLORS[ex - 1]
			var c: Color = (pair[0] as Color).lerp(pair[1], _border_percent)
			draw_rect(Rect2(cells.position, cells.size - Vector2.ONE), c, false, 1.0)
	if _put_pos.x != NO_PUT_POS:
		var r := Rect2(_put_pos.x * unit_w + unit_border, _put_pos.y * unit_h + unit_border,
			hand_size.x * unit_w - unit_border * 2, hand_size.y * unit_h - unit_border * 2)
		draw_rect(r, bg_colors.putdown, true)
