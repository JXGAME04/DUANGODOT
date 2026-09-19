# Scn3DModelTest - xem thu vai model nhan vat canh nhau (nghi + animation xx), chup user://logs/view_rest.png, view_anim.png roi thoat.
extends Node3D

var files := ["cha_25_baizhu.gltf", "cha_26_meihualu.gltf", "cha_1_zj01.gltf"]

func _ready() -> void:
	var light := DirectionalLight3D.new()
	light.rotation_degrees = Vector3(-50, 30, 0)
	add_child(light)
	var env := WorldEnvironment.new()
	var e := Environment.new()
	e.background_mode = Environment.BG_COLOR
	e.background_color = Color(0.5, 0.6, 0.7)
	e.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	e.ambient_light_color = Color(0.6, 0.6, 0.6)
	env.environment = e
	add_child(env)
	var floor_mi := MeshInstance3D.new()
	var pm := PlaneMesh.new()
	pm.size = Vector2(20, 20)
	floor_mi.mesh = pm
	add_child(floor_mi)
	var cam := Camera3D.new()
	cam.position = Vector3(6, 3, 6)
	add_child(cam)
	cam.look_at(Vector3(0, 0.8, 0), Vector3.UP)
	var x := -3.0
	var models: Array = []
	for f in files:
		var doc := GLTFDocument.new()
		var st := GLTFState.new()
		if doc.append_from_file(ProjectSettings.globalize_path("res://assets3d/npc/") + f, st) != OK:
			continue
		var m: Node3D = doc.generate_scene(st, 30.0, false, false)
		add_child(m)
		m.position = Vector3(x, 0, 0)
		m.rotation.y = PI * 0.75
		x += 3.0
		models.append(m)
	await _shot("user://logs/view_rest.png")
	for m in models:
		var ap: AnimationPlayer = _find(m, "AnimationPlayer") as AnimationPlayer
		if ap:
			ap.get_animation("xx").loop_mode = Animation.LOOP_LINEAR
			ap.play("xx")
	for i in 30:
		await get_tree().process_frame
	await _shot("user://logs/view_anim.png")
	get_tree().quit()

func _shot(path: String) -> void:
	for i in 4:
		await get_tree().process_frame
	await RenderingServer.frame_post_draw
	get_viewport().get_texture().get_image().save_png(path)
	print("SHOT ", path)

func _find(n: Node, cls: String) -> Node:
	if n.get_class() == cls:
		return n
	for c in n.get_children():
		var r := _find(c, cls)
		if r:
			return r
	return null
