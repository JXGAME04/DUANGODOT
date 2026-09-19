# One visible entity (KNpc of the old client, client side).  Positions are kept in *scene units*
# (the zone's coordinate system) and projected to the screen as (x, y / 2), the same projection
# the map bundle uses.  Movement follows the waypoints the zone sent at the same speed, so the
# view stays within a tick of the server.  Animation runs on the old 18 Hz logic tick: the doing
# (stand / walk / run / attack / hurt / death) picks an action, cur_frame counts to the action's
# frame count (KNpc::WaitForFrame) and KNpcRes maps that progress onto the sprite frames.
extends Node2D

const KNpcResScript := preload("res://scenes/KNpcRes.gd")
const KNpcResNode := preload("res://scenes/KNpcResNode.gd")
const KNpcGold := preload("res://scenes/KNpcGold.gd")
const KMath := preload("res://scenes/KMath.gd")

const RADIUS := 14.0
const TICK := 1.0 / 18.0        # old logic frame
const RUN_SPEED := 150.0        # scene units / s: faster plays the run action (zone default is 200)
const ENTITY_PLAYER := 1        # jx.pb.EntityType
const ENTITY_MONSTER := 3
# jx.pb.Action
const ACTION_STAND := 0
const ACTION_ATTACK := 1
const ACTION_HURT := 2
const ACTION_DEATH := 3
const ACTION_REVIVE := 4
const ACTION_JUMP := 5
const ACTION_KNOCK_BACK := 6
const ACTION_SIT := 7
const SIT_FRAME := 15              # m_SitFrame +0x1930 of the server (KNpc::Init 0x0807E09B): the sit runs 15 frames, then holds
# the name block over a character as gamecl.exe 0x005F2DB0 sizes it with names shown: 3 (life bar) + 5, the name
# line 12 + 2 - the Head state pictures hang from it (0x006DFAC0: z = block height + 9 - 100)
const INFO_LINES := 22

var entity_id := 0
var entity_type := 0
var display_name := ""
var template_id := 0
var sex := 0
var scene_pos := Vector2.ZERO      # scene units
var path: Array = []               # remaining waypoints (Vector2, scene units)
var speed := 0.0                   # scene units per second
var is_own := false
var is_target := false             # selected by the local player
var dir64 := 0                     # m_Dir: where the entity faces
var res_dir := 0                   # m_ResDir: the drawn facing, turning toward dir64
var doing := -1                    # KNpcResNode.Doing
var total_frame := 15              # m_Frames.nTotalFrame
var cur_frame := 0                 # m_Frames.nCurrentFrame
var life := 0
var life_max := 0
var has_res := false
var frames := {"stand": 15, "stand1": 15, "walk": 12, "run": 15}
var stature := 0                   # m_nStature (npcs.txt): name height above the feet
var _res: Node2D
var _label: Label
var _tick_acc := 0.0
var _rng := RandomNumberGenerator.new()
var _knock_dest := Vector2.ZERO    # KNpc+0x13b4/+0x13b8 of the 2.0 client: where a knock back pushes to
var _knocked := false
var _knock_from := Vector2.ZERO    # the slide of the current logic frame, drawn interpolated (HANDOVER §0.1 rule 13)
var _knock_to := Vector2.ZERO
var state_icons: Array = []        # the six StateSpecialIds of the 0x7a packet (G2C_STATE_ICONS), drawn by KNpcRes
var sounds = null                  # the world's KWavSound (UiGame), null = silent
var level := 0                     # m_Level (+0x28 of the 2.0 client): "%s/Lv:%d" of the name line (0x005F242F)
# KNpcGold at KNpc+0x4c of the 2.0 client (SetGoldType 0x006E3560 from the 0x4c / 0x9a packets): the kind = the
# NpcGoldTemplate row + 1 while gold, 0 plain; a boss carries the server table's count + 1 (KNpcGold.gd)
var gold_type := 0
var hovered := false               # the npc under the mouse (the pate loop 0x0067021A: [core+0xa8c4] == this)
var camp := 4                      # m_Camp (+0xf4 of the 2.0 client, the byte +0xb of the 0x4c packet)
var equip_rows: Dictionary = {}    # the equipment rows of the 0x4a / 0x4b sync (KNpc+0x13f0..+0x1400): group -> row, -1 none
var riding := false                # m_bRideHorse (+0x19c0): the on-horse actions, the pate + 38
var pk_state := 0                  # KNpc+0x16e4 (the 0x4a / 0x4b flag & 3): a player's PK state - the life-bar colour (PaintLife 0x005EADF4)
var current_camp := 4              # m_CurrentCamp (+0xf8, the byte +3): the colour of a player's name (0x005F2507)
# the two show switches (KNpcGold.gd): "showplayername" (F7) and "showplayerlife" (F8) of the option word, shared by every npc
static var name_switch := 3        # this client starts with the names on (2.0 starts at 0: docs/CLIENT-2.0.md §17)
static var life_switch := 0
var _life_label: Label             # the "%d/%d" line over the name line of a monster (0x005F2358)


