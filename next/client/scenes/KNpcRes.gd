# KNpcRes of the old client: the sprites of one character.  A normal npc is one image per
# doing; a main character is composed of body parts (head, body, hands, weapons ...) drawn in
# the order of its sort table, all sharing the direction and frame of the first part.  Every
# image is drawn from its reference spot (sprite centre, RUIMAGE_RENDER_FLAG_REF_SPOT) placed on
# the character's feet, the shadow underneath.  The pictures of the states it holds (docs/CLIENT-2.0.md §14)
# sit in six slots (KNpcRes+0x15b0 of the 2.0 client, KStateSpr): Foot ones under the body, Body ones behind it
# for the frames of their behind range and in front for the rest, Head ones over everything.
extends Node2D

const KNpcResNode := preload("res://scenes/KNpcResNode.gd")
const KMath := preload("res://scenes/KMath.gd")
const KStateSpr := preload("res://scenes/KStateSpr.gd")

var res: Dictionary = {}
var special := false
var weapon := 0               # equipment row of the weapon group (bare hands = 0)
var doing := -1
var action := -1
var parts: Array = []         # [{index, sprite, atlas, frames, dirs}] in part-slot order
var shadow: Dictionary = {}   # {sprite, atlas, frames, dirs} or empty
var head_top := -40.0         # y of the top of the drawn body (name label anchor)
var state_sprs: Array = []    # m_cStateSpr: MAX_STATE_SPR slots of KStateSpr
var frame_time := 0           # SubWorld[0].m_dwCurrentTime: the logic frames painted, the clock of the state pictures
var _order: Array = []
var _draw_order: Array = []   # the children in draw order, as last arranged


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
	_draw_order.clear()


# KNpcRes::SetState (2.0: 0x006DF7E0 from the state list the 0x7a packet built): `icons` are the StateSpecialIds
# the character holds; KStateSpr.sync frees the gone ones and fills free slots from the state graphics table, and a
# slot with a new picture gets its atlas and node here.
func set_state_spr(icons: Array) -> void:
	for s in KStateSpr.sync(state_sprs, icons, NpcResList.state_gfx, frame_time):
		s.atlas = Assets.sprite(s.ctrl.file)
		if s.atlas != null:
			s.sprite = Sprite2D.new()
			s.sprite.centered = false
			s.sprite.visible = false
			add_child(s.sprite)


# The state pictures on the body now: [{id, type, frame, behind, rect}] of the drawn ones (the --auto proof).
func state_spr_info() -> Array:
	var out: Array = []
	for s in state_sprs:
		if s.id == 0 or s.sprite == null or not s.sprite.visible or s.sprite.texture == null:
			continue
		out.append({"id": s.id, "type": s.type, "frame": s.ctrl.cur_frame, "behind": s.behind(),
			"rect": Rect2(s.sprite.position, s.sprite.texture.get_size())})
	return out


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
# head_z: the z (scene units above the feet, screen y = -z * 887 / 1024) of the Head pictures - the 2.0 client's
# 0x006DFAC0 puts them at the name block's height + 9 - 100 (KNpc::_head_effect_z).
func paint(dir64: int, all_frame: int, cur_frame: int, head_z: int = 0) -> void:
	frame_time += 1
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
	_step_state_sprs(dir64, head_z)
	_reorder(KMath.dir64_to_sprite(dir64, first.dirs), f)


# KNpcRes::Draw 0x006E05FD: every state picture steps on the logic clock - its block follows the facing (a turn
# restarts the pass and skips the step), a Loop one runs on, a once-only one is dropped at its last frame; then
# each drawn one is put at the feet like a body part (REF_SPOT: - centre + frame offset), lifted by its z.
func _step_state_sprs(dir64: int, head_z: int) -> void:
	for s in state_sprs:
		s.step(dir64, frame_time)
		var sp: Sprite2D = s.sprite
		if sp == null:
			continue
		if s.id == 0 or not s.loaded or s.type == KStateSpr.STATE_MINIMAP or s.atlas.frame_count() == 0:
			sp.visible = false
			continue
		var fr := clampi(s.ctrl.cur_frame, 0, s.atlas.frame_count() - 1)
		var tex: Texture2D = s.atlas.frame_texture(fr)
		if sp.texture != tex:
			sp.texture = tex
		var z := head_z if s.type == KStateSpr.STATE_HEAD else 0   # Foot / Body: z 0 (no jump height, no riding yet)
		var pos: Vector2 = -KNpcResNode.ref_spot(s.atlas.width, s.atlas.center_x, s.atlas.center_y) + s.atlas.frame_offset(fr)
		pos.y -= float((z * 887) >> 10)   # KRepresentShell2::CoordinateTransform
		if sp.position != pos:
			sp.position = pos
		sp.visible = true


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


# Child order = draw order (KNpcRes::Draw, gamecl.exe 0x006E0340): the shadow, the Foot pictures and the Body ones
# within their behind range, then the parts back to front (weapon effects 10/11 skipped like the old Draw), the
# Body pictures outside their range, and the Head pictures last (0x006DFAC0 draws them after the body).
func _reorder(dir: int, frame: int) -> void:
	var order := KNpcResNode.sort_order(res, action, dir, frame)
	var nodes: Array = []
	if not shadow.is_empty():
		nodes.append(shadow.sprite)
	for s in state_sprs:
		if s.sprite != null and s.sprite.visible and s.behind():
			nodes.append(s.sprite)
	for pi in order:
		var i := int(pi)
		if i == 10 or i == 11:
			continue
		for p in parts:
			if p.index == i:
				nodes.append(p.sprite)
	for s in state_sprs:
		if s.sprite != null and s.sprite.visible and not s.behind() and s.type == KStateSpr.STATE_BODY:
			nodes.append(s.sprite)
	for s in state_sprs:
		if s.sprite != null and s.sprite.visible and s.type == KStateSpr.STATE_HEAD:
			nodes.append(s.sprite)
	if order == _order and nodes == _draw_order:
		return
	_order = order
	_draw_order = nodes
	var idx := 0
	for n in nodes:
		move_child(n, idx)
		idx += 1
