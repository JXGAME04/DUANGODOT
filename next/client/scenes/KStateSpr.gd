# KStateSpr of the old client (Core/Src/KNpcRes.h): one state picture in a slot of a character's KNpcRes - the row of
# the state graphics table (StateSpecialId), its type and play type, the frames a Body picture spends behind the
# character, and the KSprControl that steps it.  The 2.0 client keeps six slots (KNpcRes+0x15b0, stride 0xa0: +0 id,
# +4 type, +8 not-loop, +0xc/+0x10 behind range, +0x14 split, +0x18 loaded, +0x1c the control).
extends RefCounted

const KSprControl := preload("res://scenes/KSprControl.gd")
const MAX_STATE_SPR := 6          # the 2.0 client's slots (the 2004 source had 18, six per type)
# StateMagicType (KNpcResNode.h; the 2.0 loader 0x006AE200 adds MiniMap)
const STATE_HEAD := 0
const STATE_BODY := 1
const STATE_FOOT := 2
const STATE_MINIMAP := 3

var id := 0                       # m_nID: the row of the table (StateSpecialId), 0 = a free slot
var type := STATE_BODY            # m_nType
var loop := true                  # m_nPlayType 0 = Loop
var back_start := 0               # m_nBackStart: a Body picture is behind the character for [back_start, back_end)
var back_end := 0                 # m_nBackEnd
var split := 1                    # 2.0 slot +0x14 (every row is 1; 2 / 3 would cut the picture, 0x0070B900)
var loaded := true                # 2.0 slot +0x18: a once-only picture that reached its end is dropped
var ctrl := KSprControl.new()     # m_SprContrul
var sprite: Sprite2D = null       # the drawn node (KNpcRes makes it once the atlas is known)
var atlas = null                  # Assets.SpriteAtlas


# KStateSpr::Release (2.0: 0x006DDFB0)
func release() -> void:
	id = 0
	loaded = false
	ctrl.release()
	if sprite != null:
		sprite.queue_free()
		sprite = null
	atlas = null


# Drawn behind the body this frame (KNpcRes::Draw 0x006E07A0 / 0x006E0843: Foot always, Body within its range).
func behind() -> bool:
	return type == STATE_FOOT or (type == STATE_BODY and back_start <= ctrl.cur_frame and ctrl.cur_frame < back_end)


# KNpcRes::SetState (2.0: 0x006DF7E0 from the state list the 0x7a packet built): `icons` are the StateSpecialIds
# held; slots whose id is gone are freed, a new id takes the first free slot with the table row `row_of(id)` gives
# (CStateMagicTable::GetInfo: sprite, type, loop, behind range, frames, dirs, interval, split), skipped when the row
# has no sprite (the "Special" rows) or a type past MiniMap.  Returns the slots that got a new picture.
static func sync(slots: Array, icons: Array, row_of: Callable, now: int) -> Array:
	while slots.size() < MAX_STATE_SPR:
		slots.append(new())
	var keep := {}
	for v in icons:
		if int(v) > 0:
			keep[int(v)] = true
	for s in slots:
		if s.id != 0 and not keep.has(s.id):
			s.release()
	var fresh: Array = []
	for v in keep:
		var found := false
		for s in slots:
			if s.id == int(v):
				found = true
				break
		if found:
			continue
		var row = row_of.call(int(v))
		if row == null or row.is_empty():
			continue
		var sid := str(row.get("sprite", ""))
		if sid == "" or int(row.get("type", STATE_BODY)) > STATE_MINIMAP:
			continue
		for s in slots:
			if s.id != 0:
				continue
			s.release()
			s.id = int(v)
			s.type = int(row.get("type", STATE_BODY))
			s.loop = bool(row.get("loop", true))
			s.back_start = int(row.get("behind_start", 0))
			s.back_end = int(row.get("behind_end", 0))
			s.split = clampi(int(row.get("split", 1)), 1, 3)
			s.loaded = true
			s.ctrl.set_spr_file(sid, int(row.get("frames", 1)), int(row.get("dirs", 1)), int(row.get("interval", 1)), now)
			fresh.append(s)
			break
	return fresh


# KNpcRes::Draw 0x006E0610: one logic frame of the picture - its block follows the facing (a turn restarts the pass
# and skips the step), a Loop one runs on, a once-only one is dropped at its last frame.
func step(dir64: int, now: int) -> void:
	if id == 0 or not ctrl.check_exist():
		return
	if not ctrl.set_cur_dir64(dir64, now):
		return
	if loop:
		ctrl.get_next_frame(now, true)
	elif ctrl.get_next_frame(now, false) and ctrl.check_end():
		loaded = false
