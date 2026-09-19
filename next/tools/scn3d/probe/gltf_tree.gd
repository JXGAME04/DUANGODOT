# In cay node cua mot glTF sau khi Godot nap (kiem skin mot xuong, BoneAttachment...):
#   godot --headless --path client -s ../tools/scn3d/probe/gltf_tree.gd -- <duong dan gltf>
extends SceneTree


func _init() -> void:
	var args := OS.get_cmdline_user_args()
	if args.is_empty():
		print("usage: -- <gltf>")
		quit(1)
		return
	var doc := GLTFDocument.new()
	var st := GLTFState.new()
	var err := doc.append_from_file(args[0], st)
	if err != OK:
		print("load failed ", error_string(err))
		quit(1)
		return
	var root := doc.generate_scene(st)
	_dump(root, 0)
	quit(0)


func _dump(n: Node, depth: int) -> void:
	var extra := ""
	if n is MeshInstance3D:
		var mi := n as MeshInstance3D
		extra = " skeleton=%s skin=%s binds=%d pos=%s" % [str(mi.skeleton), str(mi.skin != null), mi.skin.get_bind_count() if mi.skin != null else -1, str(mi.position)]
		if mi.skin != null:
			var names := []
			for i in mi.skin.get_bind_count():
				names.append(str(mi.skin.get_bind_name(i)) + "/" + str(mi.skin.get_bind_bone(i)))
			extra += " names=" + str(names.slice(0, 3))
			var skel: Skeleton3D = mi.get_node_or_null(mi.skeleton) as Skeleton3D
			if skel != null and mi.skin.get_bind_count() <= 3:
				for i in mi.skin.get_bind_count():
					var b := mi.skin.get_bind_bone(i)
					extra += " | bind %d bone %s ibm_origin=%s rest_origin=%s global_rest=%s" % [i, skel.get_bone_name(b), str(mi.skin.get_bind_pose(i).origin), str(skel.get_bone_rest(b).origin), str(skel.get_bone_global_rest(b).origin)]
	elif n is BoneAttachment3D:
		extra = " bone=%s" % str((n as BoneAttachment3D).bone_name)
	elif n is Skeleton3D:
		extra = " bones=%d" % (n as Skeleton3D).get_bone_count()
	print("  ".repeat(depth) + n.name + " (" + n.get_class() + ")" + extra)
	for c in n.get_children():
		_dump(c, depth + 1)
