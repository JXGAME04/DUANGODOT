# Scn3DPlayer - nhan vat tam (hinh tru) de di thu tren map 3D: chuot trai = di toi diem bam, WASD = di theo camera.
# Cao do lay bang raycast xuong dia hinh (layer 1). Hinh nhan vat/NPC that lam o buoc sau.
extends Node3D
class_name Scn3DPlayer

var speed := 6.0
var move_target: Vector3
var moving := false
var cam_rig: Node3D  # Scn3DCamera
var yaw := 0.0

var _body: MeshInstance3D
var _nose: MeshInstance3D
var model: Node3D   # Scn3DNpc (neu co) thay cho hinh tru
var _was_moving := false


func _ready() -> void:
	_body = MeshInstance3D.new()
	var cap := CapsuleMesh.new()
	cap.radius = 0.35
	cap.height = 1.7
	_body.mesh = cap
	_body.position = Vector3(0, 0.85, 0)
	var m := StandardMaterial3D.new()
	m.albedo_color = Color(0.9, 0.3, 0.2)
	_body.material_override = m
	add_child(_body)
	_nose = MeshInstance3D.new()
	var box := BoxMesh.new()
	box.size = Vector3(0.2, 0.2, 0.5)
	_nose.mesh = box
	_nose.position = Vector3(0, 1.2, -0.5)
	var m2 := StandardMaterial3D.new()
	m2.albedo_color = Color(1, 1, 0.2)
	_nose.material_override = m2
	add_child(_nose)


# Thay hinh tru bang nhan vat 3D (Scn3DNpc da setup). Con Model cua no da quay 180 do nen huong -Z nhu hinh tru.
func set_model(npc: Node3D) -> void:
	if model:
		model.queue_free()
	model = npc
	add_child(npc)
	_body.visible = false
	_nose.visible = false
	if npc.has_method("idle"):
		npc.idle()


func snap_to_ground() -> void:
	var space := get_world_3d().direct_space_state
	var from := global_position + Vector3(0, 50, 0)
	var q := PhysicsRayQueryParameters3D.create(from, global_position + Vector3(0, -50, 0), 1)
	var hit := space.intersect_ray(q)
	if hit:
		global_position.y = hit.position.y


func go_to(p: Vector3) -> void:
	move_target = p
	moving = true


func _process(delta: float) -> void:
	var dir := Vector3.ZERO
	if model and model.get("busy"):
		moving = false
		snap_to_ground()
		return
	if cam_rig:
		if Input.is_key_pressed(KEY_W):
			dir += cam_rig.forward_flat()
		if Input.is_key_pressed(KEY_S):
			dir -= cam_rig.forward_flat()
		if Input.is_key_pressed(KEY_A):
			dir -= cam_rig.right_flat()
		if Input.is_key_pressed(KEY_D):
			dir += cam_rig.right_flat()
	if dir.length() > 0.01:
		moving = false
		dir = dir.normalized()
		global_position += dir * speed * delta
		yaw = atan2(-dir.x, -dir.z)
	elif moving:
		var to := move_target - global_position
		to.y = 0.0
		var step := speed * delta
		if to.length() <= step:
			global_position = Vector3(move_target.x, global_position.y, move_target.z)
			moving = false
		else:
			var d := to.normalized()
			global_position += d * step
			yaw = atan2(-d.x, -d.z)
	rotation.y = yaw
	snap_to_ground()
	var is_moving := moving or dir.length() > 0.01
	if model and model.has_method("walk"):
		if is_moving:
			model.walk()
		elif is_moving != _was_moving or not model.busy:
			model.idle()
	_was_moving = is_moving


func _unhandled_input(ev: InputEvent) -> void:
	if ev is InputEventMouseButton and ev.pressed and (ev as InputEventMouseButton).button_index == MOUSE_BUTTON_LEFT and cam_rig:
		var cam: Camera3D = cam_rig.cam
		var mpos := get_viewport().get_mouse_position()
		var from := cam.project_ray_origin(mpos)
		var to := from + cam.project_ray_normal(mpos) * 500.0
		var q := PhysicsRayQueryParameters3D.create(from, to, 1)
		var hit := get_world_3d().direct_space_state.intersect_ray(q)
		if hit:
			go_to(hit.position)
