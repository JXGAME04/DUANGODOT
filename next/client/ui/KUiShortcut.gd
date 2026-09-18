# KUiShortcut - the nine shortcut skills of the 2.0 client (the table 0x83f440 of KUiSkillTree: 9 x {genre, id, side};
# autoexec.lua binds Q W E / A S D / Z X C to ShortcutSkill(0..8); docs/CLIENT-2.0.md §8).
#   - ShortcutSkill(k) 0x00495F70 with the tree open: the entry under the mouse becomes shortcut k {id, the tree's side}
#     and any other slot holding the same {id, side} is cleared (0x00495FEA); with the tree closed: slot k, when set,
#     becomes the mouse skill of its side (OperationRequest(0xd, entry, side == right)).
#   - DirectShortcutSkill(k) 0x00496070: the same assignment; closed, slot k is cast at once (0x005BC500).
#   - the table is read / written as [ShortSkill] ShortcutSkill_%d of the character's settings (0x0052D720, 0x00495810);
#     the tree paints the key of an entry that has a slot (0x0049627D) in KeyFont / KeyColor.
extends RefCounted

const SLOTS := 9
const KEYS := ["Q", "W", "E", "A", "S", "D", "Z", "X", "C"]   # autoexec.lua: AddCommand("Q", "", "ShortcutSkill(0)") ...

var slots: Array = []   # [{id, right}] - id 0 = empty


func _init() -> void:
	clear()


func clear() -> void:
	slots.clear()
	for _i in SLOTS:
		slots.append({"id": 0, "right": false})


# ShortcutSkill(k) with the tree open: slot k takes {id, side}; the same skill on the same side elsewhere is cleared
func assign(k: int, skill_id: int, right: bool) -> bool:
	if k < 0 or k >= SLOTS or skill_id <= 0:
		return false
	slots[k] = {"id": skill_id, "right": right}
	for i in SLOTS:
		if i != k and int(slots[i].id) == skill_id and bool(slots[i].right) == right:
			slots[i] = {"id": 0, "right": false}
	return true


func slot(k: int) -> Dictionary:
	if k < 0 or k >= SLOTS:
		return {"id": 0, "right": false}
	return slots[k]


# the slot (and so the key) of an entry of the tree, -1 for none (0x00496260: same side, same id)
func slot_of(skill_id: int, right: bool) -> int:
	for i in SLOTS:
		if int(slots[i].id) == skill_id and bool(slots[i].right) == right:
			return i
	return -1


static func key_of(k: int) -> String:
	return KEYS[k] if k >= 0 and k < SLOTS else ""


# the keycode a slot answers to (Godot key constants of the letters)
static func slot_of_key(keycode: int) -> int:
	match keycode:
		KEY_Q: return 0
		KEY_W: return 1
		KEY_E: return 2
		KEY_A: return 3
		KEY_S: return 4
		KEY_D: return 5
		KEY_Z: return 6
		KEY_X: return 7
		KEY_C: return 8
	return -1


func to_json() -> Array:
	var out := []
	for s in slots:
		out.append({"id": int(s.id), "right": bool(s.right)})
	return out


func from_json(list) -> void:
	clear()
	if not (list is Array):
		return
	for i in mini(SLOTS, list.size()):
		var s = list[i]
		if s is Dictionary:
			slots[i] = {"id": int(s.get("id", 0)), "right": bool(s.get("right", false))}
