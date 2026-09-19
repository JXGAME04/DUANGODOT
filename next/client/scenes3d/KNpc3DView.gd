# The 3D picture of one entity (KNpcRes's part in 3D): a Node3D that follows the state of a KNpc / KObj node
# (scene_pos, res_dir, doing, frames, life...) the way KNpcRes follows it on the canvas.  The state machine stays
# in KNpc.gd (the 2.0 client's rules, docs/CLIENT-2.0.md); this node only shows it:
#   - position: scene units -> metres through the map (KScenePlace3D.to_world), the feet on the terrain;
#   - facing: yaw of res_dir (KNpc::Paint turns res_dir half way to dir every logic frame [2.0]); the yaw eases
#     between logic frames at the real frame rate (owner rule: movement interpolated to the frame rate);
#   - doing -> a clip of the model (Scn3DNpc, the reference client's anim_group names [TK]) with the length the
#     zone dictates (frames / 18 s), the death clip holds its last frame like KNpc::OnDeath;
#   - a marker capsule when the entity has no 3D model yet, a ring at the feet when it is the target.
extends Node3D

const SfxScript := preload("res://scenes3d/Scn3DSfx.gd")
const KScene3DMath := preload("res://scenes3d/KScene3DMath.gd")
const KNpcResNode := preload("res://scenes/KNpcResNode.gd")
const MirrorScript := preload("res://scenes3d/KSpriteMirror3D.gd")
const TICK := 1.0 / 18.0
const TURN_TAU := 0.055        # seconds: the yaw closes 63 % of the gap in one logic frame [tự chọn]
const ENTITY_PLAYER := 1
const ENTITY_MONSTER := 3
const ENTITY_DROP := 4

var npc: Node = null           # the KNpc / KObj state node (a Node2D with no_2d)
var place: Node3D = null       # KScenePlace3D
var model: Node3D = null       # Scn3DNpc, null for the marker
var radius := 0.5              # pick cylinder around the feet, metres (cp_radius [TK]; 0.5 when unknown)
var bar_height := 2.0          # the name over the feet, metres (sys_bar [TK] or GetNpcPate [2.0])
var is_own := false
var _last_scene := Vector2(INF, INF)
var _ground := 0.0
var _yaw := 0.0
var _marker: MeshInstance3D = null
var _ring: MeshInstance3D = null
var _doing := -1
var mirror: Node3D = null      # KSpriteMirror3D in the 2.5D world: the 2.0 sprites of the entity as a board
var horse: Node3D = null       # Scn3DNpc of the mount while riding (anim_group 21 [TK]); the rider sits on its ma_qi1
var horse_loader: Callable     # (npc) -> Scn3DNpc, set by the world view (the horse of the worn item, or the default)
var _rider_group := ""         # the rider's weapon group to go back to when dismounting


