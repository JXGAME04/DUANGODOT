# KMissle - a missile of a cast on the client, drawn the way KMissle::Paint (Core/Src/KMissle.cpp 1519) and
# KMissleRes::Draw draw it.  The 2.0 client re-runs CastMissles itself from the 0x5a packet; here the zone that already
# flies the missile tells the client (G2C_MISSLE: born, the first flying frame and every sixth, gone) and the node
# flies it in between by the vector and the speed (KMissle::OnFly).  A deliberate deviation, docs/CLIENT-2.0.md §11.
#   - status wait: nothing drawn; fly: AnimFile2 of missles.txt (the fly status), the frame from the frames flown;
#   - the end: the vanish movie AnimFile3 (CreateSpecialEffect(MS_DoVanish)) plays once at the last spot, then the node
#     frees itself (KMissle::Paint 1537: the missile stays until its effects have all played);
#   - MultiShow: the B set of anims at random (KMissle::Init 1704);
#   - sounds (KMissleRes::PlaySound 0x00717ED0 through `sounds`, a KWavSound): SndFile2 once when the flight starts
#     (KMissle::Activate 0x006B393D, PrePareFly at start_life_time), SndFile3 with the vanish movie
#     (CreateSpecialEffect 0x006B1DC0: only when AnimFile3 exists), never while the same file still plays.
extends Node2D

const KMissleResMath := preload("res://scenes/KMissleResMath.gd")
const KNpcResNode := preload("res://scenes/KNpcResNode.gd")
const KMath := preload("res://scenes/KMath.gd")
const TICK := 1.0 / 18.0
const STATUS_WAIT := 0
const STATUS_FLY := 1
const STATUS_VANISHED := 2
const ANIM_FLY := 1        # AnimFile2 (MS_DoFly)
const ANIM_VANISH := 2     # AnimFile3 (MS_DoVanish)

signal gone(index: int)

var index := 0
var missle_id := 0
var skill_id := 0
var scene_pos := Vector2.ZERO
var z := 0                    # m_nCurrentMapZ = height >> 10
var height := 0               # m_nHeight, 1/1024 units
var height_speed := 0         # m_nHeightSpeed
var z_acceleration := 0       # m_nZAcceleration
var dir64 := 0
var x_factor := 0
var y_factor := 0
var speed := 0
var life_time := 0
var start_life_time := 0
var cur_life := 0
var status := STATUS_WAIT
var res := {}                 # the row of missle_res.json
var anims: Array = []         # the four status anims in use (A or B set)
var _sprite: Sprite2D = null
var sounds = null             # the world's KWavSound (UiGame), null = silent
var _fly_sounded := false     # SndFile2 played once
var _vanish_started := -1     # cur_life when the vanish movie began
var _tick_acc := 0.0
var _rng := RandomNumberGenerator.new()
var no_2d := false            # a 3D view draws it: no sprite (is_drawn stays false)


func _ready() -> void:
	if no_2d:
		visible = false
		return
	_sprite = Sprite2D.new()
	_sprite.centered = false   # the old renderer draws a frame from its top-left: spot - centre + frame offset (KRepresentShell2.cpp 2004)
	_sprite.visible = false
	add_child(_sprite)
	_refresh()


static func to_screen(p: Vector2) -> Vector2:
	return Vector2(p.x, p.y * 0.5)


func setup(d: Dictionary, row: Dictionary) -> void:
	index = int(d.get("index", 0))
	missle_id = int(d.get("missle_id", 0))
	skill_id = int(d.get("skill_id", 0))
	res = row
	anims = row.get("anims", [])
	if bool(row.get("multi_show", false)) and row.get("anims_b", []).size() > 0 and _rng.randi_range(0, 1) == 1:
		anims = row.get("anims_b", [])
	apply(d)


# a sync from the zone: the spot and the frame counters are the zone's word
func apply(d: Dictionary) -> void:
	scene_pos = Vector2(float(d.get("x", 0)), float(d.get("y", 0)))
	z = int(d.get("z", 0))
	height = int(d.get("height", z << 10))
	height_speed = int(d.get("height_speed", 0))
	z_acceleration = int(d.get("z_acceleration", 0))
	dir64 = clampi(int(d.get("dir", 0)), 0, 63)
	if z_acceleration != 0:
		# KMissle::Paint 1531: a missile with a Z acceleration faces its vector (g_GetDirIndex(0, 0, XFactor, YFactor) through
		# g_DirIndex2Dir(., 64), which is the index itself); the vector is what the zone sent
		var facing := KMath.get_dir_index(0, 0, int(d.get("x_factor", 0)), int(d.get("y_factor", 0)))
		if facing >= 0:
			dir64 = facing
	x_factor = int(d.get("x_factor", 0))
	y_factor = int(d.get("y_factor", 0))
	speed = int(d.get("speed", 0))
	life_time = int(d.get("life_time", 0))
	start_life_time = int(d.get("start_life_time", 0))
	cur_life = int(d.get("current_life", 0))
	if bool(d.get("removed", false)) or int(d.get("status", 0)) == STATUS_VANISHED:
		_begin_vanish()
	else:
		status = int(d.get("status", 0))
		if status == STATUS_FLY:
			_on_fly_start()
	_tick_acc = 0.0
	_place()
	_refresh()


