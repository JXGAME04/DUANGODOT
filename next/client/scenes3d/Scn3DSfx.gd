# Scn3DSfx - dung lai mot hieu ung ky nang cua bo tham khao tu assets3d/sfx/<ten>.json + .gltf
# (tools/scn3d/export_sfx.py): cay node glTF (mesh + animation bien doi), moi ParticleSystem -> CPUParticles3D,
# MeshRenderer -> vat lieu unshaded (cong/alpha, atlas), Light -> OmniLight3D. Tu huy khi het thoi gian.
extends Node3D

static var _cache := {}   # duong dan gltf -> [GLTFDocument, GLTFState, json]
static var _tex_cache := {}

var life := 0.0          # giay; <= 0: tu tinh
var looping := false
var _t := 0.0
var _tweens: Array = []
var _players: Array = []
var _particles: Array = []


static func load_desc(dir: String, name: String) -> Array:
	var key := dir + "/" + name
	if _cache.has(key):
		return _cache[key]
	var txt := FileAccess.get_file_as_string(key + ".json")
	if txt == "":
		return []
	var desc = JSON.parse_string(txt)
	var doc := GLTFDocument.new()
	var st := GLTFState.new()
	if doc.append_from_file(key + ".gltf", st) != OK:
		return []
	var pair := [doc, st, desc]
	_cache[key] = pair
	return pair


static func _tex(dir: String, file) -> Texture2D:
	if file == null or str(file) == "":
		return null
	var p := dir + "/" + str(file)
	if _tex_cache.has(p):
		return _tex_cache[p]
	var img := Image.load_from_file(p)
	var t: Texture2D = null
	if img != null:
		img.generate_mipmaps()
		t = ImageTexture.create_from_image(img)
	_tex_cache[p] = t
	return t


static func _material(dir: String, m, extra_color := Color.WHITE, billboard := false, tiles := Vector2i(1, 1), vertex_color := true) -> StandardMaterial3D:
	var mat := StandardMaterial3D.new()
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mat.cull_mode = BaseMaterial3D.CULL_DISABLED
	mat.disable_receive_shadows = true
	mat.depth_draw_mode = BaseMaterial3D.DEPTH_DRAW_DISABLED
	mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	mat.vertex_color_use_as_albedo = vertex_color
	var blend := "alpha"
	var tint := Color.WHITE
	if m is Dictionary:
		blend = str(m.get("blend", "alpha"))
		var t = m.get("tint", null)
		if t is Array and t.size() >= 4:
			tint = Color(t[0], t[1], t[2], t[3])
		mat.albedo_texture = _tex(dir, m.get("tex", null))
		var st = m.get("st", null)
		if st is Array and st.size() >= 4:
			mat.uv1_scale = Vector3(st[0], st[1], 1)
			mat.uv1_offset = Vector3(st[2], st[3], 0)
	match blend:
		"add":
			mat.blend_mode = BaseMaterial3D.BLEND_MODE_ADD
		"mul":
			mat.blend_mode = BaseMaterial3D.BLEND_MODE_MUL
		_:
			mat.blend_mode = BaseMaterial3D.BLEND_MODE_MIX
	mat.albedo_color = tint * extra_color
	if billboard:
		mat.billboard_mode = BaseMaterial3D.BILLBOARD_PARTICLES
		mat.billboard_keep_scale = true
	if tiles.x > 1 or tiles.y > 1:
		mat.particles_anim_h_frames = tiles.x
		mat.particles_anim_v_frames = tiles.y
		mat.particles_anim_loop = false
		if not billboard:
			mat.billboard_mode = BaseMaterial3D.BILLBOARD_PARTICLES
	return mat


static func _curve(keys) -> Curve:
	if keys == null or not (keys is Array) or keys.is_empty():
		return null
	var c := Curve.new()
	var vals: Array = []
	for k in keys:
		vals.append(float(k[1]))
	var mx: float = 1.0
	for v in vals:
		mx = maxf(mx, v)
	c.min_value = 0.0
	c.max_value = mx
	for k in keys:
		c.add_point(Vector2(clampf(float(k[0]), 0.0, 1.0), float(k[1])))
	return c


static func _gradient(keys) -> Gradient:
	if keys == null or not (keys is Array) or keys.is_empty():
		return null
	var g := Gradient.new()
	var offsets := PackedFloat32Array()
	var colors := PackedColorArray()
	for k in keys:
		offsets.append(clampf(float(k[0]), 0.0, 1.0))
		colors.append(Color(k[1], k[2], k[3], k[4]))
	g.offsets = offsets
	g.colors = colors
	return g


