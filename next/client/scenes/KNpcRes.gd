# KNpcRes of the old client: the sprites of one character.  A normal npc is one image per
# doing; a main character is composed of body parts (head, body, hands, weapons ...) drawn in
# the order of its sort table, all sharing the direction and frame of the first part.  Every
# image is drawn from its reference spot (sprite centre, RUIMAGE_RENDER_FLAG_REF_SPOT) placed on
# the character's feet, the shadow underneath.
extends Node2D

const KNpcResNode := preload("res://scenes/KNpcResNode.gd")
const KMath := preload("res://scenes/KMath.gd")

var res: Dictionary = {}
var special := false
var weapon := 0               # equipment row of the weapon group (bare hands = 0)
var doing := -1
var action := -1
var parts: Array = []         # [{index, sprite, atlas, frames, dirs}] in part-slot order
var shadow: Dictionary = {}   # {sprite, atlas, frames, dirs} or empty
var head_top := -40.0         # y of the top of the drawn body (name label anchor)
var _order: Array = []


# KNpcRes::Init
func setup(res_name: String) -> bool:
	_release()
	res = NpcResList.res(res_name)
	special = bool(res.get("special", false))
	doing = -1
	action = -1
	weapon = int(res.get("equips", {}).get("2", 0)) if special else 0
	return not res.is_empty()


# KNpcRes::SetAction: pick the action of a doing and (re)load the part images.
func set_action(new_doing: int) -> bool:
	if res.is_empty() or new_doing < 0:
		return false
	if new_doing == doing:
		return true
	doing = new_doing
	action = KNpcResNode.act_no(res, doing, weapon, false)
	_load_images()
	return action >= 0


func _release() -> void:
	for p in parts:
		p.sprite.queue_free()
	parts.clear()
	if not shadow.is_empty():
		shadow.sprite.queue_free()
		shadow = {}
	_order.clear()


func _load_images() -> void:
	_release()
	if action < 0:
		return
	if special:
		var sh: Array = res.get("shadow", [])
		if action < sh.size():
			shadow = _make(sh[action], -1)
		var equips: Dictionary = res.get("equips", {})
		for part in res.get("parts", []):
			var index := int(part.get("index", 0))
			@warning_ignore("integer_division")
			var group := index / KNpcResNode.PART_SECTS
			if not equips.has(str(group)):
				continue
			var rows: Dictionary = part.get("equips", {})
			var row = rows.get(str(int(equips[str(group)])))
			if row == null or action >= row.size():
				continue
			var img := _make(row[action], index)
			if not img.is_empty():
				parts.append(img)
	else:
		var acts: Array = res.get("actions", [])
		if action < acts.size():
			var entry: Dictionary = acts[action]
			var img := _make(entry, KNpcResNode.NORMAL_NPC_PART)
			if not img.is_empty():
				parts.append(img)
			if str(entry.get("shadow", "")) != "":
				shadow = _make({"s": entry.shadow, "frames": entry.get("frames", 16), "dirs": entry.get("dirs", 8)}, -1)


func _make(entry: Dictionary, index: int) -> Dictionary:
	var sid := str(entry.get("s", ""))
	if sid == "":
		return {}
	var atlas = Assets.sprite(sid)
	if atlas == null:
		return {}
	var sp := Sprite2D.new()
	sp.centered = false
	add_child(sp)
	var dirs := maxi(int(entry.get("dirs", 8)), 1)
	return {"index": index, "sprite": sp, "atlas": atlas, "frames": maxi(int(entry.get("frames", 16)), dirs), "dirs": dirs}


# KNpcRes::Draw ("draw" is a CanvasItem signal in Godot, hence the name): dir64 is the facing
# (0..63), cur_frame / all_frame the progress of the action.
func paint(dir64: int, all_frame: int, cur_frame: int) -> void:
	if parts.is_empty() or dir64 < 0 or all_frame <= 0 or cur_frame < 0:
		return
	var first: Dictionary = parts[0]
	var frame := KNpcResNode.frame_no(dir64, all_frame, cur_frame, first.frames, first.dirs)
	for p in parts:
		_apply(p, frame)   # the other parts follow the first one's frame, like the old code
	if not shadow.is_empty():
		_apply(shadow, KNpcResNode.frame_no(dir64, all_frame, cur_frame, shadow.frames, shadow.dirs))
	var f := clampi(frame, 0, first.atlas.frame_count() - 1)
	head_top = first.sprite.position.y
	_reorder(KMath.dir64_to_sprite(dir64, first.dirs), f)


func _apply(p: Dictionary, frame: int) -> void:
	var atlas = p.atlas
	var f := clampi(frame, 0, atlas.frame_count() - 1)
	var sp: Sprite2D = p.sprite
	var tex: Texture2D = atlas.frame_texture(f)
	if sp.texture != tex:
		sp.texture = tex
	var pos: Vector2 = -KNpcResNode.ref_spot(atlas.width, atlas.center_x, atlas.center_y) + atlas.frame_offset(f)
	if sp.position != pos:
		sp.position = pos


# Child order = draw order: shadow, then the parts back to front (weapon effects 10/11 skipped
# like the old Draw), then anything the table does not mention.
func _reorder(dir: int, frame: int) -> void:
	var order := KNpcResNode.sort_order(res, action, dir, frame)
	if order == _order:
		return
	_order = order
	var idx := 0
	if not shadow.is_empty():
		move_child(shadow.sprite, idx)
		idx += 1
	for pi in order:
		var i := int(pi)
		if i == 10 or i == 11:
			continue
		for p in parts:
			if p.index == i:
				move_child(p.sprite, idx)
				idx += 1
