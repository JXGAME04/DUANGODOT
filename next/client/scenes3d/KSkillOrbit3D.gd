# KSkillOrbit3D - a child object that circles its initial-position object (skill_childobj col 17 = 7 "绕初始位置目标旋转"
# [TK ChildObject.UpdateMove 0x4e0b00, CalcDeltaAngle 0x4e0590]; Ngũ Độc's 百毒穿心, JX skill 384).  Each frame the centre
# is the initial-position object's current position (the creator: here the zone's missile view this node hangs under,
# form 7 CastZone at the launcher [Linux KMissle]), the born angle grows by the angular speed x dt - the speed column
# "end * start * radius0 * frames": lerp(start, end, t) with t = time / (frames at 30 Hz) clamped, only a positive speed
# moves - the radius lerps from radius0 to the born distance (the reference's Atb_Skill_Distance x 0.01 from its skill
# script; here the JX row's AttackRadius x UNIT), a negative radius0 = a random radius between |radius0| and the born
# distance drawn once; the object faces its move direction (set_forward of the tangent) when it moved more than 1 cm.
# MathEx.DegreeToDirection(d) = Unity (sin d, 0, cos d) -> Godot (-sin d, 0, cos d) after the mirror; the born angle
# starts at 0 (OnDataReset) and the height offset of the row is added on top of the centre (set_position 0x4e28e0).
extends Node3D

const TICK_HZ := 30.0   # Global.cbLogicTick of the reference: the frames of the speed column

var speed_start := 0.0
var speed_end := 0.0
var frames := 0.0
var radius0 := 0.0
var born_distance := 0.0
var height := 0.0
var _t := 0.0
var _angle := 0.0
var _radius_fixed := -1.0   # the random radius once drawn (mBornDistance turned negative)
var _last := Vector3.INF


func setup(o: Dictionary, end_radius: float, h: float) -> void:
	speed_end = float(o.get("end", 0.0))
	speed_start = float(o.get("start", 0.0))
	radius0 = float(o.get("radius0", 0.0))
	frames = float(o.get("frames", 0.0))
	born_distance = maxf(0.0, end_radius)
	height = h
	top_level = true


func _process(delta: float) -> void:
	var parent := get_parent() as Node3D
	if parent == null:
		return
	var frac := 1.0 if frames <= 0.0 else clampf(_t / (frames / TICK_HZ), 0.0, 1.0)
	var w := lerpf(speed_start, speed_end, frac) if frames > 0.0 else speed_end
	var d_angle := w * delta if w > 0.0 else 0.0
	_angle += d_angle
	var r: float
	if radius0 >= 0.0:
		r = lerpf(radius0, born_distance, frac)
	else:
		if _radius_fixed < 0.0:
			_radius_fixed = -radius0 + (born_distance + radius0) * float(randi() % 100) * 0.01
		r = _radius_fixed
	_t += delta
	var a := deg_to_rad(_angle)
	var centre := parent.global_position
	var p := centre + Vector3(-sin(a), height, cos(a)) * Vector3(r, 1.0, r)
	if d_angle > 0.0 and _last != Vector3.INF:
		var dir := p - _last
		dir.y = 0.0
		if dir.length_squared() > 1e-4:
			global_rotation = Vector3(0.0, atan2(dir.x, dir.z), 0.0)
	global_position = p
	_last = p
