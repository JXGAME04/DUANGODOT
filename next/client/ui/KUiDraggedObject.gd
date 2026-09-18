# KUiDraggedObject - the item on the cursor (Wnd_SetDragObj / Wnd_GetDragObj of Ui\Elem\Wnds.cpp).
#
# The old client had no drag-and-drop in the modern sense: a click on an item lifted it onto the
# cursor, the next click put it down (the server did the same with its "hand", KItemList::m_Hand).
# This node draws the lifted item at the mouse and remembers which item it is; the windows ask it
# what is held and tell the zone where it should go.
extends Control

signal changed

var item_id := 0
var image = null                 # KUiImage
var cells := Vector2i.ZERO       # w x h in cells; ZERO = nothing held
var pending_seq := 0             # the request that would put it down; 0 = none on the way


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	set_anchors_preset(Control.PRESET_FULL_RECT)
	z_index = 100


func holding() -> bool:
	return item_id != 0


func lift(item: Dictionary) -> void:
	item_id = int(item.id)
	image = Assets.item_image(str(item.get("image", "")))
	cells = Vector2i(maxi(1, int(item.w)), maxi(1, int(item.h)))
	pending_seq = 0
	changed.emit()
	queue_redraw()


func drop() -> void:
	item_id = 0
	image = null
	cells = Vector2i.ZERO
	pending_seq = 0
	changed.emit()
	queue_redraw()


func _process(_delta: float) -> void:
	if item_id != 0:
		queue_redraw()


func _draw() -> void:
	if item_id == 0:
		return
	var at := get_local_mouse_position()
	if image != null:
		image.draw(self, (at - Vector2(image.box) * 0.5).floor(), 0, Color(1, 1, 1, 0.9))
	else:
		draw_rect(Rect2(at - Vector2(12, 12), Vector2(24, 24)), Color(1, 1, 1, 0.5), false, 1.0)