# dir: thu muc assets3d/sfx ; name: ten hieu ung (vd Skill_wd_nuleizhi) ; scale_all: ti le chung
func build(dir: String, name: String, scale_all := 1.0) -> bool:
	var pair := load_desc(dir, name)
	if pair.is_empty():
		push_error("Scn3DSfx: khong co hieu ung %s" % name)
		return false
	var doc: GLTFDocument = pair[0]
	var st: GLTFState = pair[1]
	var desc: Dictionary = pair[2]
	var root: Node = doc.generate_scene(st, 30.0, false, false)
	if root == null:
		return false
	root.name = "Fx"
	add_child(root)
	if scale_all != 1.0:
		root.scale = Vector3.ONE * scale_all
	var longest := 0.0
	# animation bien doi
	var ap := _find(root, "AnimationPlayer") as AnimationPlayer
	if ap:
		var names := ap.get_animation_list()
		if names.size() > 0:
			var a := ap.get_animation(names[0])
			a.loop_mode = Animation.LOOP_LINEAR if looping else Animation.LOOP_NONE
			ap.play(names[0])
			longest = maxf(longest, a.length)
			_players.append(ap)
	# node theo chi so: export_sfx.py ghi JSON nodes va glTF nodes cung thu tu (duyet truoc), ten co the trung
	# (prefab shifa_tuxi co "shang" la particle o tang 1 va "shang" la mesh o tang 2) nen khong tim theo ten
	var jnodes: Array = desc.get("nodes", [])
	var paths := _child_paths(jnodes)   # per JSON node: the child indices from glTF node 0 (the hierarchy, not the names)
	for i in jnodes.size():
		var jn: Dictionary = jnodes[i]
		var n: Node = _node_at(root, paths[i])
		if n == null:
			continue
		if not bool(jn.get("active", true)):
			if n is Node3D:
				(n as Node3D).visible = false
			continue
		if jn.has("mesh_material"):
			var mi := (n as MeshInstance3D) if n is MeshInstance3D else (_find(n, "MeshInstance3D") as MeshInstance3D)
			if mi:
				var extra := Color.WHITE
				var sfx = jn.get("mesh_sfx", null)
				if sfx is Dictionary:
					# SFXMeshModify.Update [TK 0x6fdb70]: vertex colour = color * adjustColor (1) + emissive, per channel; the
					# reference renders in linear space, so a gain v > 1 there is the same gain v in our sRGB pipeline (v^2.2 in,
					# ^(1/2.2) out) - the sum is used as it is
					var c = sfx.get("color", [1, 1, 1, 1])
					var e = sfx.get("emissive", [0, 0, 0, 0])
					extra = Color(c[0] + e[0], c[1] + e[1], c[2] + e[2], clampf(c[3] + e[3], 0.0, 1.0))
				var mat := _material(dir, jn["mesh_material"], extra, false, Vector2i(1, 1), true)
				for s in mi.mesh.get_surface_count():
					mi.set_surface_override_material(s, mat)
				mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		if jn.has("ps"):
			var t := _make_particles(dir, jn["ps"], st)
			if t:
				(n as Node3D).add_child(t)
				_particles.append(t)
				var ps: Dictionary = jn["ps"]
				longest = maxf(longest, float(ps.get("duration", 1.0)) + float(ps["lifetime"].get("max", 1.0)) + float(ps.get("start_delay", 0.0)))
		if jn.has("tweens") and n is Node3D:
			for tw in jn["tweens"]:
				_start_tween(n as Node3D, tw)
		if jn.has("light"):
			var l: Dictionary = jn["light"]
			var ol := OmniLight3D.new()
			var c = l.get("color", [1, 1, 1])
			ol.light_color = Color(c[0], c[1], c[2])
			ol.light_energy = clampf(float(l.get("intensity", 1.0)), 0.0, 4.0)
			ol.omni_range = clampf(float(l.get("range", 6.0)), 0.5, 20.0)
			ol.shadow_enabled = false
			(n as Node3D).add_child(ol)
	if life <= 0.0:
		life = maxf(0.6, longest) if not looping else 0.0
	return true


