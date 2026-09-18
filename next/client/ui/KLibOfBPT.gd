# KLibOfBPT - the client's OWN item tables (\settings\item\NNN\*.txt of the 2.0 archives, written
# by `jxassets export-items` as items/client_vNNN.json): the name, the picture and the
# description the player reads come from here, the way the real client's KItem copied them out of
# its table row (gamecl.exe 0x0062fc20), and not from the server's tables - the two differ in
# wording ("Chủy thủ bằng sắt, sát thương kém." here, "Loại kiếm nhỏ bằng sắt..." on the server).
#
# A row is found as KItemGenerator found it (KBasPropTbl.h): equipment by table (detail type) and
# row particular * 10 + level - 1 (a mask by particular), a gold piece by its goldequip row, medicine
# by detail * 5 + level - 1, a quest item by detail, the town portal as the first row, a script
# item by its magicscript row.  An item whose row the client's tables do not have keeps the
# server's text.
extends RefCounted

const EQUIP_TABLES := ["meleeweapon", "rangeweapon", "armor", "ring", "amulet", "boot", "belt", "helm", "cuff", "pendant", "horse", "mask"]
const DETAIL_MASK := 11

static var _sets: Dictionary = {}        # version -> Dictionary (the JSON) or null


# The client's table set of a version (client_vNNN.json), null when it is not exported
static func table_set(version: int):
	if _sets.has(version):
		return _sets[version]
	var raw = Assets.load_json("%s/items/client_v%03d.json" % [Assets.assets_root(), version])
	_sets[version] = raw if raw is Dictionary else null
	return _sets[version]


static func forget() -> void:
	_sets = {}


# The client's row for an item of Game.items, or {} when there is none.
static func row_of(item: Dictionary) -> Dictionary:
	var set = table_set(int(item.get("version", 0)))
	if set == null:
		return {}
	var genre := int(item.get("genre", 0))
	var detail := int(item.get("detail", 0))
	var particular := int(item.get("particular", 0))
	var level := int(item.get("level", 0))
	var rows = null
	var index := -1
	match genre:
		0:
			if int(item.get("ex_type", 0)) == 1:
				rows = set.get("gold", [])
				index = int(item.get("gen_param", 0)) - 1
			elif detail >= 0 and detail < EQUIP_TABLES.size():
				rows = set.get("equipment", {}).get(EQUIP_TABLES[detail], [])
				index = particular if detail == DETAIL_MASK else particular * 10 + level - 1
		1:
			rows = set.get("medicine", [])
			index = detail * 5 + level - 1
		4:
			rows = set.get("quest", [])
			index = detail
		5:
			rows = set.get("town_portal", [])
			index = 0
		6:
			rows = set.get("scripts", [])
			index = int(item.get("gen_param", 0)) - 1
	if rows == null or index < 0 or index >= rows.size():
		return {}
	var r = rows[index]
	return r if r is Dictionary else {}


# The item with the client's name / picture / description in place of the server's
static func display(item: Dictionary) -> Dictionary:
	var r := row_of(item)
	if r.is_empty():
		return item
	var d := item.duplicate()
	d["name"] = str(r.get("name", item.get("name", "")))
	d["intro"] = str(r.get("intro", item.get("intro", "")))
	if str(r.get("image", "")) != "":
		d["image"] = str(r.get("image", ""))
	return d