# Binds the state node; `model_node` is a set-up Scn3DNpc (or null for a marker), `model_info` its npc_models.json row.
func bind(state: Node, place3d: Node3D, model_node: Node3D, model_info: Dictionary) -> void:
	npc = state
	place = place3d
	is_own = _flag(state, "is_own")
	if model_node != null:
		model = model_node
		model.name = "Model"
		add_child(model)
		bar_height = float(model.get("bar_height")) if model.get("bar_height") != null else 2.0
		# cha_pic col 14/15 [TK]: the pick collider - a sphere (radius) or a box (x*y*z, the player 1 x 1.8 x 1, a pig
		# 0.8 x 1.4 x 1) -> the pick cylinder's radius; its height stays the name bar (sys_bar)
		var pick = model_info.get("pick", null)
		if pick is Dictionary and pick.get("size", []).size() >= 1:
			var sz: Array = pick["size"]
			if int(pick.get("type", 0)) == 1 and sz.size() >= 3:
				radius = maxf(0.25, maxf(float(sz[0]), float(sz[2])) / 2.0)
			else:
				radius = maxf(0.25, float(sz[0]))
		if not bool(place.quality_settings().get("shadows", true)):
			_add_blob_shadow()
	elif not _flag(state, "no_2d") and _flag(state, "has_res"):
		# the 2.5D world: the KNpcRes parts of the invisible canvas node, mirrored onto a board facing the camera
		mirror = Node3D.new()
		mirror.set_script(MirrorScript)
		mirror.name = "Mirror"
		add_child(mirror)
		mirror.bind(state)
		var pate_px: float = float(npc.get("stature")) if npc.get("stature") != null else 0.0
		if int(npc.get("entity_type")) == ENTITY_PLAYER:
			pate_px += 84.0
		bar_height = KScene3DMath.px_height_to_m(pate_px) if pate_px > 0.0 else 1.8
	else:
		if not _flag(state, "no_2d") and int(state.get("entity_type")) == ENTITY_DROP:
			# a thing on the ground in the 2.5D world: its own KObj picture
			mirror = Node3D.new()
			mirror.set_script(MirrorScript)
			mirror.name = "Mirror"
			add_child(mirror)
			mirror.bind(state)
		else:
			_make_marker()
		# GetNpcPate of the 2.0 client in metres: stature (+84 for players) screen px over the feet
		var pate: float = float(npc.get("stature")) if npc.get("stature") != null else 0.0
		if int(npc.get("entity_type")) == ENTITY_PLAYER:
			pate += 84.0
		bar_height = KScene3DMath.px_height_to_m(pate) if pate > 0.0 else 1.0
	if npc.has_signal("doing_changed"):
		npc.doing_changed.connect(_on_doing_changed)
	if npc.has_signal("riding_changed"):
		npc.riding_changed.connect(_on_riding_changed)
	if npc.get("dir64") != null:
		_yaw = KScene3DMath.yaw_of_dir(int(npc.res_dir))
		rotation.y = deg_to_rad(_yaw)
	_place(true)
	if model != null and bool(_flag(npc, "riding")):
		_on_riding_changed(true)
	if npc.get("doing") != null and npc.get("total_frame") != null:
		_on_doing_changed(int(npc.doing), int(npc.total_frame))
		# a late joiner sees a corpse at its last frame (KNpc.setup puts cur_frame at the end)
		if int(npc.doing) == KNpcResNode.Doing.DEATH and int(npc.cur_frame) >= int(npc.total_frame) - 1 and model != null:
			model.seek_end()


# a bool property of the state node, false when it has none (a KObj has no hovered / res_dir)
static func _flag(n: Node, prop: String) -> bool:
	var v = n.get(prop)
	return v != null and bool(v)


func _make_marker() -> void:
	var t := int(npc.get("entity_type"))
	if t == ENTITY_DROP and npc.get("obj") != null and str(npc.obj.get("image", "")) != "":
		# a thing on the ground: its ObjData picture (KObj::Draw [2.0]) as a billboard, 1 px = UNIT
		var atlas = Assets.sprite(str(npc.obj.get("image", "")))
		if atlas != null and atlas.frame_count() > 0:
			var spr := Sprite3D.new()
			spr.texture = atlas.frame_texture(0)
			spr.billboard = BaseMaterial3D.BILLBOARD_ENABLED
			spr.pixel_size = KScene3DMath.UNIT
			spr.centered = true
			spr.position = Vector3(0, float(spr.texture.get_height()) * KScene3DMath.UNIT * 0.5, 0)
			spr.texture_filter = BaseMaterial3D.TEXTURE_FILTER_NEAREST
			add_child(spr)
			radius = 0.3
			return
	_marker = MeshInstance3D.new()
	if t == ENTITY_DROP:
		var box := BoxMesh.new()
		box.size = Vector3(0.3, 0.3, 0.3)
		_marker.mesh = box
		_marker.position = Vector3(0, 0.15, 0)
	else:
		var cap := CapsuleMesh.new()
		cap.radius = 0.3
		cap.height = 1.7
		_marker.mesh = cap
		_marker.position = Vector3(0, 0.85, 0)
	var m := StandardMaterial3D.new()
	# the colours of KNpc._draw: me blue, other players green, monsters red, npcs orange, things yellow
	var color := Color(0.95, 0.6, 0.2)
	if t == ENTITY_PLAYER:
		color = Color(0.3, 0.9, 0.4)
	if is_own:
		color = Color(0.35, 0.65, 1.0)
	elif t == ENTITY_MONSTER:
		color = Color(0.95, 0.3, 0.3)
	elif t == ENTITY_DROP:
		color = Color(1.0, 0.85, 0.3)
	m.albedo_color = color
	_marker.material_override = m
	add_child(_marker)