# KMissle::Activate 0x006B38C8: at start_life_time PrePareFly -> the fly sound (SndFile2) -> DoFly
func _on_fly_start() -> void:
	if _fly_sounded:
		return
	_fly_sounded = true
	if sounds != null:
		sounds.play(str(_anim(ANIM_FLY).get("sound", "")), scene_pos, false, true)


func _begin_vanish() -> void:
	if status == STATUS_VANISHED:
		return
	status = STATUS_VANISHED
	_vanish_started = cur_life
	# CreateSpecialEffect(MS_DoVanish) 0x006B38BA: the vanish movie with its sound (SndFile3), only when the movie exists
	var a := _anim(ANIM_VANISH)
	if sounds != null and str(a.get("sprite", "")) != "":
		sounds.play(str(a.get("sound", "")), scene_pos, false, true)


func _process(delta: float) -> void:
	_tick_acc += delta
	while _tick_acc >= TICK:
		_tick_acc -= TICK
		_tick()


# one logic frame between two syncs: KMissle::OnFly moves the vector times the speed (1/1024 units); the wait ends
# at start_life_time and the life at life_time - the zone's word overrides when it comes
func _tick() -> void:
	cur_life += 1
	if status == STATUS_FLY:
		if z_acceleration != 0:
			# ZAxisMove (KMissle::OnFlyFPS 1018 with one step, the zone's missle_activate): climb, never below the ground,
			# z = height >> 10, then the speed loses the acceleration
			var hs: Array = KMissleResMath.z_step(height, height_speed, z_acceleration)
			height = int(hs[0])
			height_speed = int(hs[1])
			z = height >> 10
		scene_pos += Vector2(float(x_factor * speed) / 1024.0, float(y_factor * speed) / 1024.0)
		if cur_life >= life_time:
			_begin_vanish()
	elif status == STATUS_WAIT and cur_life >= start_life_time:
		status = STATUS_FLY
		_on_fly_start()
	_place()
	_refresh()


func is_drawn() -> bool:
	return _sprite != null and _sprite.visible


# the frame's rectangle on the scene (the node's position + the sprite's top-left, the texture's size)
func drawn_rect() -> Rect2:
	if not is_drawn() or _sprite.texture == null:
		return Rect2(position, Vector2.ZERO)
	return Rect2(position + _sprite.position, _sprite.texture.get_size())


func _place() -> void:
	position = to_screen(scene_pos) + Vector2(0, -float(z))


func _anim(i: int) -> Dictionary:
	if i < 0 or i >= anims.size() or not (anims[i] is Dictionary):
		return {}
	return anims[i]


func _show_frame(anim: Dictionary, frame: int) -> bool:
	if _sprite == null or frame < 0 or str(anim.get("sprite", "")) == "":
		return false
	var atlas = Assets.sprite(str(anim.sprite))
	if atlas == null or atlas.frame_count() == 0:
		return false
	var f := clampi(frame, 0, atlas.frame_count() - 1)
	var tex: Texture2D = atlas.frame_texture(f)
	if _sprite.texture != tex:
		_sprite.texture = tex
	_sprite.position = -KNpcResNode.ref_spot(atlas.width, atlas.center_x, atlas.center_y) + atlas.frame_offset(f)
	_sprite.visible = true
	return true


func _refresh() -> void:
	if _sprite == null:
		# a 3D view draws it; the node still ends itself when the vanish movie would be over
		if no_2d and status == STATUS_VANISHED:
			var av := _anim(ANIM_VANISH)
			if KMissleResMath.special_frame(int(av.get("frames", 0)), int(av.get("dirs", 0)), int(av.get("interval", 1)), dir64, cur_life - _vanish_started) < 0:
				gone.emit(index)
				queue_free()
		return
	match status:
		STATUS_FLY:
			var a := _anim(ANIM_FLY)
			var frame := KMissleResMath.frame_index(int(a.get("frames", 0)), int(a.get("dirs", 0)), int(a.get("interval", 1)), dir64,
				cur_life - start_life_time, life_time - start_life_time, bool(res.get("loop", false)), bool(res.get("sub_loop", false)),
				int(res.get("sub_start", 0)), int(res.get("sub_stop", 0)))
			if not _show_frame(a, frame):
				_sprite.visible = false
		STATUS_VANISHED:
			var a := _anim(ANIM_VANISH)
			var elapsed := cur_life - _vanish_started
			var frame := KMissleResMath.special_frame(int(a.get("frames", 0)), int(a.get("dirs", 0)), int(a.get("interval", 1)), dir64, elapsed)
			if frame < 0 or not _show_frame(a, frame):
				_sprite.visible = false
				gone.emit(index)
				queue_free()
		_:
			_sprite.visible = false
