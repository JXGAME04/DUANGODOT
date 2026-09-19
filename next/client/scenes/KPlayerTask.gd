# KPlayerTask.gd - the task values the zone mirrors on the client (KPlayer::SetTaskValue 0x00601ED0 of the 2.0 client:
# the map at KPlayer+0xa1a0, fed by the 0xa7 packet 0x006512F0 {id, value} and the 0xb5 packet 0x00651350 of up to 79
# {id, value} pairs; docs/CLIENT-2.0.md §25).  Pure functions over the Dictionary id -> value that Game holds, so the
# rules are testable without the autoloads.
extends RefCounted

const VALUE_COUNT := 0x1770   # ids 0..0x176f like the server's KPlayerTask (docs/LINUX-SERVER.md §21)


# the value of an id, 0 when the zone never sent it (the client's map lookup 0x00616D60: 0 when absent)
static func value_of(values: Dictionary, id: int) -> int:
	return int(values.get(id, 0))


# SetTaskValue: the value stored - a 0 drops the entry like the server map does; true when it changed
static func set_value(values: Dictionary, id: int, value: int) -> bool:
	if id < 0 or id >= VALUE_COUNT:
		return false
	if value_of(values, id) == value:
		return false
	if value == 0:
		values.erase(id)
	else:
		values[id] = value
	return true


# a G2C_TASK_VALUES batch (the 0xb5 packet): the pairs applied in order; the ids that changed.  (The 2.0 client stops
# at an id 0 because its packet is a fixed array of eighty; ours carries only what was sent, so every pair counts)
static func apply_batch(values: Dictionary, pairs: Array) -> Array:
	var changed: Array = []
	for p in pairs:
		var id := int(p.get("id", 0))
		if set_value(values, id, int(p.get("value", 0))):
			changed.append(id)
	return changed


# GetBits of the server (0x080CB5E0): `count` bits from `start` of a value, 0 when the arguments make no bit field
static func bits(value: int, start: int, count: int) -> int:
	if count <= 0 or start < 0 or start + count > 32:
		return 0
	return ((value & 0xFFFFFFFF) >> start) & ((1 << count) - 1)