# The selection ring of the reference (TargetSelectEffect [TK Update 0x698b70]: the prefab Cmn/cmn_select at the target's
# feet, its "enemy" branch when PKRule.IsEnemy, else "friend"; Cmn/cmn_select_temp flashes once when the target changes);
# a plain torus stands in when the prefab is not exported
var _select_fx: Node3D = null
var _select_enemy := false


func _make_ring() -> void:
	var fx: Node3D = SfxScript.spawn(self, "%s/sfx" % Assets.assets3d_root(), "Cmn_cmn_select", global_position + Vector3(0, 0.03, 0), 0.0, 0.0, true)
	if fx != null:
		fx.top_level = true
		fx.name = "Select"
		_select_fx = fx
		_ring = MeshInstance3D.new()   # the visibility switch of the old code path
		_ring.visible = false
		add_child(_ring)
		_select_enemy = npc.has_method("is_attackable") and bool(npc.call("is_attackable"))
		_apply_select_branch()
		var temp: Node3D = SfxScript.spawn(get_parent() if get_parent() != null else self, "%s/sfx" % Assets.assets3d_root(), "Cmn_cmn_select_temp", global_position + Vector3(0, 0.03, 0), 0.0, 1.2, false)
		if temp != null:
			for c in temp.find_children("*", "Node3D", true, false):
				(c as Node3D).visible = true
		return
	_ring = MeshInstance3D.new()
	var tor := TorusMesh.new()
	tor.inner_radius = maxf(radius, 0.4)
	tor.outer_radius = maxf(radius, 0.4) + 0.06
	tor.rings = 24
	tor.ring_segments = 6
	_ring.mesh = tor
	_ring.position = Vector3(0, 0.03, 0)
	var m := StandardMaterial3D.new()
	m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	m.albedo_color = Color(1.0, 0.9, 0.2, 0.85)
	m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	_ring.material_override = m
	_ring.top_level = true
	_ring.visible = false
	add_child(_ring)


func _apply_select_branch() -> void:
	if _select_fx == null or not is_instance_valid(_select_fx):
		return
	var en := _select_fx.find_child("enemy", true, false)
	var fr := _select_fx.find_child("friend", true, false)
	if en is Node3D:
		(en as Node3D).visible = _select_enemy
	if fr is Node3D:
		(fr as Node3D).visible = not _select_enemy


func _place(snap: bool) -> void:
	var sp: Vector2 = npc.scene_pos
	if sp != _last_scene:
		if snap or sp.distance_to(_last_scene) > 6.0:   # a new ground sample every ~12 cm of travel
			var w: Vector3 = place.to_world(sp)
			_ground = place.ground_height(w.x, w.z)
		_last_scene = sp
	var world: Vector3 = place.to_world(sp)
	world.y = _ground
	global_position = world


func _process(delta: float) -> void:
	if _ghost_left > 0.0:
		_ghost_left -= delta
		_ghost_timer -= delta
		if _ghost_timer <= 0.0:
			_ghost_timer = _ghost_interval
			_spawn_ghost()
	if npc == null or not is_instance_valid(npc):
		queue_free()
		return
	_place(false)
	if npc.get("res_dir") != null and mirror == null:
		var want := KScene3DMath.yaw_of_dir(int(npc.res_dir))
		var k := 1.0 - exp(-delta / TURN_TAU)
		_yaw = wrapf(_yaw + KScene3DMath.yaw_delta(_yaw, want) * k, -180.0, 180.0)
		rotation.y = deg_to_rad(_yaw)
	var target := _flag(npc, "is_target")
	if target and _ring == null:
		_make_ring()
	if _ring != null:
		var shown := target and not (npc.has_method("is_dead") and bool(npc.call("is_dead")))
		_ring.visible = shown
		_ring.global_position = global_position + Vector3(0, 0.03, 0)
		if _select_fx != null and is_instance_valid(_select_fx):
			_select_fx.visible = shown
			_select_fx.global_position = global_position + Vector3(0, 0.03, 0)
			var enemy := npc.has_method("is_attackable") and bool(npc.call("is_attackable"))
			if enemy != _select_enemy:
				_select_enemy = enemy
				_apply_select_branch()


