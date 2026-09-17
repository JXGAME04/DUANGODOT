# KMath.h of the old client: the 64-direction index of a movement, 0 = down and clockwise on
# screen (16 = left, 32 = up, 48 = right).  The boundary table is 1024 * cos((i - 1/2) * 5.625
# degrees); the old table was read from the copy-protection dongle, so it is regenerated here and
# shared verbatim with the zone (server/zone/include/jx/zone/KMath.h).
extends RefCounted

const SIN := [1024, 1022, 1012, 993, 964, 925, 878, 822, 758, 687, 609, 526, 437, 344, 248, 150,
	50, -50, -150, -248, -344, -437, -526, -609, -687, -758, -822, -878, -925, -964, -993, -1012]


# g_GetDirIndex: direction 0..63 from (x1, y1) to (x2, y2) in scene units, -1 when they coincide.
@warning_ignore("integer_division")
static func get_dir_index(x1: int, y1: int, x2: int, y2: int) -> int:
	if x1 == x2 and y1 == y2:
		return -1
	var dist := int(sqrt(float((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2))))
	if dist == 0:
		return -1
	var nsin := ((y2 - y1) << 10) / dist
	var ret := -1
	for i in 32:
		if nsin > SIN[i]:
			break
		ret = i
	if x2 - x1 > 0:
		ret = 63 - ret
	return ret


# KNpcRes::Draw / KSprControl::SetCurDir64: the sprite direction (of `dirs`) for a 64-direction.
@warning_ignore("integer_division")
static func dir64_to_sprite(dir64: int, dirs: int) -> int:
	if dirs <= 0:
		dirs = 1
	var d := (dir64 + 32 / dirs) / (64 / dirs)
	if d >= dirs:
		d -= dirs
	return d
