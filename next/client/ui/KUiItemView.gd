# KUiItemView - what a window needs to show one item of Game.items: the container object (cells,
# picture, shade), whether this character can wear it, and its description lines
# (KUiBase::GetObjImage / GetItemDesc through CoreShell of the old client).
extends RefCounted

const KMagicDesc := preload("res://ui/KMagicDesc.gd")

# ITEMGENRE of KItem.h
const GENRE_EQUIP := 0
const GENRE_MEDICINE := 1
const GENRE_TASK := 4
const GENRE_TOWN_PORTAL := 5
const GENRE_SCRIPT := 6
# the attribute ids this window reads by number (MAGIC_ATTRIB)
const MAGIC_DURABILITY := 31
const REQUIRE_STR := 32
const REQUIRE_DEX := 33
const REQUIRE_VIT := 34
const REQUIRE_ENG := 35
const REQUIRE_LEVEL := 36
const REQUIRE_SERIES := 37
const REQUIRE_SEX := 38
const REQUIRE_MENPAI := 39
# The colour of a name, KItem::GetDesc of the old core with the named colours of Engine/Text.cpp:
# equipment by ITEMEXTENDTYPE - gold Yellow (255,255,0), platina Yellow too, purple Purple
# (188,40,255) - else Blue (100,100,255) when it carries a magic prefix / suffix, White without;
# a quest item Yellow; everything else White.
const NAME_COLORS := [Color.WHITE, Color8(255, 255, 0), Color8(255, 255, 0), Color8(188, 40, 255)]
const NAME_COLOR_MAGIC := Color8(100, 100, 255)


# The object a KWndObjContainer draws for an item (at the item's own cells).
static func object_of(item: Dictionary) -> Dictionary:
	return {
		"id": int(item.id), "x": int(item.x), "y": int(item.y), "w": maxi(1, int(item.w)), "h": maxi(1, int(item.h)),
		"image": Assets.item_image(str(item.get("image", ""))), "ex_type": int(item.get("ex_type", 0)),
		"usable": usable(item), "name": str(item.get("name", "")), "count": int(item.get("count", 1)),
	}


# KItemList::EnoughAttrib as far as the client knows its own character: level, series and sex
# (strength and the rest come with the attribute system, M12 - they read as enough).
static func usable(item: Dictionary) -> bool:
	if int(item.get("genre", 0)) != GENRE_EQUIP:
		return true
	var me = Game.entities.get(Game.entity_id)
	if me == null:
		return true
	for r in item.get("require", []):
		var v: Array = r.get("value", [])
		var need := int(v[0]) if v.size() > 0 else 0
		match int(r.get("type", 0)):
			REQUIRE_LEVEL:
				if int(me.get("level", 1)) < need:
					return false
			REQUIRE_SERIES:
				if int(me.get("series", 0)) != need:
					return false
			REQUIRE_SEX:
				if int(me.get("sex", 0)) != need:
					return false
	return true


static func name_color(item: Dictionary) -> Color:
	var genre := int(item.get("genre", 0))
	if genre == GENRE_TASK:
		return Color8(255, 255, 0)
	if genre != GENRE_EQUIP:
		return Color.WHITE
	var ex := int(item.get("ex_type", 0))
	if ex > 0:
		return NAME_COLORS[ex] if ex < NAME_COLORS.size() else Color.WHITE
	var magic: Array = item.get("magic", [])
	return NAME_COLOR_MAGIC if magic.size() > 0 and int(magic[0].get("type", 0)) != 0 else Color.WHITE


# The first line of the tooltip: the name, " [Cấp N]" for equipment with a level (KItem::GetDesc),
# " xN" for a stack.
static func title_of(item: Dictionary) -> String:
	var name := str(item.get("name", ""))
	var level := int(item.get("level", 0))
	if int(item.get("genre", 0)) == GENRE_EQUIP and level > 0:
		name += " [Cấp %d]" % level
	var count := int(item.get("count", 1))
	return name + (" x%d" % count if count > 1 else "")


# The lines of the item's tooltip: [{text, color}].  Name first, then what the tables say about
# it through KMagicDesc, the requirements, the description, the price.
static func describe(item: Dictionary) -> Array:
	var lines: Array = []
	lines.append({"text": title_of(item), "color": name_color(item)})
	var ok := usable(item)
	var has_durability: bool = int(item.get("durability", -1)) >= 0 and int(item.get("max_durability", -1)) > 0
	for a in item.get("base", []):
		if has_durability and int(a.get("type", 0)) == MAGIC_DURABILITY:
			continue   # the durability line below says what is left of it
		var t := KMagicDesc.describe(a)
		if t != "":
			lines.append({"text": t, "color": Color8(255, 255, 255)})
	if has_durability:
		lines.append({"text": "Độ bền: %d/%d" % [int(item.durability), int(item.max_durability)], "color": Color8(255, 255, 255)})
	for a in item.get("require", []):
		var t := KMagicDesc.describe(a)
		if t != "":
			lines.append({"text": t, "color": Color8(200, 255, 200) if ok else Color8(255, 80, 80)})
	for a in item.get("magic", []):
		var t := KMagicDesc.describe(a)
		if t != "":
			lines.append({"text": t, "color": Color8(125, 255, 216)})
	var intro := str(item.get("intro", "")).strip_edges()
	if intro != "":
		lines.append({"text": intro, "color": Color8(210, 210, 210)})
	var price := int(item.get("price", 0))
	if price > 0:
		lines.append({"text": "Giá: %d" % price, "color": Color8(255, 217, 78)})
	return lines