# The JSON nodes are the glTF nodes in pre-order with their depth: the child index path of each one from node 0.
static func _child_paths(jnodes: Array) -> Array:
	var paths: Array = []
	var stack: Array = []          # [[depth, path, child_count]] of the open ancestors
	for jn in jnodes:
		var depth := int(jn.get("depth", 0))
		while not stack.is_empty() and int(stack.back()[0]) >= depth:
			stack.pop_back()
		var path: Array = []
		if not stack.is_empty():
			var parent: Array = stack.back()
			path = (parent[1] as Array).duplicate()
			path.append(int(parent[2]))
			parent[2] = int(parent[2]) + 1
		paths.append(path)
		stack.append([depth, path, 0])
	return paths


# glTF node 0 is the first child of the generated root; the others follow the child indices (Godot keeps the order)
static func _node_at(root: Node, path: Array) -> Node:
	if root.get_child_count() == 0:
		return null
	var n: Node = root.get_child(0)
	for idx in path:
		if int(idx) >= n.get_child_count():
			return null
		n = n.get_child(int(idx))
	return n


func _make_particles(dir: String, ps: Dictionary, st: GLTFState) -> CPUParticles3D:
	var p := CPUParticles3D.new()
	var life_: Dictionary = ps.get("lifetime", {"min": 1.0, "max": 1.0})
	var lmax: float = maxf(0.05, float(life_.get("max", 1.0)))
	var lmin: float = clampf(float(life_.get("min", lmax)), 0.05, lmax)
	p.lifetime = lmax
	p.lifetime_randomness = clampf(1.0 - lmin / lmax, 0.0, 1.0)
	p.one_shot = not bool(ps.get("looping", false))
	var rate: Dictionary = ps.get("rate", {"max": 0})
	var bursts: Array = ps.get("bursts", [])
	var burst_count := 0.0
	for b in bursts:
		burst_count += float(b.get("count", 0)) * maxf(1.0, float(b.get("cycles", 1)))
	var duration: float = maxf(0.1, float(ps.get("duration", 1.0)))
	var amount := int(ceil(float(rate.get("max", 0)) * (lmax if p.one_shot else duration) + burst_count))
	amount = clampi(amount, 1, int(ps.get("max_particles", 200)))
	p.amount = amount
	if float(rate.get("max", 0)) <= 0.0 and burst_count > 0:
		p.explosiveness = 1.0
	elif burst_count > 0:
		p.explosiveness = 0.6
	p.speed_scale = float(ps.get("sim_speed", 1.0))
	p.local_coords = not bool(ps.get("world_space", false))
	p.preprocess = 0.0
	# hinh dang phat
	var sh: Dictionary = ps.get("shape", {})
	var stype := int(sh.get("type", 0))
	var radius: float = float(sh.get("radius", 0.0))
	p.direction = Vector3(0, 0, 1)
	p.spread = 180.0
	match stype:
		0, 1, 2, 3:
			p.emission_shape = CPUParticles3D.EMISSION_SHAPE_SPHERE
			p.emission_sphere_radius = maxf(0.01, radius)
			p.spread = 180.0
		4, 7, 8:
			p.emission_shape = CPUParticles3D.EMISSION_SHAPE_SPHERE
			p.emission_sphere_radius = maxf(0.01, radius)
			p.direction = Vector3(0, 0, 1)
			p.spread = clampf(float(sh.get("angle", 25.0)), 0.0, 90.0)
		5, 15, 16:
			p.emission_shape = CPUParticles3D.EMISSION_SHAPE_BOX
			var sc = sh.get("scale", [1, 1, 1])
			p.emission_box_extents = Vector3(absf(sc[0]) * 0.5, absf(sc[1]) * 0.5, absf(sc[2]) * 0.5)
		10, 11:
			p.emission_shape = CPUParticles3D.EMISSION_SHAPE_SPHERE
			p.emission_sphere_radius = maxf(0.01, radius)
			p.spread = 180.0
		_:
			p.emission_shape = CPUParticles3D.EMISSION_SHAPE_POINT
	if not bool(sh.get("enabled", true)):
		p.emission_shape = CPUParticles3D.EMISSION_SHAPE_POINT
	var spos = sh.get("pos", [0, 0, 0])
	p.position = Vector3(spos[0], spos[1], spos[2])
	var srot = sh.get("rot", [0, 0, 0])
	p.rotation_degrees = Vector3(srot[0], -srot[1], -srot[2])
	# van toc, trong luc
	var spd: Dictionary = ps.get("speed", {"min": 0, "max": 0})
	p.initial_velocity_min = float(spd.get("min", 0))
	p.initial_velocity_max = float(spd.get("max", 0))
	var vel = ps.get("velocity", null)
	if vel is Dictionary:
		p.linear_accel_min = 0.0
		p.direction = (p.direction + Vector3(-float(vel.get("x", 0)), float(vel.get("y", 0)), float(vel.get("z", 0)))).normalized() if p.initial_velocity_max > 0.0 else Vector3(-float(vel.get("x", 0)), float(vel.get("y", 0)), float(vel.get("z", 0)))
		if p.initial_velocity_max <= 0.0:
			var vl: float = p.direction.length()
			if vl > 0.001:
				p.direction = p.direction / vl
				p.initial_velocity_min = vl
				p.initial_velocity_max = vl
				p.spread = 0.0
	var grav: Dictionary = ps.get("gravity", {"max": 0})
	p.gravity = Vector3(0, -9.81 * float(grav.get("max", 0)), 0)
	# kich thuoc
	var sz: Dictionary = ps.get("size", {"min": 1, "max": 1})
	p.scale_amount_min = float(sz.get("min", 1))
	p.scale_amount_max = float(sz.get("max", 1))
	var sc_curve := _curve(ps.get("size_over_life", null))
	if sc_curve:
		p.scale_amount_curve = sc_curve
	# xoay
	var rot: Dictionary = ps.get("rotation", {"min": 0, "max": 0})
	p.angle_min = rad_to_deg(float(rot.get("min", 0)))
	p.angle_max = rad_to_deg(float(rot.get("max", 0)))
	var rol = ps.get("rot_over_life", null)
	if rol is Dictionary:
		p.angular_velocity_min = rad_to_deg(float(rol.get("min", 0)))
		p.angular_velocity_max = rad_to_deg(float(rol.get("max", 0)))
	# mau
	var sc0 = ps.get("start_color", null)
	if sc0 is Array and sc0.size() > 0:
		var k = sc0[0]
		p.color = Color(k[1], k[2], k[3], k[4])
	var ramp := _gradient(ps.get("color_over_life", null))
	if ramp:
		p.color_ramp = ramp
	# ve
	var rd: Dictionary = ps.get("render", {})
	var mode := int(rd.get("mode", 0))
	var tiles := Vector2i(1, 1)
	var uv = ps.get("uv", null)
	if uv is Dictionary:
		tiles = Vector2i(maxi(1, int(uv.get("tiles_x", 1))), maxi(1, int(uv.get("tiles_y", 1))))
		var sf: Dictionary = uv.get("start_frame", {"min": 0, "max": 0})
		p.anim_offset_min = clampf(float(sf.get("min", 0)), 0.0, 1.0)
		p.anim_offset_max = clampf(float(sf.get("max", 0)), 0.0, 1.0)
		var fot: Dictionary = uv.get("frame_over_time", {"min": 0, "max": 0})
		var cycles: float = float(uv.get("cycles", 1.0))
		p.anim_speed_min = float(fot.get("max", 0)) * cycles
		p.anim_speed_max = float(fot.get("max", 0)) * cycles
	var mat := _material(dir, rd.get("material", null), Color.WHITE, mode != 4, tiles, true)
	if mode == 4 and rd.get("mesh_index", null) != null:
		var gm: Array = st.get_meshes()
		var idx := int(rd["mesh_index"])
		if idx >= 0 and idx < gm.size():
			var im: ImporterMesh = gm[idx].mesh
			var am: ArrayMesh = im.get_mesh()
			p.mesh = am
			mat.billboard_mode = BaseMaterial3D.BILLBOARD_DISABLED
	if p.mesh == null:
		var q := QuadMesh.new()
		q.size = Vector2(1, 1)
		p.mesh = q
		if mode == 2:   # nam ngang
			q.orientation = PlaneMesh.FACE_Y
			mat.billboard_mode = BaseMaterial3D.BILLBOARD_DISABLED
	p.mesh = p.mesh.duplicate()
	p.mesh.surface_set_material(0, mat)
	p.emitting = true
	return p