static func to_screen(p: Vector2) -> Vector2:
	return Vector2(p.x, p.y * 0.5)


static func to_scene(p: Vector2) -> Vector2:
	return Vector2(p.x, p.y * 2.0)


var npc_kind := 0   # NPCKIND (+0x13 of the 0x4c packet): 3 = a dialoger, a click on it talks (the 0x6e packet) instead of attacking


func setup(d: Dictionary, own: bool) -> void:
	entity_id = int(d.id)
	entity_type = int(d.type)
	display_name = str(d.name)
	template_id = int(d.get("template_id", 0))
	sex = int(d.get("sex", 0))
	level = int(d.get("level", 0))
	gold_type = int(d.get("gold_type", 0))
	camp = int(d.get("camp", 4))
	current_camp = int(d.get("current_camp", 4))
	equip_rows = d.get("res", {})
	riding = bool(d.get("riding", false))
	pk_state = int(d.get("pk_state", 0))
	menu_state = int(d.get("menu_state", 0))
	menu_sentence = str(d.get("menu_sentence", ""))
	npc_kind = int(d.get("npc_kind", 0))
	if menu_state != 0:
		call_deferred("_refresh_sign")
	is_own = own
	scene_pos = Vector2(d.x, d.y)
	speed = float(d.speed)
	dir64 = clampi(int(d.get("dir", 0)), 0, 63)
	res_dir = dir64
	life = int(d.get("life", 0))
	life_max = int(d.get("life_max", 0))
	path = _waypoints(d)
	position = to_screen(scene_pos)
	_rng.seed = entity_id
	if _res == null:
		_res = Node2D.new()
		_res.set_script(KNpcResScript)
		_res.name = "Res"
		add_child(_res)
	if _label == null:
		_label = _make_label()
		_life_label = _make_label()
		_life_label.visible = false
	_refresh_name()
	_label.position = Vector2(-70, -RADIUS - 26)
	# appearance: players are the composed main characters, everything else its npcs.txt template
	var res_name := ""
	if entity_type == ENTITY_PLAYER:
		res_name = KNpcResNode.player_res_name(sex)
		var pf := NpcResList.player_frames(sex)
		frames = {"stand": int(pf.get("stand_frame", 15)), "stand1": int(pf.get("stand_frame", 15)),
			"walk": int(pf.get("walk_frame", 12)), "run": int(pf.get("run_frame", 15))}
	else:
		var tpl := NpcResList.template(template_id)
		res_name = str(tpl.get("res", ""))
		frames = {"stand": int(tpl.get("stand_frame", 15)), "stand1": int(tpl.get("stand_frame1", 15)),
			"walk": int(tpl.get("walk_frame", 12)), "run": int(tpl.get("run_frame", 15))}
		stature = int(tpl.get("stature", 0))
	has_res = _res.setup(res_name)
	if not has_res:
		Log.debug("npcres", "no appearance, drawing a marker", {"entity": entity_id, "type": entity_type,
			"template": template_id, "res": res_name})
	elif entity_type == ENTITY_PLAYER:
		# 0x005F1A15..0x005F1A63: SetRideHorse(+0x19c0), SetArmor(+0x13f4), SetHelm(+0x13f0), SetMantle(+0x13f8), SetHorse(+0x13fc), the weapon
		if not equip_rows.is_empty():
			_res.set_equips(equip_rows)
		_res.set_ride(riding)
	set_state_icons(d.get("state_icons", state_icons))
	# KNpc::GetNpcPate: the name sits m_nStature (+84 for players) above the feet
	_place_labels()
	doing = -1
	_set_doing(KNpcResNode.Doing.STAND)
	# a late joiner sees corpses and swings already under way
	var now := int(d.get("doing", ACTION_STAND))
	if now == ACTION_DEATH:
		_set_action(KNpcResNode.Doing.DEATH, int(d.get("doing_frames", 1)))
		cur_frame = total_frame - 1
	elif now == ACTION_ATTACK or now == ACTION_HURT or now == ACTION_KNOCK_BACK:
		apply_action({"action": now, "frames": d.get("doing_frames", 1), "x": d.x, "y": d.y, "dir": dir64})
	elif now == ACTION_SIT:
		# the 0x4c sync carries m_Doing 8: a late joiner sees the sitter already down (the last frame held)
		_set_action(KNpcResNode.Doing.SIT, SIT_FRAME)
		cur_frame = total_frame - 1
	_tick_acc = 0.0
	queue_redraw()


