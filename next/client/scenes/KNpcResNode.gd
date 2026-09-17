# KNpcResNode of the old client as pure table lookups over one exported resource file
# (assets/npcres/res/<name>.json): which action a doing maps to, the part draw order and the
# frame arithmetic of KNpcRes::Draw.  No scene or autoload access, so it is unit-testable.
extends RefCounted

const KMath := preload("res://scenes/KMath.gd")

# CLIENTACTION of KNpc.h (npc_actions order of the tables)
enum Doing { FIGHT_STAND, STAND, STAND1, FIGHT_WALK, WALK, FIGHT_RUN, RUN, HURT, DEATH, ATTACK, ATTACK1, MAGIC, SIT, JUMP }
const NORMAL_NPC_PART := 5   # NORMAL_NPC_PART_NO
const PART_SECTS := 4        # MAX_BODY_PART_SECT: part index / 4 = equipment group
const CENTER_X := 160        # DrawScaleSprite: sprites without a centre use this when wider than 160
const CENTER_Y := 192


# KNpcResNode::GetActNo: the action of a doing for the weapon kind (equipment row).
static func act_no(res: Dictionary, doing: int, equip: int, ride: bool) -> int:
	if not bool(res.get("special", false)):
		return doing
	var table: Array = res.get("on_horse" if ride else "no_horse", [])
	if equip < 0 or equip >= table.size():
		return -1
	var row: Array = table[equip]
	if doing < 0 or doing >= row.size():
		return -1
	return int(row[doing])


# CSortTable::GetSort: part indices back to front for an action, sprite direction and frame.
# A normal npc has its single image in slot NORMAL_NPC_PART.
static func sort_order(res: Dictionary, act: int, dir: int, frame: int) -> Array:
	if not bool(res.get("special", false)):
		return [NORMAL_NPC_PART]
	var sort: Dictionary = res.get("sort", {})
	var def: Array = sort.get("default", [])
	if def.is_empty() or dir < 0 or dir >= def.size():
		return []
	var a = sort.get("acts", {}).get(str(act))
	if a == null:
		return def[dir]
	for line in a.get("lines", []):
		if int(line[0]) == frame:
			return line.slice(1)
	var dirs: Array = a.get("dirs", [])
	if bool(a.get("use_default", true)) or dir >= dirs.size():
		return def[dir]
	return dirs[dir]


# KNpcRes::Draw: the frame of a sprite with `frames` frames in `dirs` directions when the
# character faces dir64 and is at cur_frame of an all_frame long action.
@warning_ignore("integer_division")
static func frame_no(dir64: int, all_frame: int, cur_frame: int, frames: int, dirs: int) -> int:
	dirs = maxi(dirs, 1)
	frames = maxi(frames, dirs)   # KSprControl::SetSprFile
	if all_frame <= 0:
		all_frame = 1
	var per_dir := frames / dirs
	return KMath.dir64_to_sprite(dir64, dirs) * per_dir + per_dir * cur_frame / all_frame


# The reference spot of a sprite as the old renderer subtracts it (RUIMAGE_RENDER_FLAG_REF_SPOT):
# the header centre, or (160, 192) for wide sprites without one.
static func ref_spot(width: int, center_x: int, center_y: int) -> Vector2:
	if center_x != 0 or center_y != 0:
		return Vector2(center_x, center_y)
	if width > CENTER_X:
		return Vector2(CENTER_X, CENTER_Y)
	return Vector2.ZERO


# Resource name of a player character (rows of 人物类型.txt).
static func player_res_name(sex: int) -> String:
	return "MainLady" if sex == 1 else "MainMan"
