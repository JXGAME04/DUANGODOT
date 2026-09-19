# Scene units of the zone <-> metres of the 3D world (docs/3D-QUY-UOC.md).  The zone keeps the old 2D
# units (cell 32, region 512 x 1024, dir 0..63 at 18 Hz); only the drawing changes.  Sources: the 2.0
# projection sx = x, sy = y/2 - z*887/1024 (KRepresentShell2::CoordinateTransform: an orthographic view
# raised 30 degrees), the male stand sprite 75 px tall = 86.6 units, the player's name 84 px up
# (GetNpcPate, gamecl.exe 0x005EC13D); UNIT = 0.02 m makes him 1.73 m and his name 1.94 m up, next to
# the reference client's sys_bar 2.03..2.15 m.
extends RefCounted

const UNIT := 0.02                 # metres per scene unit (our choice, see the doc)
const CELL := 32                   # obstacle cell, scene units (KRegion)
const REGION_W := 512
const REGION_H := 1024
const COS30 := 887.0 / 1024.0      # the 2.0 client's z factor: cos 30 degrees
const CLASSIC_PITCH_DEG := 30.0    # the 2.0 view: raised 30 degrees over the ground, orthographic
const DIR_STEP_DEG := 360.0 / 64.0 # 5.625 degrees per direction index


# Scene (x, y) with a drawing height z (scene units) -> Godot metres: +y of the screen (down, toward the
# classic camera) is +Z, height is +Y.
static func to_world(scene: Vector2, z: float = 0.0) -> Vector3:
	return Vector3(scene.x * UNIT, z * UNIT, scene.y * UNIT)


static func to_scene(world: Vector3) -> Vector2:
	return Vector2(world.x / UNIT, world.z / UNIT)


# A screen-pixel height of the 2.0 client (the name block, the head pictures) -> metres above the feet:
# the 30 degree view foreshortens heights by cos 30.
static func px_height_to_m(px: float) -> float:
	return px / COS30 * UNIT


# dir 0..63 (0 = down / +y / +Z, clockwise on the screen: 16 = -x, 32 = -y, 48 = +x) -> yaw in degrees for a
# model that looks down -Z at yaw 0 (Node3D.rotation.y): dir 32 -> 0, dir 0 -> 180, dir 16 -> 90, dir 48 -> -90.
static func yaw_of_dir(dir64: int) -> float:
	return float(32 - dir64) * DIR_STEP_DEG


static func dir_of_yaw(yaw_deg: float) -> int:
	return posmod(32 - int(roundf(yaw_deg / DIR_STEP_DEG)), 64)


# The facing as a unit vector on the ground (scene units): the direction the index points at.
static func dir_vector(dir64: int) -> Vector2:
	var a := deg_to_rad(float(dir64) * DIR_STEP_DEG)
	# dir 0 = +y, dir 16 = -x: x = -sin, y = cos
	return Vector2(-sin(a), cos(a))


# Shortest signed turn from yaw a to yaw b, degrees in (-180, 180].
static func yaw_delta(a_deg: float, b_deg: float) -> float:
	return wrapf(b_deg - a_deg, -180.0, 180.0)


# Obstacle cell of a scene position.
static func cell_of(scene: Vector2) -> Vector2i:
	return Vector2i(int(floorf(scene.x / CELL)), int(floorf(scene.y / CELL)))