func apply_move(mv: Dictionary) -> void:
	scene_pos = Vector2(mv.x, mv.y)
	speed = float(mv.speed)
	path = _waypoints(mv)
	position = to_screen(scene_pos)
	if doing == KNpcResNode.Doing.ATTACK or doing == KNpcResNode.Doing.ATTACK1 or doing == KNpcResNode.Doing.SIT:
		_set_doing(KNpcResNode.Doing.STAND)   # KNpc::DoWalk interrupts the swing (and writes m_Doing 3 over a sit)


# EntityAction from the zone: KNpc::DoAttack / DoHurt / DoDeath on the client side.
func apply_action(a: Dictionary) -> void:
	scene_pos = Vector2(a.x, a.y)
	path = []
	position = to_screen(scene_pos)
	dir64 = clampi(int(a.get("dir", dir64)), 0, 63)
	var n := maxi(int(a.get("frames", 1)), 1)
	_knocked = false
	match int(a.action):
		ACTION_ATTACK:
			# one of the two attack animations at random, like KNpc::DoAttack
			_set_action(KNpcResNode.Doing.ATTACK if _rng.randi_range(0, 1) == 1 else KNpcResNode.Doing.ATTACK1, n)
		ACTION_HURT:
			_set_action(KNpcResNode.Doing.HURT, n)
		ACTION_DEATH:
			_set_action(KNpcResNode.Doing.DEATH, n)
			is_target = false
		ACTION_KNOCK_BACK:
			# KNpc::KnockBack of the 2.0 client (gamecl.exe 0x005EE950, the 0x56 packet with doing 0x18): the hurt animation
			# (action 7 = cdo_hurt) for `frames` frames while OnKnockBack 0x005EFE00 slides it to the spot each frame; it faces
			# the way it came from (0x005E8DB0 of here - spot) and keeps its facing when the spot is here.  docs/CLIENT-2.0.md §12
			_knock_dest = Vector2(float(a.get("ax", a.x)), float(a.get("ay", a.y)))
			_knocked = true
			_knock_from = scene_pos
			_knock_to = scene_pos
			var face := KMath.get_dir_index(int(_knock_dest.x), int(_knock_dest.y), int(scene_pos.x), int(scene_pos.y))
			if face >= 0:
				dir64 = face
			_set_action(KNpcResNode.Doing.HURT, n)
		ACTION_SIT:
			# the 0x83 packet (KNpc::DoSit 0x0807B550) -> the 2.0 handler 0x00650400 -> KNpc::DoAction(8) 0x005EA2E0: the sit
			# animation plays its frames once and holds the last one (the server's frame 0x08087880 does the same)
			_set_action(KNpcResNode.Doing.SIT, SIT_FRAME)
		ACTION_JUMP:
			# a jump of a style-1 skill (zone KSubWorld::start_jump): to the landing spot within `frames` logic frames;
			# shown as a run until the jump animation of the 2.0 client is wired (B4)
			var land := Vector2(float(a.get("ax", a.x)), float(a.get("ay", a.y)))
			path = [land]
			speed = maxf(1.0, scene_pos.distance_to(land) / (float(n) / 18.0))
			_set_doing(KNpcResNode.Doing.RUN)
		_:
			_set_doing(KNpcResNode.Doing.STAND)


func set_life(l: Dictionary) -> void:
	life = int(l.get("life", life))
	life_max = int(l.get("life_max", life_max))
	if _life_label != null and _life_label.visible:
		_refresh_name()
	queue_redraw()


