# UiSkillState - the skill state list of the VLTK 2.0 client (KUiSkillState, 技能状态列表.ini; gamecl.exe LoadScheme
# 0x0041F460, the [BuffList] table 0x0041EFF0, the update 0x0041EA60, the time text 0x0041D720; docs/CLIENT-2.0.md §9):
#   - [Main] (146,28) 240 x 72; ten [BuffImage] 24 x 24 at x = 24 i on the top row, each with a [txtBuffTime] under it
#     (0,21); ten [DebuffImage] at y = 36 with [txtDebuffTime]; all hidden until a state fills them.
#   - [BuffList]: BuffCount entries Buff_%d_ID (the skill), Buff_%d_Level (-1 = any), Buff_%d_IsDebuff, Buff_%d_Name,
#     Buff_%d_Image, Buff_%d_Desc - the icon and the words of a state, keyed by the skill that put it.
#   - the update (every ninth frame): the character's own states (OperationRequest 0x8c); a state whose skill has an
#     entry (and the level fits) takes the next buff slot, or the next debuff slot when IsDebuff; the icon is the entry's
#     picture, the text under it the time left (0x0041D720: "N/A", "%ds", "%dm", "%dh"), the tip "name\ndesc\ntime".
# The zone tells the character its states with G2C_ENTITY_STATE (the 0x87 packet of KNpc::SetStateSkillEffect
# 0x08086260 to the player itself; RemoveStateSkillEffect 0x0807D310 sends it empty).
extends "res://ui/elem/KWndWindow.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndImage := preload("res://ui/elem/KWndImage.gd")
const StateMath := preload("res://ui/KUiStateMath.gd")

const SCHEME := "trang-thai-ky-nang"

signal state_hovered(text: String)   # "" = none

var _buff_images: Array = []
var _buff_times: Array = []
var _debuff_images: Array = []
var _debuff_times: Array = []
var _table := {}          # skill id -> [{level, debuff, name, desc, image}]
var _shown: Array = []    # {skill_id, debuff, slot, entry} of the last refresh
var _ini: KUiScheme = null
var _tick := 0.0


func load_scheme(screen: Vector2i) -> bool:
	_ini = KUiScheme.open(SCHEME)
	if _ini == null or not init_from(_ini, "Main"):
		return false
	# the 2.0 layout is drawn for 1024 x 768: the list keeps its place in the centred frame (like the bars)
	position.x += (screen.x - _ini.screen_size().x) / 2.0
	name = "UiSkillState"
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	for i in StateMath.SLOTS:
		_buff_images.append(_slot_image("BuffImage", i, false))
		_buff_times.append(_slot_text("txtBuffTime", i, false))
		_debuff_images.append(_slot_image("DebuffImage", i, true))
		_debuff_times.append(_slot_text("txtDebuffTime", i, true))
	# [BuffList] 0x0041EFF0
	var count := _ini.get_integer("BuffList", "BuffCount", 0)
	for k in count:
		var id := _ini.get_integer("BuffList", "Buff_%d_ID" % k, 0)
		if id <= 0:
			continue
		var entry := {
			"level": _ini.get_integer("BuffList", "Buff_%d_Level" % k, -1),
			"debuff": _ini.get_integer("BuffList", "Buff_%d_IsDebuff" % k, 0) != 0,
			"name": _ini.get_string("BuffList", "Buff_%d_Name" % k, ""),
			"desc": _ini.get_string("BuffList", "Buff_%d_Desc" % k, ""),
			"image": _ini.image("BuffList", "Buff_%d_Image" % k),
		}
		if not _table.has(id):
			_table[id] = []
		_table[id].append(entry)
	Log.info("ui", "skill state list", {"buffs": _table.size()})
	return true


func _slot_image(section: String, i: int, debuff: bool) -> KWndImage:
	var img := KWndImage.new()
	add_child(img)
	img.init_from(_ini, section)
	img.position = Vector2(StateMath.slot_pos(i, debuff))
	img.mouse_filter = Control.MOUSE_FILTER_STOP
	img.visible = false
	img.mouse_entered.connect(_on_slot_entered.bind(i, debuff))
	img.mouse_exited.connect(func(): state_hovered.emit(""))
	return img


func _slot_text(section: String, i: int, debuff: bool) -> KWndText:
	var t := KWndText.new()
	add_child(t)
	t.init_from(_ini, section)
	t.position += Vector2(StateMath.slot_pos(i, debuff))
	t.visible = false
	return t


# the entry of a state: the skill's rows of [BuffList] whose level is -1 or the state's (0x0041EC45)
func _entry_of(skill_id: int, level: int):
	for e in _table.get(skill_id, []):
		if int(e.level) == -1 or int(e.level) == level:
			return e
	return null


func refresh() -> void:
	_shown.clear()
	for i in StateMath.SLOTS:
		_buff_images[i].visible = false
		_buff_times[i].visible = false
		_debuff_images[i].visible = false
		_debuff_times[i].visible = false
	var buffs := 0
	var debuffs := 0
	var now := Time.get_ticks_msec()
	for skill_id in Game.states:
		var st: Dictionary = Game.states[skill_id]
		var entry = _entry_of(int(skill_id), int(st.get("level", 0)))
		if entry == null:
			continue
		var seconds := -1
		if int(st.get("time", -1)) >= 0:
			seconds = maxi(0, int(ceil((int(st.get("until_ms", now)) - now) / 1000.0)))
		var text := StateMath.time_text(seconds) if seconds >= 0 else ""
		if entry.debuff:
			if debuffs >= StateMath.SLOTS:
				continue
			_debuff_images[debuffs].set_image(entry.image)
			_debuff_images[debuffs].visible = true
			_debuff_times[debuffs].text = text
			_debuff_times[debuffs].visible = true
			_shown.append({"skill_id": int(skill_id), "debuff": true, "slot": debuffs, "entry": entry, "text": text})
			debuffs += 1
		else:
			if buffs >= StateMath.SLOTS:
				continue
			_buff_images[buffs].set_image(entry.image)
			_buff_images[buffs].visible = true
			_buff_times[buffs].text = text
			_buff_times[buffs].visible = true
			_shown.append({"skill_id": int(skill_id), "debuff": false, "slot": buffs, "entry": entry, "text": text})
			buffs += 1


func _on_slot_entered(i: int, debuff: bool) -> void:
	for s in _shown:
		if int(s.slot) == i and bool(s.debuff) == debuff:
			state_hovered.emit("%s\n%s\n%s" % [s.entry.name, s.entry.desc, s.text])   # 0x0041ECE4
			return


func _process(delta: float) -> void:
	_tick += delta
	if _tick >= 0.5:   # the 2.0 list redraws every ninth frame; the times move by the second
		_tick = 0.0
		if not _shown.is_empty() or not Game.states.is_empty():
			refresh()
