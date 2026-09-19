# The orbit camera of the 3D world (KWorldView3D): SpringArm3D + Camera3D around a point over the character.
# Limits per map from the reference client's scn_list.cameraInit "dist*min*max*yaw*pitch*pitch_min*pitch_max"
# (Ba Lăng: 19*10*21*0*40*40*80) [TK].  Right drag turns it (the way the reference client does); a right *click*
# without a drag is left to the game screen (the right mouse skill of the 2.0 client) through `right_click`.
# Wheel zoom comes from KWorldView.zoom_step.  Drag speed and the eased follow are ours [tự chọn].
extends Node3D

signal right_click(screen: Vector2)

const DRAG_PX := 4.0           # a right press that moves less than this is a click

var dist := 19.0
var dist_min := 10.0
var dist_max := 21.0
var yaw := 0.0                 # degrees around Y
var pitch := 40.0              # degrees, looking down
var pitch_min := 40.0
var pitch_max := 80.0
var target_height := 1.4       # metres over the character's feet the camera looks at
var rotate_speed := 0.25       # degrees per pixel
var zoom_metres := 1.0         # per wheel step
var follow: Node3D = null      # the node the camera orbits (the character's 3D view)
# the classic mode of a 2.5D map: the 2.0 view - orthographic, raised 30 degrees (sy = y/2 - z*887/1024 [2.0]); the size
# 14.4 m tall keeps 1 scene unit = 1 screen pixel at 1280 x 720 (720 px of screen = 1440 units of depth = 28.8 m x sin 30)
var classic := false
const CLASSIC_SIZE := 14.4
const CLASSIC_YAW := 25.0       # the boards of a 2.5D map look like boards past this turn [tự chọn]
var yaw_min := -360.0            # a limited turn (classic mode); a full circle when min <= -360
var yaw_max := 360.0
var ortho_size := CLASSIC_SIZE
var focus := Vector3.ZERO      # where it looks when there is no follow node

var arm: SpringArm3D
var cam: Camera3D
var _right_down := false
var _right_start := Vector2.ZERO
var _dragged := false


func setup(p: Dictionary, fov: float = 40.0) -> void:
	classic = str(p.get("mode", "")) == "classic"
	if classic:
		dist = 30.0
		dist_min = 30.0
		dist_max = 30.0
		pitch = 30.0
		pitch_min = 30.0
		pitch_max = 30.0
		yaw = float(p.get("yaw", 0.0))
		yaw_min = -CLASSIC_YAW
		yaw_max = CLASSIC_YAW
		ortho_size = CLASSIC_SIZE
		if cam:
			cam.projection = Camera3D.PROJECTION_ORTHOGONAL
			cam.size = ortho_size
			cam.keep_aspect = Camera3D.KEEP_HEIGHT
		return
	if cam:
		cam.projection = Camera3D.PROJECTION_PERSPECTIVE
	yaw_min = -360.0
	yaw_max = 360.0
	dist = float(p.get("dist", dist))
	dist_min = float(p.get("dist_min", dist_min))
	dist_max = float(p.get("dist_max", dist_max))
	yaw = float(p.get("yaw", yaw))
	pitch = float(p.get("pitch", pitch))
	pitch_min = float(p.get("pitch_min", pitch_min))
	pitch_max = float(p.get("pitch_max", pitch_max))
	dist = clampf(dist, dist_min, dist_max)
	pitch = clampf(pitch, pitch_min, pitch_max)
	if cam:
		cam.fov = fov


# the far plane of the scene's own camera (the reference's far_clip_plane), else the quality setting stays
func set_far(f: float) -> void:
	if cam and f > 0.0:
		cam.far = f


func _ready() -> void:
	arm = SpringArm3D.new()
	arm.name = "Arm"
	arm.spring_length = dist
	arm.margin = 0.3
	arm.collision_mask = 1     # the terrain only
	add_child(arm)
	cam = Camera3D.new()
	cam.name = "Camera"
	cam.fov = 40.0
	cam.near = 0.1
	cam.far = float(preload("res://scenes3d/KScenePlace3D.gd").quality_settings()["far"])
	arm.add_child(cam)
	cam.make_current()
	_apply(true)


func _apply(snap: bool) -> void:
	var want := focus
	if follow != null and is_instance_valid(follow):
		want = follow.global_position + Vector3(0, target_height, 0)
	global_position = want if snap else global_position.lerp(want, 0.35)
	rotation_degrees = Vector3(-pitch, yaw, 0)
	arm.spring_length = dist


func snap_to(world: Vector3) -> void:
	focus = world
	_apply(true)


func _process(_delta: float) -> void:
	_apply(false)


func zoom_step(steps: int) -> void:
	if classic:
		ortho_size = clampf(ortho_size / pow(1.15, float(steps)), CLASSIC_SIZE * 0.5, CLASSIC_SIZE * 2.0)
		if cam:
			cam.size = ortho_size
		return
	dist = clampf(dist - float(steps) * zoom_metres, dist_min, dist_max)


func _unhandled_input(ev: InputEvent) -> void:
	if ev is InputEventMouseButton:
		var mb := ev as InputEventMouseButton
		if mb.button_index == MOUSE_BUTTON_RIGHT:
			if mb.pressed:
				_right_down = true
				_dragged = false
				_right_start = mb.position
			else:
				var was_drag := _dragged
				_right_down = false
				_dragged = false
				if not was_drag:
					right_click.emit(mb.position)
			get_viewport().set_input_as_handled()
	elif ev is InputEventMouseMotion and _right_down:
		var mm := ev as InputEventMouseMotion
		if not _dragged and mm.position.distance_to(_right_start) < DRAG_PX:
			return
		_dragged = true
		yaw = fmod(yaw - mm.relative.x * rotate_speed, 360.0)
		if yaw_min > -360.0:
			yaw = clampf(wrapf(yaw, -180.0, 180.0), yaw_min, yaw_max)
		pitch = clampf(pitch + mm.relative.y * rotate_speed, pitch_min, pitch_max)
		get_viewport().set_input_as_handled()


func state_text() -> String:
	if classic:
		return "classic yaw %.0f size %.1f at %s" % [yaw, ortho_size, str(global_position.snapped(Vector3(0.1, 0.1, 0.1)))]
	return "yaw %.0f pitch %.0f dist %.1f at %s" % [yaw, pitch, dist, str(global_position.snapped(Vector3(0.1, 0.1, 0.1)))]