func _make_label() -> Label:
	var l := Label.new()
	l.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	l.size = Vector2(140, 20)
	l.add_theme_color_override("font_color", Color.WHITE)
	l.add_theme_color_override("font_shadow_color", Color.BLACK)
	l.add_theme_constant_override("shadow_offset_x", 1)
	l.add_theme_constant_override("shadow_offset_y", 1)
	add_child(l)
	return l


# The name block as gamecl.exe 0x005F21B0 draws it: a monster (kind 0) gets "%d/%d" (0x005F2358, white) over "%s/Lv:%d"
# (0x005F242F) in the colour of its gold kind (0x005F23E5: none = white, a kind = 0xFF6365FF, above the client's table =
# 0xFFEBB200) - only as the show switch and the hover / target allow (KNpcGold.name_block: size 14 with a black outline when
# hovered or targeted, 12 with the switch's second bit, else nothing); players and the other kinds keep their name.
# docs/CLIENT-2.0.md §16 / §17
func _refresh_name() -> void:
	if _label == null:
		return
	var block := KNpcGold.name_block(entity_type, name_switch, hovered or is_target)
	_label.visible = block != 0
	_life_label.visible = block != 0 and entity_type == ENTITY_MONSTER
	if block == 0:
		return
	_label.text = KNpcGold.name_text(display_name, entity_type, level)
	_label.add_theme_color_override("font_color", name_color())
	_life_label.text = "%d/%d" % [life, life_max]
	for l in [_label, _life_label]:
		l.add_theme_font_size_override("font_size", block)
		# the block is as wide as its text (the 2.0 painter measures each line, 0x005F22A2 / 0x005F2445)
		var font: Font = l.get_theme_font("font")
		var w: float = font.get_string_size(l.text, HORIZONTAL_ALIGNMENT_CENTER, -1, block).x + 6.0
		l.size = Vector2(w, float(block) + 6.0)
		l.position.x = -w * 0.5
		# 0x006702ED hands OutputText a BorderColor of 0xff000000 for the hovered / targeted npc (iRepresentShell::OutputText's last
		# argument is the outline of the letters, not a background): a black outline at 14, none at 12
		l.add_theme_constant_override("outline_size", 2 if block == 14 else 0)
		l.add_theme_color_override("font_outline_color", Color(0, 0, 0, 1))


# the pate loop's hover (0x0067021A) and the switches (F7 / F8) changed: the block again
func refresh_info() -> void:
	_refresh_name()
	queue_redraw()


func set_hovered(on: bool) -> void:
	if hovered == on:
		return
	hovered = on
	refresh_info()


func name_color() -> Color:
	if entity_type == ENTITY_PLAYER:
		return KNpcGold.player_name_color(current_camp)   # 0x005F2507: the table 0x5f2d94 by +0xf8
	return KNpcGold.name_color(entity_type, gold_type, NpcResList.gold_rows())


# the 0x59 / 0x58 packets (G2C_ENTITY_CAMP): the camps, and a player's name colour with them
func set_camp(c: int, current: int) -> void:
	camp = c
	current_camp = current
	_refresh_name()


# the 0xad packet (KNpc::SetPlayerRes 0x005ED920 -> 0x005EBF90): the equipment rows of a player - the pictures are picked again
func set_equip_rows(rows: Dictionary) -> void:
	equip_rows = rows
	if has_res and entity_type == ENTITY_PLAYER:
		_res.set_equips(rows)
		queue_redraw()


# a team mate of the character (0x0066D070 == 8 in PaintLife 0x005EADB8): the life bar is (230, 190, 0) before any PK colour
var team_mate := false
# KNpc+0x1cb4 of the 2.0 client (0x006DDD70): the sign over the head (KPlayerMenuStateGraph, menustate01..04.spr drawn as the
# first extra picture of KNpcRes::Draw 0x006DFD83, skipped for the stall 5) and, for the trade sign, the sentence (+0x1cc0,
# 24 characters, 0x006DFBD3) written above it
var menu_state := 0
var menu_sentence := ""
var _sign: Sprite2D = null
var _sign_label: Label = null


func set_menu_state(state: int, sentence: String) -> void:
	if menu_state == state and menu_sentence == sentence:
		return
	menu_state = state
	menu_sentence = sentence
	_refresh_sign()


