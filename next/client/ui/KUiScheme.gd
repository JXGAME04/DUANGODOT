# KUiScheme - one layout file of the old client, read the way KIniFile was.
#
# Every window of the old client loads an .ini and hands its sections to its parts:
#     Ini.Load("<theme>\UiNewLogin\选服务器.ini");  m_List.Init(&Ini, "LeftList");  ...
# jxassets export-ui transcribed those files WHOLE into assets/ui/<man>/bo-cuc.json - every key of
# every section, decoded to UTF-8, plus the pictures the values name.  This class is the KIniFile of
# the new client: get_integer / get_string / get_color with the old defaults-when-missing, so a
# window's code reads like the LoadScheme it was ported from.
extends RefCounted

const KUiImage := preload("res://ui/KUiImage.gd")
const KText := preload("res://ui/KText.gd")

const SERIES := ["metal", "wood", "water", "fire", "earth"]
const SEX := ["male", "female"]

var name := ""                 # folder below assets/ui
var data: Dictionary = {}
var _sections: Dictionary = {} # lower-cased section name -> its dictionary
var _images: Dictionary = {}   # "section/key" -> KUiImage


# Opens assets/ui/<screen_name>/bo-cuc.json; null when it has not been exported.
static func open(screen_name: String):
	var raw = Assets.ui_screen(screen_name)
	if raw == null or not (raw is Dictionary):
		return null
	var script: GDScript = load("res://ui/KUiScheme.gd")
	var s = script.new()
	s.name = screen_name
	s.data = raw
	for w in raw.get("widgets", []):
		s._sections[str(w.get("key", ""))] = w
	return s


# The screen the theme was drawn for (1024x768 for ui3_1024).
func screen_size() -> Vector2i:
	return Vector2i(int(data.get("width", 800)), int(data.get("height", 600)))


func has_section(section: String) -> bool:
	return _sections.has(section.to_lower())


func section_names() -> Array:
	var out: Array = []
	for w in data.get("widgets", []):
		out.append(str(w.get("name", "")))
	return out


func has_key(section: String, key: String) -> bool:
	return _values(section).has(key.to_lower())


func get_string(section: String, key: String, def: String = "") -> String:
	return str(_values(section).get(key.to_lower(), def))


# KIniFile::GetInteger: the leading number of the value, `def` when there is no such key.
func get_integer(section: String, key: String, def: int = 0) -> int:
	var v = _values(section).get(key.to_lower(), null)
	if v == null:
		return def
	return leading_int(str(v), def)


# KIniFile::GetInteger2: "a,b".
func get_integer2(section: String, key: String, def: Vector2i = Vector2i.ZERO) -> Vector2i:
	var v = _values(section).get(key.to_lower(), null)
	if v == null:
		return def
	var parts := str(v).split(",")
	if parts.size() < 2:
		return Vector2i(leading_int(parts[0], def.x), def.y)
	return Vector2i(leading_int(parts[0], def.x), leading_int(parts[1], def.y))


func get_bool(section: String, key: String, def: bool = false) -> bool:
	return get_integer(section, key, 1 if def else 0) != 0


# "r,g,b"; `def` when the key is missing or empty.
func get_color(section: String, key: String, def: Color) -> Color:
	var v := get_string(section, key, "")
	if v == "":
		return def
	return KText.color_of(v, def)


# The picture a key of a section names (Image=, SprImg=...), null when there is none.
func image(section: String, key: String = "image"):
	var id := section.to_lower() + "/" + key.to_lower()
	if _images.has(id):
		return _images[id]
	var entry = _sections.get(section.to_lower(), {}).get("images", {}).get(key.to_lower(), null)
	var img = KUiImage.from_entry(entry)
	_images[id] = img
	return img


# A character figure the select / create windows build by name
# (KUiSelPlayer::GetRoleImageName: "<PlayerImgPrefix>_<series>_<sex>_<n>.spr"): n = 0 the still
# figure, 1 the one that moves when picked, 2 the one that steps back.
func portrait(series: int, sex: int, n: int):
	var key := "%s_%s_%d" % [SERIES[clampi(series, 0, 4)], SEX[clampi(sex, 0, 1)], clampi(n, 0, 2)]
	var id := "portrait/" + key
	if _images.has(id):
		return _images[id]
	var img = KUiImage.from_entry(data.get("portraits", {}).get(key, null))
	_images[id] = img
	return img


func _values(section: String) -> Dictionary:
	return _sections.get(section.to_lower(), {}).get("values", {})


# atoi: optional sign, digits, the rest ignored ("5000 ;ms" is 5000, "" is def).
static func leading_int(text: String, def: int = 0) -> int:
	var t := text.strip_edges()
	var end := 0
	while end < t.length() and (t[end] == "-" or t[end] == "+" or (t[end] >= "0" and t[end] <= "9")):
		end += 1
	if end == 0:
		return def
	var head := t.substr(0, end)
	if not head.is_valid_int():
		return def
	return head.to_int()
