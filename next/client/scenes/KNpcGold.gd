extends RefCounted
# KNpcGold of the 2.0 client (KNpc+0x4c: SetGoldType 0x006E3560 from the 0x4c / 0x9a packets, GetGoldKind 0x006E3540) and
# what the client makes of the kind: the name colour of the pate painter 0x005F21B0 (0x005F23E5..0x005F2419), the class the
# hang-up target filter sorts a npc into (0x00642550) and the cursor over it (0x0069D25D: 0xf for any kind).  The kind is the
# NpcGoldTemplate.txt row + 1 while gold, 0 plain; the zone sends a boss (+0x181c != 0) as the SERVER table's count + 1, and
# the client compares it with ITS OWN copy's count (gamecl.exe 0x006E35C0 -> [0x21a12c0]; npc_gold.json client_rows).
# docs/CLIENT-2.0.md §16, docs/LINUX-SERVER.md §16.12

const ENTITY_MONSTER := 3   # jx.pb.EntityType

# the ARGB colours of 0x005F2401..0x005F2419: -1 white, 0xFF6365FF within the table, 0xFFEBB200 above it
const NAME_COLOR_PLAIN := Color(1.0, 1.0, 1.0)
const NAME_COLOR_GOLD := Color(0x63 / 255.0, 0x65 / 255.0, 0xFF / 255.0)
const NAME_COLOR_BOSS := Color(0xEB / 255.0, 0xB2 / 255.0, 0x00 / 255.0)


# 0x005F242F: a monster's line is "%s/Lv:%d"; a townsman (kind 3, 0x005F2283) and a player keep the name
static func name_text(name_: String, entity_type: int, level: int) -> String:
	return "%s/Lv:%d" % [name_, level] if entity_type == ENTITY_MONSTER else name_


static func name_color(entity_type: int, kind: int, client_rows: int) -> Color:
	if entity_type != ENTITY_MONSTER or kind == 0:
		return NAME_COLOR_PLAIN
	return NAME_COLOR_BOSS if kind > client_rows else NAME_COLOR_GOLD


# 0x00642550: 1 plain, 2 gold, 3 above the client's table
static func npc_class(kind: int, client_rows: int) -> int:
	if kind == 0:
		return 1
	return 3 if kind > client_rows else 2