func _refresh_sign() -> void:
	var row: Dictionary = NpcResList.menu_state(menu_state) if menu_state > 0 and menu_state != 5 else {}
	var atlas = Assets.sprite(str(row.get("sprite", ""))) if not row.is_empty() else null
	if atlas == null:
		if _sign != null:
			_sign.visible = false
		if _sign_label != null:
			_sign_label.visible = false
		return
	if _sign == null:
		_sign = Sprite2D.new()
		_sign.centered = false
		_sign.z_index = 2
		add_child(_sign)
	_sign.texture = atlas.frame_texture(0)
	_sign.visible = _sign.texture != null
	if _sign.texture != null:
		var sz := _sign.texture.get_size()
		_sign.position = Vector2(-sz.x / 2.0, -float(_pate()) - 20.0 - 16.0 - sz.y)
	if menu_state == 2 and menu_sentence != "":
		if _sign_label == null:
			_sign_label = Label.new()
			_sign_label.add_theme_font_size_override("font_size", 12)
			_sign_label.add_theme_color_override("font_color", Color(255.0 / 255.0, 217.0 / 255.0, 78.0 / 255.0))
			_sign_label.add_theme_color_override("font_outline_color", Color.BLACK)
			_sign_label.add_theme_constant_override("outline_size", 2)
			_sign_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
			_sign_label.z_index = 2
			add_child(_sign_label)
		_sign_label.text = menu_sentence.substr(0, 24)
		_sign_label.visible = true
		_sign_label.size = Vector2(160, 16)
		_sign_label.position = Vector2(-80.0, _sign.position.y - 16.0)
	elif _sign_label != null:
		_sign_label.visible = false


func set_team_mate(on: bool) -> void:
	if team_mate == on:
		return
	team_mate = on
	queue_redraw()


# the flag & 3 of the 0x4b sync (0x0065D617 -> +0x16e4): the PK state colours the life bar
func set_pk_state(s: int) -> void:
	if pk_state == s:
		return
	pk_state = s
	queue_redraw()


# the ride flag of the 0x4a / 0x4b sync -> KNpc::SetRideHorse 0x005EC3E0 -> KNpcRes::SetRideHorse 0x006DF420: the on-horse
# actions of the same doing, the name 38 higher (GetNpcPate)
func set_riding(on: bool) -> void:
	if riding == on:
		return
	riding = on
	if has_res and entity_type == ENTITY_PLAYER:
		_res.set_ride(on)
	_place_labels()
	queue_redraw()


# KNpcGold::SetGoldType 0x006E3560 (the 0x9a packet): the kind, 0 = plain again
func set_gold_type(kind: int) -> void:
	gold_type = kind
	_refresh_name()


# gamecl.exe 0x00642550: the class the hang-up target filter and the cursor (0x0069D25D: 0xf for any gold kind) sort a
# npc into - 1 plain, 2 gold, 3 above the client's table (a boss)
func npc_class() -> int:
	return KNpcGold.npc_class(gold_type, NpcResList.gold_rows())


# The 0x7a packet (KNpc::SetNpcState of the old client): the states shown on the body - KNpcRes::SetState picks
# their pictures from the state graphics table.  docs/CLIENT-2.0.md §14
func set_state_icons(icons: Array) -> void:
	state_icons = []
	for v in icons:
		state_icons.append(int(v))
	if has_res:
		_res.set_state_spr(state_icons)


# The state pictures drawn on the body now (the --auto proof): [{id, type, frame, behind, rect}].
func state_spr_info() -> Array:
	return _res.state_spr_info() if has_res else []


# gamecl.exe 0x006DFAC0 (the head pictures, called from KNpc::Paint 0x005F2FC4 with GetNpcPate + the name block):
# their z above the feet = pate + block + 9 - 100.
func _head_effect_z() -> int:
	return _pate() + INFO_LINES + 9 - 100


func is_sitting() -> bool:
	return doing == KNpcResNode.Doing.SIT


func set_target(on: bool) -> void:
	is_target = on
	refresh_info()


func is_dead() -> bool:
	return doing == KNpcResNode.Doing.DEATH


# What a click can attack: monsters that are still alive (the zone refuses the rest anyway).
func is_attackable() -> bool:
	return entity_type == ENTITY_MONSTER and not is_dead()


# a dialoger (NPCKIND 3): the 2004 KPlayer::DialogNpc(int) on a click - the zone answers with its script's Say / Talk
func is_dialoger() -> bool:
	return npc_kind == 3 and not is_own and not is_dead()


