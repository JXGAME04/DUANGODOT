# KUiShortcutItem - the nine quick slots of the 2.0 bottom bar (Item_0..8 of 玩家信息主界面.ini; docs/CLIENT-2.0.md
# §7.2).  The 2.0 client keeps them in a 9 x 1 grid inside its item list (KItemList+0x4ce8) whose cells reference an
# item of the bag or a skill (the id with bit 0x4000000); GetGameData 0x3ec (0x00661B03) reads it as 9 x {genre, id}.
#   - ShortcutUseItem(k) 0x0042F990 -> 0x00472F80: OperationRequest(0xa, {genre, id, k, 0, x, y}, 3) (0x5c2d91):
#     genre 5 (an item) -> UseItem(id) 0x005FD4B0, which wants the item in the bag (pos_equiproom 3);
#     genre 4 (a skill) -> cast at the cursor (0x005BC500);
#   - a drop from the cursor (WndProc 0x00475A10 msg 0x511 -> 0x00472E70 -> op 3 kind 7 -> Exchange 0x005FAD10):
#     refused with a message when the bar already holds an item of that genre / detail / particular (0x006386C0),
#     else the cell takes the item (0x0060EB50: an item that exists, a skill the character holds);
#   - saved per character as [Player] Item_%d {5, id, genre, detail, particular} (0x00473B10 with GetGameData 0x7dd),
#     restored by op 0xe (0x5c30e1): an item of the saved kind takes the cell again (0x0060CA60 -> 0x00638500), none
#     -> the cell is cleared.  Here: user://shortcuts_<player_id>.json, "items".
extends RefCounted

const SLOTS := 9
const GENRE_SKILL := 4   # the object genre of a skill in the bar (0x40004 of the skill lists)
const GENRE_ITEM := 5    # the object genre of an item

var slots: Array = []    # [{genre, id, kind, detail, particular}]: genre 0 = empty; kind = the item's own genre


func _init() -> void:
	clear()


func clear() -> void:
	slots.clear()
	for _i in SLOTS:
		slots.append(_empty())


static func _empty() -> Dictionary:
	return {"genre": 0, "id": 0, "kind": 0, "detail": 0, "particular": 0}


func slot(k: int) -> Dictionary:
	if k < 0 or k >= SLOTS:
		return _empty()
	return slots[k]


# the cell of an item / skill, -1 for none
func slot_of(genre: int, id: int) -> int:
	for i in SLOTS:
		if int(slots[i].genre) == genre and int(slots[i].id) == id:
			return i
	return -1


# 0x006386C0: is an item of that kind (genre, detail, particular) already in the bar (another cell)?
func has_kind(kind: int, detail: int, particular: int, except_slot: int = -1) -> bool:
	for i in SLOTS:
		var s: Dictionary = slots[i]
		if i != except_slot and int(s.genre) == GENRE_ITEM and int(s.kind) == kind and int(s.detail) == detail and int(s.particular) == particular:
			return true
	return false


# a drop of a bag item (Game.items entry) on cell k: false when the bar already holds that kind (0x005FAE0E..0x005FAE5C)
func put_item(k: int, item: Dictionary) -> bool:
	if k < 0 or k >= SLOTS or item.is_empty() or int(item.get("id", 0)) <= 0:
		return false
	var kind := int(item.get("genre", 0))
	var detail := int(item.get("detail", 0))
	var particular := int(item.get("particular", 0))
	if has_kind(kind, detail, particular, k):
		return false
	slots[k] = {"genre": GENRE_ITEM, "id": int(item.id), "kind": kind, "detail": detail, "particular": particular}
	return true


# a skill dropped on cell k (0x0060EB50 genre 4: the character must hold it - the caller checks); the same skill
# elsewhere in the bar is cleared so one key means one thing
func put_skill(k: int, skill_id: int) -> bool:
	if k < 0 or k >= SLOTS or skill_id <= 0:
		return false
	for i in SLOTS:
		if i != k and int(slots[i].genre) == GENRE_SKILL and int(slots[i].id) == skill_id:
			slots[i] = _empty()
	slots[k] = {"genre": GENRE_SKILL, "id": skill_id, "kind": 0, "detail": 0, "particular": 0}
	return true


func remove(k: int) -> void:
	if k >= 0 and k < SLOTS:
		slots[k] = _empty()


# op 0xe (0x5c30e1) for every item cell: the referenced item still in the bag keeps the cell; else an item of the
# same kind in the bag takes it (0x00638500 compares genre, detail, particular); none -> the cell is cleared.
# Returns true when a cell changed.
func resolve(items: Dictionary, bag_room: int) -> bool:
	var changed := false
	for i in SLOTS:
		var s: Dictionary = slots[i]
		if int(s.genre) != GENRE_ITEM:
			continue
		var it = items.get(int(s.id))
		if it != null and int(it.get("room", -1)) == bag_room:
			continue
		var found := 0
		for id in items:
			var c: Dictionary = items[id]
			if int(c.get("room", -1)) == bag_room and int(c.get("genre", -1)) == int(s.kind) and int(c.get("detail", -1)) == int(s.detail) and int(c.get("particular", -1)) == int(s.particular):
				found = int(id)
				break
		if found != 0:
			slots[i] = {"genre": GENRE_ITEM, "id": found, "kind": int(s.kind), "detail": int(s.detail), "particular": int(s.particular)}
		else:
			slots[i] = _empty()
		changed = true
	return changed


# autoexec.lua: AddCommand("1", "", "ShortcutUseItem(0)") .. "9" -> 8 (the number row and the keypad)
static func slot_of_key(keycode: int) -> int:
	if keycode >= KEY_1 and keycode <= KEY_9:
		return keycode - KEY_1
	if keycode >= KEY_KP_1 and keycode <= KEY_KP_9:
		return keycode - KEY_KP_1
	return -1


func to_json() -> Array:
	var out := []
	for s in slots:
		out.append({"genre": int(s.genre), "id": int(s.id), "kind": int(s.kind), "detail": int(s.detail), "particular": int(s.particular)})
	return out


func from_json(list) -> void:
	clear()
	if not (list is Array):
		return
	for i in mini(SLOTS, list.size()):
		var s = list[i]
		if s is Dictionary:
			slots[i] = {"genre": int(s.get("genre", 0)), "id": int(s.get("id", 0)), "kind": int(s.get("kind", 0)),
				"detail": int(s.get("detail", 0)), "particular": int(s.get("particular", 0))}