# The NGUI tweens the reference puts on effect nodes (TweenRotation / Scale / Position / Alpha / Color / Enhance [TK],
# export_sfx.py read_tween): from -> to over `duration` after `delay`, method = ease (Linear, EaseIn/Out/InOut, Bounce),
# style Once / Loop / PingPong.  Alpha / colour / enhance go to the mesh material of the node (unshaded albedo: enhance
# = brightness x (1 + e) [tự chọn, the reference shader's _Enhance]).
func _start_tween(n: Node3D, tw: Dictionary) -> void:
	var kind := str(tw.get("type", ""))
	var dur := maxf(float(tw.get("duration", 1.0)), 0.01)
	var delay := float(tw.get("delay", 0.0))
	var style := int(tw.get("style", 0))
	var method := int(tw.get("method", 0))
	var t := n.create_tween()
	match method:
		1: t.set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN)
		2: t.set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_OUT)
		3: t.set_trans(Tween.TRANS_QUAD).set_ease(Tween.EASE_IN_OUT)
		4: t.set_trans(Tween.TRANS_BOUNCE).set_ease(Tween.EASE_IN)
		5: t.set_trans(Tween.TRANS_BOUNCE).set_ease(Tween.EASE_OUT)
		_: t.set_trans(Tween.TRANS_LINEAR)
	if style == 1 or style == 2:
		t.set_loops()
	var target: Object = n
	var prop := ""
	var a: Variant = null
	var b: Variant = null
	match kind:
		"rotation":
			prop = "rotation_degrees"
			a = Vector3(tw["from"][0], tw["from"][1], tw["from"][2])
			b = Vector3(tw["to"][0], tw["to"][1], tw["to"][2])
		"scale":
			prop = "scale"
			a = Vector3(tw["from"][0], tw["from"][1], tw["from"][2])
			b = Vector3(tw["to"][0], tw["to"][1], tw["to"][2])
		"position":
			prop = "position"
			a = Vector3(tw["from"][0], tw["from"][1], tw["from"][2])
			b = Vector3(tw["to"][0], tw["to"][1], tw["to"][2])
		"alpha", "color", "enhance":
			var mi := (n as MeshInstance3D) if n is MeshInstance3D else (_find(n, "MeshInstance3D") as MeshInstance3D)
			var mat: Material = mi.get_surface_override_material(0) if mi != null and mi.mesh != null and mi.mesh.get_surface_count() > 0 else null
			if not (mat is StandardMaterial3D):
				t.kill()
				return
			target = mat
			var base: Color = (mat as StandardMaterial3D).albedo_color
			prop = "albedo_color"
			if kind == "alpha":
				a = Color(base.r, base.g, base.b, float(tw["from"]))
				b = Color(base.r, base.g, base.b, float(tw["to"]))
			elif kind == "color":
				a = Color(tw["from"][0], tw["from"][1], tw["from"][2], base.a if bool(tw.get("emissive", false)) else tw["from"][3])
				b = Color(tw["to"][0], tw["to"][1], tw["to"][2], base.a if bool(tw.get("emissive", false)) else tw["to"][3])
			else:
				a = Color(base.r * (1.0 + float(tw["from"])), base.g * (1.0 + float(tw["from"])), base.b * (1.0 + float(tw["from"])), base.a)
				b = Color(base.r * (1.0 + float(tw["to"])), base.g * (1.0 + float(tw["to"])), base.b * (1.0 + float(tw["to"])), base.a)
		_:
			t.kill()
			return
	if delay > 0.0:
		t.tween_interval(delay)
	t.tween_property(target, prop, b, dur).from(a)
	if style == 2:
		t.tween_property(target, prop, a, dur)
	_tweens.append(t)


func _process(delta: float) -> void:
	_t += delta
	if life > 0.0 and _t >= life:
		queue_free()


func _find(n: Node, cls: String) -> Node:
	if n.get_class() == cls:
		return n
	for c in n.get_children():
		var r := _find(c, cls)
		if r:
			return r
	return null


# tao nhanh: parent, hieu ung, vi tri (the gioi), goc quay Y (rad)
static func spawn(parent: Node, dir: String, name: String, pos: Vector3, yaw := 0.0, life_sec := 0.0, loop := false, scale_all := 1.0) -> Node3D:
	var fx: Node3D = Node3D.new()
	fx.set_script(load("res://scenes3d/Scn3DSfx.gd"))
	fx.name = "sfx_" + name
	fx.life = life_sec
	fx.looping = loop
	parent.add_child(fx)
	fx.global_position = pos
	fx.rotation.y = yaw
	if not fx.build(dir, name, scale_all):
		fx.queue_free()
		return null
	return fx
