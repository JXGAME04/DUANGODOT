# KMagicDesc - the sentence of an item or skill attribute (Core\Src\KMagicDesc.cpp: GetDesc; gamecl.exe 2.0
# 0x0060A2B0 with the digit helper 0x00608C00 and the sign helper 0x00608BE0; docs/CLIENT-2.0.md §10).
#
# \settings\magicdesc.ini [Descript] holds one line per attribute NAME, with holes of four characters "#" letter digit sign:
#   #d1  the first value as a number      #d1+  with its sign shown      #d1~  sign turned around
#   #f1  the same as a decimal (#f6: value 3 / 256 / 18 - frames to seconds)
#   #x1  Nam / Nữ                          #s1   the element (Kim..Thổ)     #m1   the faction
#   #k1  what a skill costs                #l1   "[ a skill's name ]" (digit A: value 1 / 256), 0 = the own-skill words
#   digit 1..3 = value[0..2]; 4..6 = value >> 8; 7..9 = value & 0xff (the 2.0 helper 0x00608C00)
# The name of an attribute id is MAGIC_ATTRIB_STRING of the JX2 build (ui/du-lieu/ten-ma-phap.json);
# the lines are ui/du-lieu/mo-ta-ma-phap.json, both written by jxassets export-ui.  describe_line() is the pure
# grammar (no autoload: the headless tests and the skill tip use it), describe() the item tip's entry point.
extends RefCounted

const SERIES := ["Kim", "Mộc", "Thủy", "Hỏa", "Thổ"]
const COST_KIND := ["Nội lực", "Sinh lực", "Thể lực", "Tiền"]

static var _names: Dictionary = {}
static var _lines: Dictionary = {}
static var _loaded := false


static func _load() -> void:
	if _loaded:
		return
	_loaded = true
	# the Assets autoload looked up at run time, so this script also loads where no autoload exists (tests/run.gd)
	var tree := Engine.get_main_loop() as SceneTree
	var assets = tree.root.get_node_or_null("Assets") if tree != null and tree.root != null else null
	if assets == null:
		return
	var n = assets.ui_data("ten-ma-phap")
	if n is Dictionary:
		_names = n.get("names", {})
	var t = assets.ui_data("mo-ta-ma-phap")
	if t is Dictionary:
		for sec in t.get("sections", []):
			if str(sec.get("name", "")).to_lower() == "descript":
				_lines = sec.get("values", {})


static func forget() -> void:
	_loaded = false
	_names = {}
	_lines = {}


static func name_of(type: int) -> String:
	_load()
	return str(_names.get(str(type), ""))


# The line of [Descript] for an attribute name ("" when there is none)
static func line_of(name: String) -> String:
	_load()
	return str(_lines.get(name.to_lower(), ""))


# The sentence for {type, value: [v1, v2, v3]}; "" when the table has no line for it.
static func describe(attrib: Dictionary) -> String:
	_load()
	var key := name_of(int(attrib.get("type", 0)))
	if key == "":
		return ""
	var line := str(_lines.get(key.to_lower(), ""))
	if line == "":
		return ""
	return describe_line(line, attrib.get("value", []), {})


# the value a digit picks (0x00608C00): 1..3 the values, 4..6 shifted right by 8, 7..9 the low byte
static func value_of(values: Array, digit: String) -> int:
	var idx := digit.unicode_at(0) - 49 if digit.length() > 0 else -1   # "1" = 49
	if idx < 0 or idx > 8:
		return int(values[0]) if values.size() > 0 else 0
	var which := idx % 3
	var v := int(values[which]) if values.size() > which else 0
	if idx >= 6:
		return v & 0xff
	if idx >= 3:
		return v >> 8
	return v


# The grammar over one line.  ctx (all optional): "series" [5 names], "series_none", "cost_types" [4 names],
# "sex" [2 names], "factions" {index: name}, "skill_name" Callable(id) -> String, "own_skill" String.
static func describe_line(line: String, values: Array, ctx: Dictionary) -> String:
	var out := ""
	var i := 0
	var n := line.length()
	while i < n:
		var c := line[i]
		if c != "#" or i + 3 >= n:
			out += c
			i += 1
			continue
		var kind := line[i + 1]
		var which := line[i + 2]
		var mode := line[i + 3]     # '+', '~' or anything else
		var v := value_of(values, which)
		match kind:
			"d":
				out += _signed(v, mode)
			"f":
				var f := float(v)
				if which == "6":
					f = float((int(values[2]) if values.size() > 2 else 0) >> 8) / 18.0
				out += _signed_f(f, mode)
			"x":
				var sex: Array = ctx.get("sex", ["Nam", "Nữ"])
				out += str(sex[1] if v != 0 else sex[0])
			"s":
				var series: Array = ctx.get("series", SERIES)
				out += str(series[v]) if v >= 0 and v < series.size() else str(ctx.get("series_none", "Vô hệ"))
			"k":
				var kinds: Array = ctx.get("cost_types", COST_KIND)
				out += str(kinds[v]) if v >= 0 and v < kinds.size() else str(kinds[0])
			"m":
				out += str(ctx.get("factions", {}).get(v, str(v)))   # the faction's name (g_Faction) when the caller knows it
			"l":
				if mode == "+" or mode == "~":
					pass   # KMagicDesc.cpp: a sign on 'l' prints nothing
				else:
					var id := v
					if which == "A" and values.size() > 0:
						id = int(values[0]) >> 8
					if id > 0:
						var name_of = ctx.get("skill_name")
						out += "[ " + (str(name_of.call(id)) if name_of is Callable else ("kỹ năng %d" % id)) + " ]"
					else:
						out += str(ctx.get("own_skill", "Võ công vốn có"))
			_:
				pass
		i += 4
	return out


static func _signed(v: int, mode: String) -> String:
	match mode:
		"+":
			return ("+%d" % v) if v >= 0 else str(v)
		"~":
			return ("-%d" % v) if v >= 0 else ("+%d" % -v)
	return str(v)


static func _signed_f(f: float, mode: String) -> String:
	match mode:
		"+":
			return ("+%.2f" % f) if f >= 0.0 else ("%.2f" % f)
		"~":
			return ("-%.2f" % f) if f >= 0.0 else ("+%.2f" % -f)
	return "%.2f" % f
