# KMissleResMath - the frame arithmetic of KMissleRes::Draw (Core/Src/KMissleRes.cpp 113) and KMissle::CreateSpecialEffect
# (KMissle.cpp 1998) of the old client, which the 2.0 client keeps: an AnimFile of missles.txt is one sprite with
# `dirs` direction blocks of `frames / dirs` frames each ("AnimFileInfo" = frames,dirs,interval).  No scene or autoload
# access: the headless tests run it.  docs/CLIENT-2.0.md §11
extends RefCounted


# the direction block of a 0..63 direction (KMissleRes::Draw 137..140): the nearest of `dirs` spokes, wrapping to 0
@warning_ignore("integer_division")
static func sprite_dir(dir64: int, dirs: int) -> int:
	if dirs <= 0 or dirs > 64:
		return 0
	var per := 64 / dirs
	if per <= 0:
		return 0
	var image_dir := dir64 / per
	if dir64 % per >= 32 / dirs:
		image_dir += 1
	if image_dir >= dirs:
		image_dir = 0
	return image_dir


# the frame inside a direction block of the flight (KMissleRes::Draw 124..174): -1 = nothing to draw (before birth, past
# the life or past the last frame).  `cur` = frames flown, `all` = the frames of the flight (0 = one pass of the block)
@warning_ignore("integer_division")
static func fly_frame(total: int, dirs: int, interval: int, cur: int, all: int, loop: bool, sub_loop: bool, sub_start: int, sub_stop: int) -> int:
	if dirs <= 0 or total <= 0:
		return -1
	if cur < 0 or (all != 0 and all < cur):
		return -1
	var per_dir := total / dirs
	if per_dir <= 0:
		return -1
	if all == 0:
		all = per_dir
	interval = maxi(1, interval)
	var frame: int
	if loop:
		if not sub_loop:
			frame = (cur / interval) % per_dir
		elif cur / interval < sub_start:
			frame = cur / interval
		elif sub_start == sub_stop:
			frame = sub_start
		else:
			frame = sub_start + ((cur - sub_start) / interval) % (sub_stop - sub_start)
	else:
		frame = per_dir * cur / all
	if frame > per_dir - 1:
		return -1
	return frame


# the frame of the whole sprite for a direction and a moment of the flight (-1 = nothing)
@warning_ignore("integer_division")
static func frame_index(total: int, dirs: int, interval: int, dir64: int, cur: int, all: int, loop: bool, sub_loop: bool, sub_start: int, sub_stop: int) -> int:
	var f := fly_frame(total, dirs, interval, cur, all, loop, sub_loop, sub_start, sub_stop)
	if f < 0:
		return -1
	return sprite_dir(dir64, dirs) * (total / dirs) + f


# a special movie (the vanish / collision effect, CreateSpecialEffect 2028): it lasts interval x frames-of-a-block game
# frames; its frame advances every `interval` frames through the direction block.  -1 = over
@warning_ignore("integer_division")
static func special_frame(total: int, dirs: int, interval: int, dir64: int, elapsed: int) -> int:
	if dirs <= 0 or total <= 0 or elapsed < 0:
		return -1
	var per_dir := total / dirs
	interval = maxi(1, interval)
	if elapsed >= interval * per_dir:
		return -1
	return sprite_dir(dir64, dirs) * per_dir + elapsed / interval


# ZAxisMove of KMissle (the zone's missle_activate, KMissle::OnFlyFPS 1018 with one step): one frame of the climb of a
# missile with a Z acceleration - {height, speed} after it, the height never below the ground
static func z_step(height: int, height_speed: int, z_acceleration: int) -> Array:
	var h := height + height_speed
	if h < 0:
		h = 0
	return [h, height_speed - z_acceleration]
