# Scn3DCamera - camera quy dao quanh nhan vat, thao tac nhu game 3D tham khao (快捷键.txt):
# chuot phai keo = xoay (yaw/pitch), con lan hoac chuot giua keo = zoom. Thong so tu scn_list.cameraInit
# "khoang cach*min*max*gocX*gocY*minY*maxY" (vd 19*10*21*0*40*40*80).
extends Node3D
class_name Scn3DCamera

var dist := 19.0
var dist_min := 10.0
var dist_max := 21.0
var yaw := 0.0          # do, quanh truc Y
var pitch := 40.0       # do, nhin xuong
var pitch_min := 40.0
var pitch_max := 80.0
var target: Node3D
var target_height := 1.4
var rotate_speed := 0.25  # do / pixel
var zoom_step := 1.0

var arm: SpringArm3D
var cam: Camera3D
var _rotating := false
var _zooming := false


func setup(p: Dictionary, fov: float) -> void:
	dist = float(p.get("dist", dist))
	dist_min = float(p.get("dist_min", dist_min))
	dist_max = float(p.get("dist_max", dist_max))
	yaw = float(p.get("yaw", yaw))
	pitch = float(p.get("pitch", pitch))
	pitch_min = float(p.get("pitch_min", pitch_min))
	pitch_max = float(p.get("pitch_max", pitch_max))
	if cam:
		cam.fov = fov


func _ready() -> void:
	arm = SpringArm3D.new()
	arm.name = "Arm"
	arm.spring_length = dist
	arm.margin = 0.3
	arm.collision_mask = 1  # chi va cham voi dia hinh (layer 1)
	add_child(arm)
	cam = Camera3D.new()
	cam.name = "Camera"
	cam.fov = 40.0
	cam.near = 0.1
	cam.far = 400.0
	arm.add_child(cam)
	cam.make_current()
	_apply()


func _apply() -> void:
	var pos := Vector3.ZERO
	if target:
		pos = target.global_position + Vector3(0, target_height, 0)
	global_position = pos
	rotation_degrees = Vector3(-pitch, yaw, 0)
	arm.spring_length = dist


func _process(_delta: float) -> void:
	_apply()


func _unhandled_input(ev: InputEvent) -> void:
	if ev is InputEventMouseButton:
		var mb := ev as InputEventMouseButton
		if mb.button_index == MOUSE_BUTTON_RIGHT:
			_rotating = mb.pressed
		elif mb.button_index == MOUSE_BUTTON_MIDDLE:
			_zooming = mb.pressed
		elif mb.button_index == MOUSE_BUTTON_WHEEL_UP and mb.pressed:
			dist = clampf(dist - zoom_step, dist_min, dist_max)
		elif mb.button_index == MOUSE_BUTTON_WHEEL_DOWN and mb.pressed:
			dist = clampf(dist + zoom_step, dist_min, dist_max)
	elif ev is InputEventMouseMotion:
		var mm := ev as InputEventMouseMotion
		if _rotating:
			yaw = fmod(yaw - mm.relative.x * rotate_speed, 360.0)
			pitch = clampf(pitch + mm.relative.y * rotate_speed, pitch_min, pitch_max)
		elif _zooming:
			dist = clampf(dist + mm.relative.y * 0.05, dist_min, dist_max)
	elif ev is InputEventKey and ev.pressed:
		var k := ev as InputEventKey
		if k.keycode == KEY_Q:
			yaw = fmod(yaw + 15.0, 360.0)
		elif k.keycode == KEY_E:
			yaw = fmod(yaw - 15.0, 360.0)
		elif k.keycode == KEY_PAGEUP:
			pitch = clampf(pitch + 5.0, pitch_min, pitch_max)
		elif k.keycode == KEY_PAGEDOWN:
			pitch = clampf(pitch - 5.0, pitch_min, pitch_max)


# Huong "toi" tren mat dat theo camera (de di WASD).
func forward_flat() -> Vector3:
	var f := -cam.global_transform.basis.z
	f.y = 0.0
	return f.normalized() if f.length() > 0.001 else Vector3.FORWARD


func right_flat() -> Vector3:
	var r := cam.global_transform.basis.x
	r.y = 0.0
	return r.normalized()
