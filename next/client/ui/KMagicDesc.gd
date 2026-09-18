# KMagicDesc - the sentence of an item attribute (Core\Src\KMagicDesc.cpp: GetDesc).
#
# \settings\magicdesc.ini [Descript] holds one line per attribute NAME, with holes:
#   #d1  the first value as a number      #d1+  with its sign shown      #d1~  sign turned around
#   #f1  the same as a decimal            #x1   Nam / Nữ                 #s1   the element (Kim..Thổ)
#   #m1  the faction                      #k1   what a skill costs       #l1   a skill's name
# The name of an attribute id is MAGIC_ATTRIB_STRING of the JX2 build (ui/du-lieu/ten-ma-phap.json);
# the lines are ui/du-lieu/mo-ta-ma-phap.json, both written by jxassets export-ui.
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
	var n = Assets.ui_data("ten-ma-phap")
	if n is Dictionary:
		_names = n.get("names", {})
	var t = Assets.ui_data("mo-ta-ma-phap")
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


# The sentence for {type, value: [v1, v2, v3]}; "" when the table has no line for it.
static func describe(attrib: Dictionary) -> String:
	_load()
	var key := name_of(int(attrib.get("type", 0)))
	if key == "":
		return ""
	var line := str(_lines.get(key.to_lower(), ""))
	if line == "":
		return ""
	var values: Array = attrib.get("value", [])
	var out := ""
	var i := 0
	while i < line.length():
		var c := line[i]
		if c != "#" or i + 3 >= line.length():
			out += c
			i += 1
			continue
		var kind := line[i + 1]
		var which := line[i + 2]
		var mode := line[i + 3]     # '+', '~' or anything else
		var v := 0
		match which:
			"1":
				v = int(values[0]) if values.size() > 0 else 0
			"2":
				v = int(values[1]) if values.size() > 1 else 0
			"3":
				v = int(values[2]) if values.size() > 2 else 0
			"7":
				v = (int(values[0]) if values.size() > 0 else 0) % 256
			"9":
				v = (int(values[2]) if values.size() > 2 else 0) % 256
			_:
				v = int(values[0]) if values.size() > 0 else 0
		match kind:
			"d":
				out += _signed(v, mode)
			"f":
				var f := float(v)
				if which == "6":
					f = float((int(values[2]) if values.size() > 2 else 0) / 256) / 18.0
				out += _signed_f(f, mode)
			"x":
				out += "Nữ" if v != 0 else "Nam"
			"s":
				out += SERIES[v] if v >= 0 and v < SERIES.size() else "Vô hệ"
			"k":
				out += COST_KIND[v] if v >= 0 and v < COST_KIND.size() else COST_KIND[0]
			"m":
				out += str(v)                 # the faction's name (g_Faction) comes with M12
			"l":
				out += ("[ kỹ năng %d ]" % v) if v > 0 else "Võ công vốn có"
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
