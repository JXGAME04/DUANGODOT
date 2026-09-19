# The floating texts of the reference client (FloatingText on the TopRoot prefab, bundle f799b063c668 [TK]): the damage
# numbers, the skill names, "miss"; drawn on the world view's 2D name layer above the entity's head.  Each format
# (client/assets3d/ui/floating_text.json, tools/scn3d/export_floating.py) has a font size, a colour (or a top / bottom
# gradient), an outline (effect 2) or shadow (1) in effect_color, an initial offset (NGUI pixels, y up) and four curves
# over the seconds since it was added: offset y, offset x, alpha, scale (FloatingText.Update 0x5a01b0 evaluates them by
# the instance's start time; the text ends past the last key of the alpha curve).
# TopRoot.ShowHpChg 0x5a3550: a heavy hit -> format 10, else 1 / 0 (crit / normal), damage on oneself -> 8;
# ShowSkillName 0x5a3f80: 3..7 by the five elements (kim moc thuy hoa tho), 13 generic; ShowHitMiss: 2.
extends RefCounted

const FMT_NORMAL := 0
const FMT_CRIT := 1
const FMT_MISS := 2
const FMT_SKILL_BASE := 3      # + series 0..4
const FMT_SELF := 8
const FMT_HEAVY := 10
const FMT_SKILL_GENERIC := 13
const FMT_STATE := 21          # 状态掉血数字: the periodic loss of a state
const FMT_EXP := 15

var formats: Array = []
var _items: Array = []         # [{text, fmt, t, node}] node = the entity's view (Node3D) it floats over
var loaded := false
var added := 0                 # the --auto proof


func load_formats() -> bool:
	if loaded:
		return not formats.is_empty()
	loaded = true
	var p := "%s/ui/floating_text.json" % Assets.assets3d_root()
	if FileAccess.file_exists(p):
		var d = Assets.load_json(p)
		if d is Dictionary:
			formats = d.get("formats", [])
	return not formats.is_empty()


func add(view: Node3D, text: String, fmt: int) -> void:
	if not load_formats() or fmt < 0 or fmt >= formats.size() or view == null:
		return
	var f: Dictionary = formats[fmt]
	# maxText: the format keeps that many at once, the oldest goes first (FloatingText.GetTextInstance)
	var same := 0
	for it in _items:
		if int(it["fmt"]) == fmt:
			same += 1
	if same >= int(f.get("max", 20)):
		for i in _items.size():
			if int(_items[i]["fmt"]) == fmt:
				_items.remove_at(i)
				break
	_items.append({"text": text if text != "" else str(f.get("text", "")), "fmt": fmt, "t": 0.0, "node": view})
	added += 1


# A damage / heal of `delta` on the entity: the reference's own-damage style when it is the player, else the normal one
func hp_change(view: Node3D, delta: int, own: bool, heavy := false, state := false) -> void:
	if delta < 0:
		var fmt := FMT_SELF if own else (FMT_HEAVY if heavy else (FMT_STATE if state else FMT_NORMAL))
		add(view, _number(-delta), fmt)
	elif delta > 0:
		add(view, "+" + _number(delta), FMT_EXP)   # a heal: the green gain style stands in [tự chọn: ShowHpChg has no heal branch here]


func skill_name(view: Node3D, name: String, series: int) -> void:
	add(view, name, FMT_SKILL_BASE + series if series >= 0 and series <= 4 else FMT_SKILL_GENERIC)


func miss(view: Node3D) -> void:
	add(view, "", FMT_MISS)


static func _number(v: int) -> String:
	# the "N0" format of ShowHpChg: thousands separated
	var s := str(v)
	var out := ""
	var n := 0
	for i in range(s.length() - 1, -1, -1):
		out = s[i] + out
		n += 1
		if n % 3 == 0 and i > 0:
			out = "," + out
	return out


static func _eval(curve: Array, t: float, dflt: float) -> float:
	if curve.is_empty():
		return dflt
	if t <= float(curve[0][0]):
		return float(curve[0][1])
	for i in range(1, curve.size()):
		if t <= float(curve[i][0]):
			var t0 := float(curve[i - 1][0])
			var t1 := float(curve[i][0])
			var f := 0.0 if t1 <= t0 else (t - t0) / (t1 - t0)
			return lerpf(float(curve[i - 1][1]), float(curve[i][1]), f)
	return float(curve[curve.size() - 1][1])


static func _end_time(f: Dictionary) -> float:
	var t := 0.0
	for key in ["alpha", "y", "x", "scale"]:
		var c: Array = f.get(key, [])
		if not c.is_empty():
			t = maxf(t, float(c[c.size() - 1][0]))
	return maxf(t, 0.5)


func tick(delta: float) -> void:
	var i := 0
	while i < _items.size():
		var it: Dictionary = _items[i]
		it["t"] = float(it["t"]) + delta
		var f: Dictionary = formats[int(it["fmt"])]
		if float(it["t"]) >= _end_time(f) or not is_instance_valid(it["node"]):
			_items.remove_at(i)
		else:
			i += 1


# Drawn on `ctrl` (the name layer): `head_of` gives the screen point above an entity's head, or Vector2.INF when off screen
func draw(ctrl: Control, cam: Camera3D, font: Font) -> void:
	for it in _items:
		var view: Node3D = it["node"]
		if not is_instance_valid(view):
			continue
		var top: Vector3 = view.global_position + Vector3(0, float(view.get("bar_height")) if view.get("bar_height") != null else 2.0, 0)
		if cam.is_position_behind(top):
			continue
		var sp := cam.unproject_position(top)
		var f: Dictionary = formats[int(it["fmt"])]
		var t := float(it["t"])
		var off: Array = f.get("offset", [0, 0, 0])
		var x := float(off[0]) + _eval(f.get("x", []), t, 0.0)
		var y := float(off[1]) + _eval(f.get("y", []), t, 0.0)
		var a := clampf(_eval(f.get("alpha", []), t, 1.0), 0.0, 1.0)
		var scale := maxf(0.05, _eval(f.get("scale", []), t, 1.0))
		var size := int(round(float(f.get("size", 22)) * scale))
		var text := str(it["text"])
		var col: Array = f.get("top", [1, 1, 1, 1]) if bool(f.get("gradient", false)) else f.get("color", [1, 1, 1, 1])
		var bot: Array = f.get("bottom", col)
		var color := Color(col[0], col[1], col[2], float(col[3]) * a)
		if bool(f.get("gradient", false)):
			color = Color((float(col[0]) + float(bot[0])) * 0.5, (float(col[1]) + float(bot[1])) * 0.5, (float(col[2]) + float(bot[2])) * 0.5, a)
		var ec: Array = f.get("effect_color", [0, 0, 0, 0.8])
		var ecol := Color(ec[0], ec[1], ec[2], float(ec[3]) * a)
		var w := font.get_string_size(text, HORIZONTAL_ALIGNMENT_CENTER, -1, size).x
		var at := Vector2(sp.x + x - w * 0.5, sp.y - y)
		match int(f.get("effect", 0)):
			2:
				ctrl.draw_string_outline(font, at, text, HORIZONTAL_ALIGNMENT_LEFT, -1, size, maxi(2, size / 8), ecol)
			1:
				ctrl.draw_string(font, at + Vector2(1, 1), text, HORIZONTAL_ALIGNMENT_LEFT, -1, size, ecol)
		ctrl.draw_string(font, at, text, HORIZONTAL_ALIGNMENT_LEFT, -1, size, color)
