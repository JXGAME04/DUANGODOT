extends SceneTree
# glow probe: does Environment.glow work in the Compatibility renderer of this Godot? A bright unshaded quad (albedo 4.0) on a
# black background; measure the pixel brightness just outside the quad with glow off / on.

func _init() -> void:
	var root3d := Node3D.new()
	get_root().add_child(root3d)
	for a in OS.get_cmdline_user_args():
		if a == "--hdr2d":
			get_root().use_hdr_2d = true
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color.BLACK
	env.ambient_light_source = Environment.AMBIENT_SOURCE_DISABLED
	var we := WorldEnvironment.new()
	we.environment = env
	root3d.add_child(we)
	var cam := Camera3D.new()
	cam.position = Vector3(0, 0, 5)
	root3d.add_child(cam)
	cam.current = true
	var mi := MeshInstance3D.new()
	var qm := QuadMesh.new()
	qm.size = Vector2(1.0, 1.0)
	mi.mesh = qm
	var m := StandardMaterial3D.new()
	m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	m.albedo_color = Color(4.0, 4.0, 4.0)
	mi.material_override = m
	root3d.add_child(mi)
	for i in 4:
		await process_frame
	var a := _probe()
	env.glow_enabled = true
	env.glow_hdr_threshold = 0.5
	env.glow_intensity = 1.0
	env.glow_strength = 1.0
	env.glow_bloom = 0.0
	env.glow_blend_mode = Environment.GLOW_BLEND_MODE_ADDITIVE
	for arg in OS.get_cmdline_user_args():
		if arg == "--strong":
			env.glow_intensity = 8.0
			env.glow_strength = 1.5
			env.glow_bloom = 1.0
			for lv in range(1, 8):
				env.set("glow_levels/%d" % lv, 1.0)
	for i in 4:
		await process_frame
	var b := _probe()
	print("GLOW_PROBE method=%s off=%s on=%s" % [RenderingServer.get_current_rendering_method(), str(a), str(b)])
	quit()


func _probe() -> Array:
	var img := get_root().get_texture().get_image()
	var w := img.get_width()
	var h := img.get_height()
	# the quad covers the centre; sample a ring of pixels 12% of the width outside its edge, and the centre
	var out: Array = []
	for f in [0.5, 0.53, 0.56, 0.59, 0.62, 0.66, 0.7, 0.75, 0.8, 0.9]:
		out.append(snappedf(img.get_pixel(int(w * f), h / 2).r, 0.001))
	return out