# True when the point (local coordinates, screen px) lies on the drawn body.
func hit_test(local: Vector2) -> bool:
	if has_res:
		for p in _res.parts:
			var sp: Sprite2D = p.sprite
			if sp.texture == null:
				continue
			if Rect2(sp.position, sp.texture.get_size()).has_point(local):
				return true
		return false
	return local.length() <= RADIUS


func _waypoints(d: Dictionary) -> Array:
	var out: Array = []
	for p in d.get("path", []):
		out.append(Vector2(p[0], p[1]))
	if out.is_empty() and d.has("tx") and (int(d.tx) != int(d.x) or int(d.ty) != int(d.y)):
		out.append(Vector2(d.tx, d.ty))
	return out


func is_moving() -> bool:
	return not path.is_empty()


func target() -> Vector2:
	return path.back() if not path.is_empty() else scene_pos


func _process(delta: float) -> void:
	var budget := speed * delta
	while budget > 0.0 and not path.is_empty():
		var wp: Vector2 = path[0]
		var d := wp - scene_pos
		var dist := d.length()
		if dist <= budget:
			scene_pos = wp
			budget -= dist
			path.pop_front()
		else:
			scene_pos += d / dist * budget
			budget = 0.0
	position = to_screen(scene_pos)
	if not path.is_empty():
		var wp: Vector2 = path[0]
		var d := KMath.get_dir_index(int(scene_pos.x), int(scene_pos.y), int(wp.x), int(wp.y))
		if d >= 0:
			dir64 = d
	_tick_acc += delta
	while _tick_acc >= TICK:
		_tick_acc -= TICK
		_tick()
	if _knocked:
		# the logic slides once per frame (OnKnockBack); the picture goes the frame's way smoothly (rule 13)
		position = to_screen(_knock_from.lerp(_knock_to, clampf(_tick_acc / TICK, 0.0, 1.0)))


# One old logic frame: choose the doing, advance the frame counter, turn, and draw.
@warning_ignore("integer_division")
func _tick() -> void:
	if doing == KNpcResNode.Doing.DEATH or doing == KNpcResNode.Doing.SIT:
		# KNpc::OnDeath: the corpse keeps its last frame until the zone removes it; a sitter holds its last frame too
		# (0x08087880) until the zone says it stood up (ACTION_STAND) or moved
		if cur_frame < total_frame - 1:
			cur_frame += 1
	elif doing == KNpcResNode.Doing.ATTACK or doing == KNpcResNode.Doing.ATTACK1 or doing == KNpcResNode.Doing.HURT:
		if _knocked:
			# KNpc::OnKnockBack 0x005EFE00: a frame's share of the way left, then the frame count as for a hurt
			_knock_from = scene_pos
			scene_pos += KMath.knock_step(scene_pos, _knock_dest, total_frame - cur_frame)
			_knock_to = scene_pos
		cur_frame += 1
		if cur_frame >= total_frame:   # KNpc::OnSpecial1 / OnHurt / OnKnockBack -> DoStand
			_knocked = false
			_set_doing(KNpcResNode.Doing.STAND)
	else:
		var want: int = KNpcResNode.Doing.STAND
		if is_moving():
			want = KNpcResNode.Doing.RUN if speed >= RUN_SPEED else KNpcResNode.Doing.WALK
		elif doing == KNpcResNode.Doing.STAND1:
			want = KNpcResNode.Doing.STAND1   # the idle variant plays to its end
		if want != doing:
			_set_doing(want)
		else:
			cur_frame += 1
			if cur_frame >= total_frame:   # KNpc::WaitForFrame
				cur_frame = 0
				if not is_moving():        # KNpc::OnStand: one cycle in six is the second idle
					_set_doing(KNpcResNode.Doing.STAND if _rng.randi_range(0, 5) != 1 else KNpcResNode.Doing.STAND1)
	# KNpc::Paint: the drawn facing turns half the way toward the real one every frame
	if res_dir != dir64:
		var off := dir64 - res_dir
		if off > 32:
			off -= 64
		elif off < -32:
			off += 64
		res_dir = posmod(res_dir + (off / 2 if absi(off) > 1 else off), 64)
	if has_res:
		_res.paint(res_dir, total_frame, cur_frame, _head_effect_z())
	if entity_type == ENTITY_PLAYER:
		_place_labels()   # a sitter's name sinks over the last sit frames and comes back up on standing
	_play_action_sound()


