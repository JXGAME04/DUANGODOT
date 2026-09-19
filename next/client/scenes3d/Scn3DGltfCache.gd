# Scn3DGltfCache - one parsed GLTFDocument/GLTFState per file, a fresh scene from it on every call.
# GLTFDocument.generate_scene adds every node, bone and animation name it makes to the state's unique-name sets, so a
# second scene from the same state gets bones called "Bip001 Spine1_2", "_3", ... - the horse's ma_qi1 seat, the hand
# hang points of a second player and sys_bd were then never found (the rider sank into the horse).  The sets are kept
# as parsed and put back before every generation, so each instance has the file's own names.
extends RefCounted

var _cache := {}   # path -> [doc, state, unique_names, unique_animation_names, [skeleton unique_names...]]


func instantiate(full: String) -> Node:
	var pair = _cache.get(full)
	if pair == null:
		var doc := GLTFDocument.new()
		var st := GLTFState.new()
		var err := doc.append_from_file(full, st)
		if err != OK:
			push_error("Scn3DGltfCache: nap %s loi %d" % [full, err])
			return null
		var sk_names: Array = []
		for sk in st.get_skeletons():
			sk_names.append((sk as GLTFSkeleton).get_unique_names())
		pair = [doc, st, st.get_unique_names(), st.get_unique_animation_names(), sk_names]
		_cache[full] = pair
	var state: GLTFState = pair[1]
	state.set_unique_names(pair[2])
	state.set_unique_animation_names(pair[3])
	var skels := state.get_skeletons()
	for i in mini(skels.size(), (pair[4] as Array).size()):
		(skels[i] as GLTFSkeleton).set_unique_names(pair[4][i])
	# remove_immutable_tracks = false: Godot drops constant rotation tracks (the Bip001 node of an animal) and the
	# AnimationMixer then turns the node back to 0 - the beast lies down
	return pair[0].generate_scene(state, 30.0, false, false)


func has(full: String) -> bool:
	return _cache.has(full)


# the parsed state of a file already instantiated (its meshes for a mesh-mode particle system), null before that
func state(full: String) -> GLTFState:
	var pair = _cache.get(full)
	return pair[1] if pair != null else null
