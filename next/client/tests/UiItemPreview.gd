# A picture of the item windows for the project owner: the bag, the character window on its
# equipment page and a tooltip, filled with rows of the real item tables (client/assets/items/
# v004.json, the icons the export wrote) - SAMPLE DATA, not a character of the zone.
#
#   godot --path client tests/UiItemPreview.tscn      -> user://logs/ui_vat-pham.png
#
# Headless it only builds the windows and quits (the same layouts the UiCheck test covers).
extends Control

const KWndShowAnimate := preload("res://ui/elem/KWndShowAnimate.gd")
const UiItem := preload("res://ui/uicase/UiItem.gd")
const UiStatus := preload("res://ui/uicase/UiStatus.gd")
const UiMouseHover := preload("res://ui/uicase/UiMouseHover.gd")
const KUiItemView := preload("res://ui/KUiItemView.gd")

const SCREEN := Vector2i(1024, 768)


func _ready() -> void:
	get_window().content_scale_size = SCREEN
	var black := ColorRect.new()
	black.color = Color8(20, 24, 20)
	black.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(black)
	KWndShowAnimate.animate = false
	if not _sample_character():
		print("UI_ITEM_PREVIEW no item tables (python tools/dev.py assets)")
		get_tree().quit(1)
		return
	var bag := UiItem.new()
	add_child(bag)
	var st := UiStatus.new()
	add_child(st)
	var tip := UiMouseHover.new()
	add_child(tip)
	if not bag.load_scheme(SCREEN) or not st.load_scheme(SCREEN) or not tip.load_scheme(SCREEN):
		print("UI_ITEM_PREVIEW layouts missing (python tools/dev.py assets)")
		get_tree().quit(1)
		return
	bag.open_window()
	st.open_window()
	st._on_page_button(true, UiStatus.PAGE_EQUIP)
	# the tooltip of the gold armour, as if the mouse rested on it
	var armour: Dictionary = Game.items[2]
	tip.show_lines(KUiItemView.describe(armour), Vector2(430, 300))
	if DisplayServer.get_name() == "headless":
		print("UI_ITEM_PREVIEW built")
		get_tree().quit(0)
		return
	for i in 8:
		await RenderingServer.frame_post_draw
	DirAccess.make_dir_recursive_absolute("user://logs")
	var path := "user://logs/ui_vat-pham.png"
	var err := get_viewport().get_texture().get_image().save_png(path)
	print("shot vat-pham -> %s%s" % [ProjectSettings.globalize_path(path), "" if err == OK else " FAILED"])
	get_tree().quit(0 if err == OK else 1)


# Rows of the newest exported table set, made into the dictionaries G2C_ITEM_LIST would give.
func _sample_character() -> bool:
	var tables = Assets.load_json("%s/items/v004.json" % Assets.assets_root())
	if tables == null:
		tables = Assets.load_json("%s/items/base.json" % Assets.assets_root())
	if tables == null:
		return false
	var eq: Dictionary = tables.get("equipment", {})
	var sword: Dictionary = eq.get("meleeweapon", [{}])[0]
	var sword2: Dictionary = eq.get("meleeweapon", [{}, {}])[1]
	var cloth: Dictionary = eq.get("armor", [{}])[0]
	var helm: Dictionary = eq.get("helm", [{}])[0]
	var ring: Dictionary = eq.get("ring", [{}])[0]
	var gold: Dictionary = tables.get("gold", [{}])[1]
	var potion: Dictionary = tables.get("medicine", [{}])[0]
	var items := {}
	var counter := {"next": 1}   # a lambda captures a plain int by value; a dictionary is shared
	var add := func(row: Dictionary, room: int, x: int, y: int, ex_type: int = 0, count: int = 1, genre: int = 0) -> void:
		var item := {
			"id": counter.next, "genre": genre, "detail": int(row.get("detail", 0)), "particular": int(row.get("particular", 0)),
			"level": int(row.get("level", 1)), "series": int(row.get("series", -1)), "count": count,
			"durability": 30 if genre == 0 else -1, "max_durability": 30 if genre == 0 else -1, "ex_type": ex_type,
			"room": room, "x": x, "y": y, "w": int(row.get("w", 1)), "h": int(row.get("h", 1)),
			"name": str(row.get("name", "?")), "image": str(row.get("image", "")), "intro": str(row.get("intro", "")),
			"price": int(row.get("price", 0)), "base": [], "require": [], "magic": [],
		}
		# a JSON null (a row without the column) reads as Nil, not as an empty list
		var basics = row.get("basics", [])
		for b in (basics if basics is Array else []):
			var r: Dictionary = b.get("range", {})
			item.base.append({"type": int(b.get("type", 0)), "value": [int(r.get("max", 0)), 0, 0]})
		var reqs = row.get("reqs", [])
		for q in (reqs if reqs is Array else []):
			item.require.append({"type": int(q.get("type", 0)), "value": [int(q.get("para", 0)), 0, 0]})
		var attribs = row.get("attribs", [])
		for a in (attribs if attribs is Array else []):
			item.base.append({"type": int(a.get("attrib", 0)), "value": [int(a.get("value", 0)), int(a.get("time", 0)), 0]})
		items[counter.next] = item
		counter.next += 1
	add.call(sword, Game.ROOM_BAG, 0, 0)
	add.call(gold, Game.ROOM_BAG, 2, 0, 1)
	add.call(helm, Game.ROOM_BAG, 4, 0)
	add.call(potion, Game.ROOM_BAG, 0, 4, 0, 1, 1)
	add.call(potion, Game.ROOM_BAG, 1, 4, 0, 1, 1)
	add.call(ring, Game.ROOM_BAG, 5, 3)
	add.call(sword2, Game.ROOM_BODY, 3, 0)
	add.call(cloth, Game.ROOM_BODY, 1, 0)
	add.call(potion, Game.ROOM_IMMEDIACY, 0, 0, 0, 1, 1)
	Game.items = items
	Game.money = 12345
	Game.bank_money = 0
	Game.entity_id = 1
	Game.entities[1] = {"id": 1, "name": "Kiếm Khách", "level": 12, "series": 0, "sex": 0, "life": 250, "life_max": 300, "speed": 200}
	return true