# The reference draws characters' shadows with a projector (ShadowProjMgr / DynamicShadowProjector: the figure's own
# shape, the scene itself is lightmapped); without sun shadows (quality "low") a soft dark disc at the feet stands in
# [tự chọn: radius 0.7 m, alpha 0.6, 8 cm over the feet], one shared radial texture
static var _blob_tex: Texture2D = null


static func _blob_texture() -> Texture2D:
	if _blob_tex != null:
		return _blob_tex
	var n := 64
	var img := Image.create_empty(n, n, false, Image.FORMAT_RGBA8)
	for y in n:
		for x in n:
			var d := Vector2(x + 0.5 - n / 2.0, y + 0.5 - n / 2.0).length() / (n / 2.0)
			var a := clampf(1.0 - d, 0.0, 1.0)
			img.set_pixel(x, y, Color(0, 0, 0, a * a * 0.6))
	img.generate_mipmaps()   # the GL renderer samples nothing from a mipmap-filtered texture without mipmaps
	_blob_tex = ImageTexture.create_from_image(img)
	return _blob_tex


func _add_blob_shadow() -> void:
	var q := PlaneMesh.new()
	q.size = Vector2(1.4, 1.4)
	var mi := MeshInstance3D.new()
	mi.name = "Blob"
	mi.mesh = q
	var m := StandardMaterial3D.new()
	m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	m.albedo_texture = _blob_texture()
	m.cull_mode = BaseMaterial3D.CULL_DISABLED
	m.render_priority = -1
	mi.material_override = m
	mi.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	mi.position = Vector3(0, 0.08, 0)
	add_child(mi)


# Ghost [TK skill_event 111 -> TaskGhost.Start(crt, interval, duration, num, matUrl) 0x5268a0]: every `interval` s the
# model's skinned meshes are baked at the pose of the moment (SkinnedMeshRenderer.BakeMesh) into a static copy that lives
# `duration` s: MirageData.RenderTick 0x528860 sets the material's _AdjustA = 1 - t / duration and clears it at duration;
# at most `max` alive.  Without a material (every JX-mapped skill: 0.06 s / 0.38 s, two at 0.033 s) Init(null) clones the
# character's own materials (cha_base_rim: SrcAlpha OneMinusSrcAlpha, alpha = _AdjustA) - the copy fades out as it is;
# with an element material (3418skill_mid_add_ghost_<jin|mu|shui|huo|tu>: blend_dst_rimlight, additive) the copy is the
# texture x _AdjustC x _Enhance blended to _EdgeColor x _Enhance by the fresnel rim, alpha fading the same way.
const GHOST_MATS := {
	"jin": {"adjust": Color(0.377, 0.368, 0.0), "edge": Color(1.498, 1.365, 0.29), "rim": 8.0, "enhance": 1.0},
	"huo": {"adjust": Color(0.717, 0.526, 0.24), "edge": Color(1.498, 0.199, 0.092), "rim": 12.7, "enhance": 1.05},
	"tu": {"adjust": Color(0.547, 0.511, 0.359), "edge": Color(1.498, 1.151, 0.742), "rim": 8.0, "enhance": 0.59},
	"shui": {"adjust": Color(0.051, 0.446, 0.642), "edge": Color(0.0, 0.0, 1.498), "rim": 8.0, "enhance": 1.94},
	"mu": {"adjust": Color(0.024, 0.557, 0.464), "edge": Color(0.431, 1.498, 0.465), "rim": 12.4, "enhance": 1.0}}
var _ghost_left := 0.0
var _ghost_interval := 0.06
var _ghost_duration := 0.38
var _ghost_max := 999
var _ghost_mat := ""
var _ghost_timer := 0.0
var _ghosts_alive := 0
var _ghost_serial := 0


func start_ghost(interval: float, duration: float, life: float, mat := "", max_num := 999) -> void:
	_ghost_interval = maxf(0.01, interval)
	_ghost_duration = maxf(0.05, duration)
	_ghost_mat = mat
	_ghost_max = maxi(1, max_num)
	_ghost_left = maxf(life, _ghost_interval)
	_ghost_timer = 0.0


