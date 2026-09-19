extends RefCounted
# KNpcGold of the 2.0 client (KNpc+0x4c: SetGoldType 0x006E3560 from the 0x4c / 0x9a packets, GetGoldKind 0x006E3540) and
# what the client makes of the kind: the name colour of the pate painter 0x005F21B0 (0x005F23E5..0x005F2419), the class the
# hang-up target filter sorts a npc into (0x00642550) and the cursor over it (0x0069D25D: 0xf for any kind).  The kind is the
# NpcGoldTemplate.txt row + 1 while gold, 0 plain; the zone sends a boss (+0x181c != 0) as the SERVER table's count + 1, and
# the client compares it with ITS OWN copy's count (gamecl.exe 0x006E35C0 -> [0x21a12c0]; npc_gold.json client_rows).
# docs/CLIENT-2.0.md §16, docs/LINUX-SERVER.md §16.12

const ENTITY_PLAYER := 1    # jx.pb.EntityType
const ENTITY_MONSTER := 3

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


# PaintName 0x005F2507 for a player (kind 1 / 2): the colour of the name by KNpc+0xf8, the current camp of the 0x4c packet
# (NPCCAMP: 0 begin, 1 justice, 2 evil, 3 balance, 4 free, 5 animal, 6 event) through the table 0x5f2d94 - 0xFFFFFFFF, 0xFFFFA85E,
# 0xFFFF92FF, 0xFF55FF91, 0xFFFF0000, above 4 -> 0xFFFF69B4 (0x005F2519 .. 0x005F254B)
static func player_name_color(current_camp: int) -> Color:
	match current_camp:
		0:
			return Color(1.0, 1.0, 1.0)
		1:
			return Color(0xFF / 255.0, 0xA8 / 255.0, 0x5E / 255.0)
		2:
			return Color(0xFF / 255.0, 0x92 / 255.0, 0xFF / 255.0)
		3:
			return Color(0x55 / 255.0, 0xFF / 255.0, 0x91 / 255.0)
		4:
			return Color(1.0, 0.0, 0.0)
		_:
			return Color(0xFF / 255.0, 0x69 / 255.0, 0xB4 / 255.0)


# 0x00642550: 1 plain, 2 gold, 3 above the client's table
static func npc_class(kind: int, client_rows: int) -> int:
	if kind == 0:
		return 1
	return 3 if kind > client_rows else 2


# ---- the two show switches of the option word [0x21a0438+0x1eb4] (gamecl.exe 0x0066B4C0 .. 0x0066B6F9): value B "showplayername"
# = bits 2 | 0x200 (getter 0x0066B600(mask): mask 1 -> bit 2, mask 2 -> bit 0x200; setter 0x0066B5C0), value C "showplayerlife"
# = bits 4 | 0x400 (0x0066B660 / 0x0066B620).  F7 / F8 (autoexec.lua Switch([[showplayername]]) / ([[showplayerlife]]) 0x0042FBF7 /
# 0x0042FC2D: GetGameData(0x402 / 0x403, 2) = 0x0066B4A0(value) = 3 when the value is 0 or 1, else 0 -> OperationRequest 0x2f / 0x30)
# flip a value between 0 and 3; the hang-up options window (0x005E6E8E) sets both to 3 or both to 0.  The object starts with the word
# at 1 (0x0066DACC): both values 0 - THIS client starts with the names on (name_switch 3), a documented choice (docs/CLIENT-2.0.md §17).

# 0x0066B4A0 through GetGameData(0x402 / 0x403, 2): the next value of a switch key
static func toggle_switch(value: int) -> int:
	return 3 if value <= 1 else 0


# the pate loop 0x006702BD .. 0x00670331 for a monster (kind 0): nothing without bit 1 of the name value (0x0066B600(1)); hovered or
# targeted -> the full block (size 14, a black outline - the BorderColor 0xff000000 of OutputText, 0x006702ED); else bit 2
# (0x0066B600(2)) -> the block at size 12 (0x0067031A);
# else nothing (show flag 0: PaintName 0x005F2316 returns for a monster).  Players and the other kinds always get their name line.
# Returns 0 hidden, 12 / 14 the font size of the block.
static func name_block(entity_type: int, name_switch: int, focus: bool) -> int:
	if entity_type != ENTITY_MONSTER:
		return 12
	if (name_switch & 1) == 0:
		return 0
	if focus:
		return 14
	return 12 if (name_switch & 2) != 0 else 0


# the life bar of the same loop (0x00670243 .. 0x0067029A -> KNpc::PaintLife 0x005EACF0): a player (kind 1 / 2) with bit 1 of the
# life value (0x0066B660(1)), a monster hovered / targeted or with bit 2 (0x0066B660(2)); the other kinds never (PaintLife refuses
# them unless forced, and the loop forces only monsters)
static func life_bar(entity_type: int, life_switch: int, focus: bool) -> bool:
	if entity_type == ENTITY_PLAYER:
		return (life_switch & 1) != 0
	if entity_type == ENTITY_MONSTER:
		return focus or (life_switch & 2) != 0
	return false


# PaintLife 0x005EADF4 .. 0x005EAE5B: the colour of the filled part by the percent - green (0, 255, 0) from 50, yellow (255, 255, 0)
# from 25, red (255, 0, 0) below; the team mate (0x0066D070 == 8: 230, 190, 0), the PK states of KNpc+0x16e4 / +0x16e8 (pink
# 255, 105, 180; red 255, 0, 0 / 255, 0, 64) wait for the team and PK systems
# PaintLife 0x005EADB8..0x005EAE5B in order: a team mate (0x0066D070 == 8, not on this client) (230, 190, 0); the PK flag
# (KNpc+0x16e8, the switch on) -> (255, 0, 0) for the kill state 2, (255, 0, 64) otherwise; no flag but state 2 -> (255, 105, 180);
# else by the percent: 50 green, 25 yellow, below red
static func life_bar_color(pct: int, pk_state: int = 0, pk_flag: bool = false) -> Color:
	if pk_flag:
		return Color(1.0, 0.0, 0.0) if pk_state == 2 else Color(1.0, 0.0, 64.0 / 255.0)
	if pk_state == 2:
		return Color(1.0, 105.0 / 255.0, 180.0 / 255.0)
	if pct >= 50:
		return Color(0.0, 1.0, 0.0)
	if pct >= 25:
		return Color(1.0, 1.0, 0.0)
	return Color(1.0, 0.0, 0.0)


# KNpc::GetNpcPate 0x005EBCF0 (2004 KNpc.cpp:6148): a sitting player's head (m_Doing 8) sinks with the sit animation - once
# MulDiv(10, cur, total) >= 8 the pate loses MulDiv(30, cur, total) (24 / 26 / 28 over the last three of 15 frames, held at 28);
# MulDiv rounds half up.  The 2.0 client skips it for the armour kind 45 (+0x13f4, not on this client yet).
@warning_ignore("integer_division")
static func sit_pate_drop(sitting: bool, cur_frame: int, total_frame: int) -> int:
	if not sitting or total_frame <= 0:
		return 0
	if (10 * cur_frame + total_frame / 2) / total_frame < 8:
		return 0
	return (30 * cur_frame + total_frame / 2) / total_frame
