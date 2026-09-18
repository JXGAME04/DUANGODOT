# KUiStateMath - the numbers of the 2.0 skill state list (KUiSkillState, gamecl.exe 技能状态列表.ini; docs/CLIENT-2.0.md
# §9), kept apart from the window script so the headless tests reach them.
extends RefCounted

const SLOT_PITCH := 24     # 0x0041F628: BuffImage[i] at x = 24 i
const DEBUFF_ROW_Y := 36   # 0x0041F5E7: DebuffImage[i] at y = 0x24
const SLOTS := 10          # 0x0041F637: ten of each (0xf0 / 0x18)
const FRAMES_PER_SECOND := 18   # a state's time is frames of the zone tick


# 0x0041D720(seconds, buf, size, long_form): "N/A" at or under 0; long_form -> "%02dh:%02dm:%02ds"; else "%ds" under a
# minute, "%dm" under an hour, "%dh" under 99 hours (0x57030 seconds), "%dd" beyond
static func time_text(seconds: int, long_form: bool = false) -> String:
	if seconds <= 0:
		return "N/A"
	@warning_ignore("integer_division")
	var h: int = seconds / 3600
	@warning_ignore("integer_division")
	var m: int = (seconds - h * 3600) / 60
	if long_form:
		return "%02dh:%02dm:%02ds" % [h, m, seconds - h * 3600 - m * 60]
	if seconds < 60:
		return "%ds" % seconds
	if seconds < 3600:
		return "%dm" % m
	if seconds < 356400:
		return "%dh" % h
	@warning_ignore("integer_division")
	return "%dd" % (seconds / 86400)


# the top-left of slot i of the buff row (y 0) or the debuff row (y 36)
static func slot_pos(i: int, debuff: bool) -> Vector2i:
	return Vector2i(SLOT_PITCH * i, DEBUFF_ROW_Y if debuff else 0)


# frames left -> whole seconds left (the zone ticks 18 times a second); -1 = a passive state (no time)
static func seconds_left(frames: int) -> int:
	if frames < 0:
		return -1
	@warning_ignore("integer_division")
	return (frames + FRAMES_PER_SECOND - 1) / FRAMES_PER_SECOND