func _spawn_ghost() -> void:
	if model == null or model.model == null or _ghosts_alive >= _ghost_max:
		return
	var holder := Node3D.new()
	_ghost_serial += 1
	holder.name = "Ghost%d" % _ghost_serial   # unique: a clash would make Godot call it "@Node3D@n"
	holder.top_level = true
	get_tree().current_scene.add_child(holder)
	holder.global_transform = Transform3D.IDENTITY
	var any := false
	var mats: Array = []
	var em: Dictionary = GHOST_MATS.get(_ghost_mat, {})
	for mi in model.model.find_children("*", "MeshInstance3D", true, false):
		if mi.mesh == null or mi.get_skeleton_path() == NodePath(""):
			continue
		var baked: ArrayMesh = mi.bake_mesh_from_current_skeleton_pose()
		if baked == null:
			continue
		var g := MeshInstance3D.new()
		g.mesh = baked
		var sk: Node = mi.get_node_or_null(mi.get_skeleton_path())
		g.global_transform = sk.global_transform if sk is Node3D else mi.global_transform
		for si in baked.get_surface_count():
			var src: Material = mi.get_active_material(si)
			var tex: Texture2D = src.albedo_texture if src is BaseMaterial3D else null
			var gm: Material
			if em.is_empty():
				# the character's own material, alpha-blended, alpha = _AdjustA
				if src is BaseMaterial3D:
					var d := (src as BaseMaterial3D).duplicate() as BaseMaterial3D
					d.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
					d.depth_draw_mode = BaseMaterial3D.DEPTH_DRAW_OPAQUE_ONLY
					gm = d
				else:
					var d2 := StandardMaterial3D.new()
					d2.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
					d2.albedo_texture = tex
					gm = d2
			else:
				var sm := ShaderMaterial.new()
				sm.shader = load("res://scenes3d/scn3d_sfx_rim_add.gdshader")
				sm.set_shader_parameter("tex", tex)
				sm.set_shader_parameter("edge_color", (em["edge"] as Color) * float(em["enhance"]))
				sm.set_shader_parameter("rim_power", float(em["rim"]))
				sm.set_shader_parameter("enhance", 1.0)
				sm.set_shader_parameter("use_vcol_mod", true)
				sm.set_shader_parameter("vcol_mod", (em["adjust"] as Color) * float(em["enhance"]))
				sm.set_shader_parameter("adjust_a", 1.0)
				gm = sm
			g.set_surface_override_material(si, gm)
			mats.append(gm)
		g.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
		holder.add_child(g)
		any = true
	if not any:
		holder.queue_free()
		return
	_ghosts_alive += 1
	var dur := _ghost_duration
	var tw := holder.create_tween()
	tw.tween_method(func(a: float) -> void:
		for m in mats:
			if m is BaseMaterial3D:
				var c: Color = (m as BaseMaterial3D).albedo_color
				c.a = a
				(m as BaseMaterial3D).albedo_color = c
			elif m is ShaderMaterial:
				(m as ShaderMaterial).set_shader_parameter("adjust_a", a), 1.0, 0.0, dur)
	var wr := weakref(self)   # the view may be gone (the entity left) before its last afterimage fades
	tw.tween_callback(func() -> void:
		var v = wr.get_ref()
		if v != null:
			v._ghosts_alive = maxi(0, v._ghosts_alive - 1)
		holder.queue_free())


