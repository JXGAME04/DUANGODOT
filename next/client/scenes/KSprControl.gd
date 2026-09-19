# KSprControl of the old client (Core/Src/KSprControl.cpp; gamecl.exe 2.0 0x0070B540..0x0070B9B4): which frame of
# an animated sprite shows now.  A sprite holds `total_frame` frames in `total_dir` direction blocks; a pass through
# one block takes `interval` logic frames (SubWorld[0].m_dwCurrentTime, the 18 Hz counter the caller passes as `now`):
#   frame = block start + (frames per block) * elapsed / interval        (GetNextFrame 0x0070B920)
# and at the end of the pass a looping picture starts over, a once-only one holds its last frame (CheckEnd 0x0070B880).
extends RefCounted

var total_frame := 1      # m_nTotalFrame (+0x4)
var cur_frame := 0        # m_nCurFrame (+0x8): the frame to draw, an index into the whole sprite
var total_dir := 1        # m_nTotalDir (+0xc)
var cur_dir := 0          # m_nCurDir (+0x10)
var timer := 0            # m_dwTimer (+0x14): the logic frame the current pass began
var interval := 1         # m_dwInterval (+0x18): logic frames of one pass
var file := ""            # m_szName (+0x2c): the sprite (the exported atlas id here)


# KSprControl::Release 0x0070B540
func release() -> void:
	total_frame = 1
	cur_frame = 0
	total_dir = 1
	cur_dir = 0
	timer = 0
	interval = 1
	file = ""


# KSprControl::SetSprFile 0x0070B580: a new sprite starts its pass now; the same sprite again changes nothing.
func set_spr_file(name: String, frames: int, dirs: int, interval_: int, now: int) -> void:
	if name == "":
		release()
		return
	if name == file:
		return
	file = name
	total_dir = maxi(dirs, 1)
	total_frame = maxi(maxi(frames, total_dir), 1)
	interval = interval_
	timer = now
	cur_frame = 0
	cur_dir = 0


# KSprControl::CheckExist
func check_exist() -> bool:
	return total_frame > 0 and file != ""


# KSprControl::GetOneDirFrames
@warning_ignore("integer_division")
func one_dir_frames() -> int:
	return total_frame / total_dir


# KSprControl::SetCurDir64 0x0070B7C0: the block of a 0..63 facing (nearest block); a change restarts the pass at the
# block's first frame and returns false (the caller skips this frame's step), the same block returns true.
@warning_ignore("integer_division")
func set_cur_dir64(dir64: int, now: int) -> bool:
	if dir64 < 0 or dir64 > 63:
		return false
	var d := (dir64 + 32 / total_dir) / (64 / total_dir)
	if d >= total_dir:
		d -= total_dir
	if cur_dir == d:
		return true
	cur_dir = d
	cur_frame = one_dir_frames() * d
	timer = now
	return false


# KSprControl::GetNextFrame 0x0070B920: the frame of the moment; false only without a sprite.
@warning_ignore("integer_division")
func get_next_frame(now: int, loop: bool) -> bool:
	if file == "":
		return false
	if interval <= 0:
		interval = 1
	var fpd := one_dir_frames()
	var elapsed := now - timer
	if elapsed >= interval or elapsed < 0:   # the binary compares unsigned: a clock behind the timer counts as over
		if loop:
			timer = now
			cur_frame = fpd * cur_dir
		else:
			cur_frame = fpd * (cur_dir + 1) - 1
	else:
		cur_frame = fpd * cur_dir + fpd * elapsed / interval
	return true


# KSprControl::CheckEnd 0x0070B880: the last frame of the current block shows.
func check_end() -> bool:
	return cur_frame == one_dir_frames() * (cur_dir + 1) - 1


# KSprControl::GetCurDirFrameNo: the frame within its block.
func cur_dir_frame_no() -> int:
	return cur_frame - cur_dir * one_dir_frames()
