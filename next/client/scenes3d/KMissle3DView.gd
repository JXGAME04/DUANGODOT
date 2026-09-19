# The 3D picture of a missile (KMissleRes's part in 3D): follows a KMissle state node (scene_pos, z = height >> 10,
# status wait / fly / vanished - the 2.0 client's KMissle::OnFly rules stay in KMissle.gd) and shows it in the world.
# Until the skill effects of M3D-3 are mapped (skill_id -> a 3D effect, docs/LO-TRINH-3D.md 3.1) every missile is a
# glowing ball with a short trail [tự chọn], and the vanish state a burst at the spot.
extends Node3D

const KScene3DMath := preload("res://scenes3d/KScene3DMath.gd")
const STATUS_FLY := 1
const STATUS_VANISHED := 2

var missle: Node = null
var place: Node3D = null
var _ball: MeshInstance3D
var _trail: CPUParticles3D
var _burst: CPUParticles3D = null
var _ground := 0.0
var _last := Vector2(INF, INF)


func bind(state: Node, place3d: Node3D) -> void:
	missle = state
	place = place3d
	_ball = MeshInstance3D.new()
	var sph := SphereMesh.new()
	sph.radius = 0.18
	sph.height = 0.36
	_ball.mesh = sph
	var m := StandardMaterial3D.new()
	m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	m.albedo_color = Color(1.0, 0.8, 0.35)
	m.emission_enabled = true
	m.emission = Color(1.0, 0.6, 0.2)
	m.emission_energy_multiplier = 2.0
	_ball.material_override = m
	_ball.visible = false
	add_child(_ball)
	_trail = CPUParticles3D.new()
	_trail.amount = 24
	_trail.lifetime = 0.35
	_trail.emitting = false
	_trail.local_coords = false
	_trail.direction = Vector3.ZERO
	_trail.spread = 180.0
	_trail.initial_velocity_min = 0.2
	_trail.initial_velocity_max = 0.6
	_trail.gravity = Vector3.ZERO
	_trail.scale_amount_min = 0.05
	_trail.scale_amount_max = 0.12
	_trail.color = Color(1.0, 0.7, 0.3, 0.8)
	var pm := SphereMesh.new()
	pm.radius = 0.5
	pm.height = 1.0
	_trail.mesh = pm
	var tm := StandardMaterial3D.new()
	tm.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	tm.blend_mode = BaseMaterial3D.BLEND_MODE_ADD
	tm.vertex_color_use_as_albedo = true
	tm.albedo_color = Color(1, 0.75, 0.35)
	pm.material = tm
	add_child(_trail)
	_update()


func _update() -> void:
	var sp: Vector2 = missle.scene_pos
	if sp != _last:
		if _last.x == INF or sp.distance_to(_last) > 24.0:
			var w: Vector3 = place.to_world(sp)
			_ground = place.ground_height(w.x, w.z)
		_last = sp
	var world: Vector3 = place.to_world(sp)
	# the 2.0 client lifts the frame by z screen pixels (m_nCurrentMapZ): metres through the 30 degree rule, plus the
	# height a flying thing has anyway (a missile at z 0 flies at chest height in the 2D pictures) [tự chọn: 0.9 m]
	world.y = _ground + 0.9 + KScene3DMath.px_height_to_m(float(missle.z))
	global_position = world
	var flying: bool = int(missle.status) == STATUS_FLY
	_ball.visible = flying
	_trail.emitting = flying
	if int(missle.status) == STATUS_VANISHED and _burst == null:
		_burst = _make_burst(Color(1.0, 0.6, 0.25))
		add_child(_burst)
		_burst.emitting = true


static func _make_burst(color: Color) -> CPUParticles3D:
	var p := CPUParticles3D.new()
	p.amount = 40
	p.lifetime = 0.45
	p.one_shot = true
	p.explosiveness = 0.95
	p.direction = Vector3.UP
	p.spread = 180.0
	p.initial_velocity_min = 1.5
	p.initial_velocity_max = 3.5
	p.gravity = Vector3(0, -4, 0)
	p.scale_amount_min = 0.06
	p.scale_amount_max = 0.16
	p.color = color
	var pm := SphereMesh.new()
	pm.radius = 0.5
	pm.height = 1.0
	var tm := StandardMaterial3D.new()
	tm.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	tm.blend_mode = BaseMaterial3D.BLEND_MODE_ADD
	tm.vertex_color_use_as_albedo = true
	tm.albedo_color = color
	pm.material = tm
	p.mesh = pm
	return p


func _process(_delta: float) -> void:
	if missle == null or not is_instance_valid(missle):
		# the state node ended (the vanish movie is over): let the burst finish, then go
		if _burst == null or not _burst.emitting:
			queue_free()
		_ball.visible = false
		_trail.emitting = false
		return
	_update()