# KNpcRes::Draw 0x006E06E5: while the action is under 5 % done (elapsed / (frames / 18 s) < 0.05 - its first frame, or
# two for long ones) its sound plays at the feet unless the same file is still playing (KNpcRes::PlaySound 0x006DFA20
# with IsPlaying).  The action's clock restarts with every cycle (KNpc::WaitForFrame 0x005EA700 sets +0x10c when the
# frame wraps), so a walk's footsteps come back each cycle and an idle's call each time it plays.
func _play_action_sound() -> void:
	if sounds != null and has_res and cur_frame * 20 < total_frame and _res.sound_name != "":
		sounds.play(_res.sound_name, scene_pos, false, true)


# KNpc::GetNpcPate 0x005EBCF0: m_nStature (+84 for a player, 0x005EC13D) + m_nHeight; a sitting player's head sinks with the
# sit frames (KNpcGold.sit_pate_drop); riding adds 38 (no jump height or riding on this client yet).
func _pate() -> int:
	var h := stature
	if entity_type == ENTITY_PLAYER:
		h += 84 - KNpcGold.sit_pate_drop(doing == KNpcResNode.Doing.SIT, cur_frame, total_frame)
		if riding:
			h += 38   # 0x005EBD58: +0x19c0 -> + 0x26
	return h


# where the pate puts the name lines this frame (the pate loop 0x00670130 measures it every frame: a sitter's sink)
func _place_labels() -> void:
	if _label == null:
		return
	_label.position.y = -float(_pate()) - 20.0
	_life_label.position.y = _label.position.y - 16.0
	if _sign != null and _sign.visible and _sign.texture != null:
		_sign.position.y = _life_label.position.y - _sign.texture.get_size().y
		if _sign_label != null and _sign_label.visible:
			_sign_label.position.y = _sign.position.y - 16.0


func _set_doing(d: int) -> void:
	doing = d
	cur_frame = 0
	var n := 15
	match d:
		KNpcResNode.Doing.STAND:
			n = int(frames.stand)
		KNpcResNode.Doing.STAND1:
			n = int(frames.stand1)
		KNpcResNode.Doing.WALK:
			n = int(frames.walk)
		KNpcResNode.Doing.RUN:
			n = int(frames.run)
	total_frame = maxi(n, 1)
	if has_res:
		_res.set_action(d)
		queue_redraw()
	_play_action_sound()


# An action whose length the zone dictates (attack / hurt / death frames).
func _set_action(d: int, n: int) -> void:
	doing = d
	cur_frame = 0
	total_frame = maxi(n, 1)
	if has_res:
		_res.set_action(d)
	queue_redraw()
	_play_action_sound()


func _draw() -> void:
	if not has_res:
		var color := Color(0.95, 0.6, 0.2)          # npc
		if entity_type == ENTITY_PLAYER:
			color = Color(0.3, 0.9, 0.4)             # other player
		if is_own:
			color = Color(0.35, 0.65, 1.0)           # me
		elif entity_type == ENTITY_MONSTER:
			color = Color(0.95, 0.3, 0.3)            # monster
		draw_circle(Vector2.ZERO, RADIUS, color)
		draw_arc(Vector2.ZERO, RADIUS, 0, TAU, 24, Color(0, 0, 0, 0.6), 2.0)
	# KNpc::PaintLife 0x005EACF0 as the pate loop calls it (KNpcGold.life_bar): players with the life switch, monsters hovered /
	# targeted or with its second bit; a bar needs a maximum (0x005EAD31).  0x005EADA1..0x005EAEF4: pct = round(life x 100 / max),
	# the filled part pct x 38 / 100 wide from x - 19, 3 tall, coloured by KNpcGold.life_bar_color; the rest to x + 19 in grey 0x808080
	if life_max > 0 and KNpcGold.life_bar(entity_type, life_switch, hovered or is_target):
		var pct := int(round(float(life) * 100.0 / float(life_max)))
		var w := float(pct * 38 / 100)
		var top := Vector2(-19.0, -float(_pate()) + 2.0)
		draw_rect(Rect2(top, Vector2(w, 3.0)), KNpcGold.life_bar_color(pct, pk_state, pk_state != 0, team_mate and not is_own))
		draw_rect(Rect2(top + Vector2(w, 0.0), Vector2(38.0 - w, 3.0)), Color(0.5, 0.5, 0.5))
	if is_target and not is_dead():
		draw_arc(Vector2(0, 0), 18.0, 0, TAU, 24, Color(1.0, 0.9, 0.2, 0.8), 2.0)