# Mounted / dismounted: the horse model under the rider, the rider on the horse's ma_qi1 hang point with its riding
# clips (anim_group 20 of the reference: xx 85 sits, zp 84 rides, weapon 80/81, magic 82), the horse on group 21
func _on_riding_changed(on: bool) -> void:
	if model == null:
		return
	if on and horse == null and horse_loader.is_valid():
		var h: Node3D = horse_loader.call(npc)
		if h == null:
			return
		horse = h
		horse.name = "Horse"
		add_child(horse)
		# the seat: hinge ma_qi1 (hinge_list 50 [TK]) on the horse's Bip001 Spine1 bone; RideUnit.CreateRide [TK 0x5bd930]
		# parents the rider there with an identity local transform (set_localPosition zero, set_localRotation identity)
		var seat: Node3D = horse.hang_node("ma_qi1") if horse.has_method("hang_node") else null
		_rider_group = str(model.group)
		remove_child(model)
		if seat != null:
			seat.add_child(model)
			model.transform = Transform3D.IDENTITY
			# the rider's Model child was turned 180 for -Z; the seat hinge is in the horse's own (turned) frame: undo that
			if model.model != null:
				model.model.rotation.y = 0.0
		else:
			horse.add_child(model)
			model.position = Vector3(0, 1.3, 0)
			Log.warn("map3d", "horse has no ma_qi1 seat", {"entity": npc.get("entity_id"), "cha": horse.get("cha")})
		model.set_group("20")
		bar_height = float(horse.bar_height) + 0.6
	elif not on and horse != null:
		var parent := model.get_parent()
		if parent != null:
			parent.remove_child(model)
		add_child(model)
		model.transform = Transform3D.IDENTITY
		if model.model != null:
			model.model.rotation.y = PI
		model.set_group(_rider_group if _rider_group != "20" else "1")
		horse.queue_free()
		horse = null
		bar_height = float(model.get("bar_height")) if model.get("bar_height") != null else 2.0
	if npc.get("doing") != null and npc.get("total_frame") != null:
		_on_doing_changed(int(npc.doing), int(npc.total_frame))


# The weapon's animation group (anim_group of the weapon row): AnimStator.UpdateAutoGroup [TK 0x4d6210] keeps group 20
# (RideHorse) as long as the creature rides, the weapon group is what it goes back to on dismounting
func set_weapon_group(g: String) -> void:
	if model == null:
		return
	if horse != null:
		_rider_group = g
		model.set_group("20")
	else:
		model.set_group(g)


# The horse rows of the 0xad sync changed while mounted: dismount and mount again so the world view picks the horse anew
func reload_horse() -> void:
	if horse == null or not (npc.get("riding") is bool and bool(npc.riding)):
		return
	_on_riding_changed(false)
	_on_riding_changed(true)


# KNpc's doing changed (its frame count too): the matching clip of the model, as long as the zone says
func _on_doing_changed(doing: int, total_frame: int) -> void:
	_doing = doing
	if model == null:
		return
	if horse != null:
		# the horse's own clips (group 21: xx stands, zp runs); the rider follows below with group 20
		match doing:
			KNpcResNode.Doing.WALK, KNpcResNode.Doing.FIGHT_WALK, KNpcResNode.Doing.RUN, KNpcResNode.Doing.FIGHT_RUN, KNpcResNode.Doing.JUMP:
				horse.walk()
			_:
				horse.idle()
	var seconds := float(maxi(total_frame, 1)) * TICK
	match doing:
		KNpcResNode.Doing.STAND, KNpcResNode.Doing.FIGHT_STAND, KNpcResNode.Doing.SIT:
			model.idle()
		KNpcResNode.Doing.STAND1:
			# the second idle (one cycle in six, KNpc::OnStand): the model's small motion when it has one
			if model.has_clip("xdz"):
				model.play_once("xdz", seconds)
			else:
				model.idle()
		KNpcResNode.Doing.WALK, KNpcResNode.Doing.FIGHT_WALK:
			model.walk()
		KNpcResNode.Doing.RUN, KNpcResNode.Doing.FIGHT_RUN, KNpcResNode.Doing.JUMP:
			model.run()
		KNpcResNode.Doing.ATTACK, KNpcResNode.Doing.ATTACK1:
			var cname: String = model.attack_clip()
			if cname == "" or model.play_once(cname, seconds) == "":
				model.idle()
		KNpcResNode.Doing.MAGIC:
			if model.play_once(model.clip_for("magic"), seconds) == "":
				model.idle()
		KNpcResNode.Doing.HURT:
			var hurt: String = model.pick_clip("ss")
			if hurt == "" or model.play_once(hurt, seconds) == "":
				model.idle()
		KNpcResNode.Doing.DEATH:
			var dead: String = model.pick_clip("sw")
			if dead != "":
				model.play_once(dead, seconds, true)
		_:
			model.idle()
