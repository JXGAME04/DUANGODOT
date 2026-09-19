# Scn3DNpc - mot nhan vat/NPC 3D: nap glTF (skin + animation) do tools/scn3d/export_npc.py xuat, phat animation
# nghi ("xx") lap, nhan ten tren dau. Model Unity nhin +Z nen con Model quay 180 do de thanh -Z cua Godot;
# node nay quay theo goc dung (rotation.y = 180 + goc trong mark JSON, giong Scn3DPlayer).
extends Node3D

static var _cache := {}   # duong dan -> [GLTFDocument, GLTFState]

var model: Node3D
var anim: AnimationPlayer
var label: Label3D
var cha := 0
var display_name := ""
var current := ""


func setup(dir: String, file: String, scale: float, name_text: String, size_y: float) -> bool:
	var full := dir + "/" + file
	var pair = _cache.get(full)
	if pair == null:
		var doc := GLTFDocument.new()
		var st := GLTFState.new()
		var err := doc.append_from_file(full, st)
		if err != OK:
			push_error("Scn3DNpc: nap %s loi %d" % [full, err])
			return false
		pair = [doc, st]
		_cache[full] = pair
	model = pair[0].generate_scene(pair[1])
	if model == null:
		return false
	model.name = "Model"
	model.rotation.y = PI
	model.scale = Vector3.ONE * scale
	add_child(model)
	anim = _find_anim(model)
	display_name = name_text
	label = Label3D.new()
	label.text = name_text
	label.position = Vector3(0, (size_y if size_y > 0.0 else 2.1) * scale + 0.15, 0)
	label.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	label.no_depth_test = true
	label.font_size = 40
	label.pixel_size = 0.004
	label.outline_size = 10
	label.modulate = Color(1.0, 0.95, 0.6)
	add_child(label)
	play("xx")
	return true


func _find_anim(n: Node) -> AnimationPlayer:
	if n is AnimationPlayer:
		return n
	for c in n.get_children():
		var r := _find_anim(c)
		if r:
			return r
	return null


# key: "xx" nghi, "zp" di/chay, "gjxx" nghi chien dau, "xdz" dong tac nho, "ss" bi danh, "sw" chet
func play(key: String, loop := true) -> bool:
	if anim == null or key == current:
		return anim != null and key == current
	if not anim.has_animation(key):
		return false
	var a := anim.get_animation(key)
	a.loop_mode = Animation.LOOP_LINEAR if loop else Animation.LOOP_NONE
	anim.play(key, 0.15)
	current = key
	return true


func animation_names() -> PackedStringArray:
	return anim.get_animation_list() if anim else PackedStringArray()
