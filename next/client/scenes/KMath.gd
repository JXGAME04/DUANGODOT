# KMath.h of the old client: the 64-direction index of a movement, 0 = down and clockwise on
# screen (16 = left, 32 = up, 48 = right).  The 2.0 client (gamecl.exe 0x005E8DB0) searches the
# full g_nSin table the way the JX2 server does (0x080EEEC0, server/zone/include/jx/zone/KMath.h,
# shared verbatim); the JX1 KMath.h searched a half-step boundary table and mirrored as 63 - i.
extends RefCounted

# g_nSin x1024: sin(i * 5.625 degrees) for the 64 directions
const SIN64 := [1024, 1019, 1004, 979, 946, 903, 851, 791, 724, 649, 568, 482, 391, 297, 199, 100,
	0, -100, -199, -297, -391, -482, -568, -649, -724, -791, -851, -903, -946, -979, -1004, -1019,
	-1024, -1019, -1004, -979, -946, -903, -851, -791, -724, -649, -568, -482, -391, -297, -199, -100,
	0, 100, 199, 297, 391, 482, 568, 649, 724, 791, 851, 903, 946, 979, 1004, 1019]


# g_GetDirIndex (gamecl.exe 0x005E8DB0, docs/CLIENT-2.0.md §12): direction 0..63 from (x1, y1) to (x2, y2) in scene
# units, -1 when they coincide.  nsin = (dy << 10) / distance is looked up in g_nSin: the last i (0..31) whose sine is
# still >= nsin, moved to i + 1 when that one is the nearer of the two, then mirrored for dx >= 0 as 64 - i; 0 stays 0.
@warning_ignore("integer_division")
static func get_dir_index(x1: int, y1: int, x2: int, y2: int) -> int:
	var dx := x2 - x1
	var dy := y2 - y1
	if dx == 0 and dy == 0:
		return -1
	var dist := int(sqrt(float(dx * dx + dy * dy)))
	if dist == 0:
		return -1
	var nsin := (dy << 10) / dist
	var k := 31
	for i in 32:
		if nsin > SIN64[i]:
			k = i - 1
			break
	if k < 0:
		k = 0
	if SIN64[k] != nsin and SIN64[k] - nsin > nsin - SIN64[k + 1]:
		k += 1
	if k == 0:
		return 0
	return k if dx < 0 else 64 - k


# KNpcRes::Draw / KSprControl::SetCurDir64: the sprite direction (of `dirs`) for a 64-direction.
@warning_ignore("integer_division")
static func dir64_to_sprite(dir64: int, dirs: int) -> int:
	if dirs <= 0:
		dirs = 1
	var d := (dir64 + 32 / dirs) / (64 / dirs)
	if d >= dirs:
		d -= dirs
	return d


# KNpc::OnKnockBack (gamecl.exe 0x005EFE00, docs/CLIENT-2.0.md §12): one frame's share of the way left to the spot,
# ((spot - here) << 10) / (frames left, at least 1) in 1/1024 units, the division truncating toward zero (0x005ECEC0
# then adds the 1/1024 parts to the position).
@warning_ignore("integer_division")
static func knock_step(here: Vector2, spot: Vector2, left: int) -> Vector2:
	var n := maxi(left, 1)
	var dx := (int(spot.x) - int(here.x)) << 10
	var dy := (int(spot.y) - int(here.y)) << 10
	return Vector2(float(dx / n) / 1024.0, float(dy / n) / 1024.0)
