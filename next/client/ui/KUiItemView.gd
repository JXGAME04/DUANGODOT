# KUiItemView - what a window needs to show one item of Game.items: the container object (cells,
# picture, shade), whether this character can wear it, and its description text.
#
# The description is KItem::GetDesc of the 2.0 client (gamecl.exe unpacked, 0x00636460, read line
# by line - docs/CLIENT-2.0.md) rebuilt exactly: the same string-table lines (G_ITEM_* of
# \lang\vn\stringtable_core.txt), the same colour tags, the same order, the same "\n"s.  The
# text is one rich string; UiMouseHover wraps and centres it the way KMouseOver drew it.
extends RefCounted

const KMagicDesc := preload("res://ui/KMagicDesc.gd")
const KTextEncode := preload("res://ui/KTextEncode.gd")
const KMagicRange := preload("res://ui/KMagicRange.gd")
const KLibOfBPT := preload("res://ui/KLibOfBPT.gd")

# ITEMGENRE of KItem.h
const GENRE_EQUIP := 0
const GENRE_MEDICINE := 1
const GENRE_TASK := 4
const GENRE_TOWN_PORTAL := 5
const GENRE_SCRIPT := 6
const GENRE_BROKEN := 7
const DETAIL_MASK := 11         # equip_mask: no element line, no prefixes / suffixes
const EQUIP_HORSE := 10         # equip_horse: from the horse on every suffix is always active
# the attribute ids this window reads by number (MAGIC_ATTRIB)
const MAGIC_DURABILITY := 31
const REQUIRE_STR := 32
const REQUIRE_DEX := 33
const REQUIRE_VIT := 34
const REQUIRE_ENG := 35
const REQUIRE_LEVEL := 36
const REQUIRE_SERIES := 37
const REQUIRE_SEX := 38
const REQUIRE_MENPAI := 39
# g_StrWrap(buf, intro, 0x28): the description is wrapped at 40 characters
const INTRO_WRAP := 40
# the name colours by kind, kept for the windows that paint names (the tooltip has its own rule)
const NAME_COLORS := [Color.WHITE, Color8(255, 255, 0), Color8(188, 64, 255), Color8(255, 255, 255)]
const NAME_COLOR_MAGIC := Color8(100, 100, 255)

# The five elements feeding each other (g_nAccrueSeries: metal -> water -> wood -> fire -> earth ->
# metal; jx_linux_y 0x0830ED18) and ms_ActivedEquip (0x082E7460): the two worn parts whose element
# can wake the suffixes of each part - KItemList::GetEquipEnhance, the nActive of a worn piece
const ACCRUE := [2, 3, 1, 4, 0]
const ACTIVED_EQUIP := [[1, 6], [8, 2], [9, 5], [6, 1], [3, 0], [4, 7], [2, 8], [3, 0], [5, 9], [4, 7]]
const ROOM_BODY := 10

# the name colour tags by genre (the table at 0x0081b398 of the client: genre 1 White, 4 Yellow,
# the rest empty - the text keeps the window's colour; genre 0 is decided by the piece itself)
const NAME_TAG_BY_GENRE := {1: "<color=White>", 4: "<color=Yellow>"}

static var _strings: Dictionary = {}
static var _strings_loaded := false


# The object a KWndObjContainer draws for an item (at the item's own cells); the picture and the
# name are the client's own table's (KLibOfBPT) when it has the row.
static func object_of(raw: Dictionary) -> Dictionary:
	var item := KLibOfBPT.display(raw)
	return {
		"id": int(item.id), "x": int(item.x), "y": int(item.y), "w": maxi(1, int(item.w)), "h": maxi(1, int(item.h)),
		"image": Assets.item_image(str(item.get("image", ""))), "ex_type": int(item.get("ex_type", 0)),
		"usable": usable(item), "name": str(item.get("name", "")), "count": int(item.get("count", 1)),
	}


# A line of \lang\vn\stringtable_core.txt (ui/du-lieu/chuoi-core.json) by key; "" when missing.
static func core_string(key: String) -> String:
	if not _strings_loaded:
		_strings_loaded = true
		var t = Assets.ui_data("chuoi-core")
		if t is Dictionary:
			_strings = t.get("strings", {})
	return str(_strings.get(key, ""))


static func forget() -> void:
	_strings_loaded = false
	_strings = {}
	_client_strings_loaded = false
	_client_strings = {}


static var _client_strings: Dictionary = {}
static var _client_strings_loaded := false


# A line of lang/vn/stringtable_client.txt (ui/du-lieu/chuoi-client.json) by key: the words of the windows
# (G_ACCEPT_WORD "Đồng ý", G_REFUSE_WORD "Từ chối", G_SysMsgCentre_0 "%s mời bạn vào đội", ...); "" when missing.
static func client_string(key: String) -> String:
	if not _client_strings_loaded:
		_client_strings_loaded = true
		var t = Assets.ui_data("chuoi-client")
		if t is Dictionary:
			_client_strings = t.get("strings", {})
	return str(_client_strings.get(key, ""))


# One requirement against this character (KItemList::EnoughAttrib as the client knows it: level,
# series, sex; strength and the rest come with M12 and read as enough)
static func enough(req: Dictionary) -> bool:
	var me = Game.entities.get(Game.entity_id)
	if me == null:
		return true
	var v: Array = req.get("value", [])
	var need := int(v[0]) if v.size() > 0 else 0
	match int(req.get("type", 0)):
		REQUIRE_LEVEL:
			return int(me.get("level", 1)) >= need
		REQUIRE_SERIES:
			return int(me.get("series", 0)) == need
		REQUIRE_SEX:
			return int(me.get("sex", 0)) == need
	return true


# KItemList::EnoughAttrib for every requirement
static func usable(item: Dictionary) -> bool:
	if int(item.get("genre", 0)) != GENRE_EQUIP:
		return true
	for r in item.get("require", []):
		if int(r.get("type", 0)) > 0 and not enough(r):
			return false
	return true


# The colour tag KItem::GetDesc opens the name with (0x0063657e..0x006366fe)
static func name_tag(item: Dictionary) -> String:
	var genre := int(item.get("genre", 0))
	var quality := int(item.get("ex_type", 0))
	var magic: Array = item.get("magic", [])
	if genre == GENRE_EQUIP:
		if quality == 1 or quality == 4 or quality == 5:
			return "<color=Yellow>"
		if quality == 2:
			return "<color=Violet>"
		if quality == 3:
			return core_string("G_ITEM_0")   # "<color=White> vật phẩm tạm thời: "
		if magic.size() > 0 and int(magic[0].get("type", 0)) != 0:
			return "<color=Blue>"
		return "<color=White>"
	if genre == GENRE_BROKEN or int(item.get("durability", -1)) == 0:
		return "<color=Red>"
	return str(NAME_TAG_BY_GENRE.get(genre, ""))


# The colour the name is painted in (for windows that show only the name)
static func name_color(item: Dictionary) -> Color:
	var runs := KTextEncode.runs_of(name_tag(item) + "x", Color.WHITE)
	return runs.back().color


# The first line: the name and " [Cấp N]" (G_ITEM_22) on equipment, as GetDesc writes it
static func title_of(raw: Dictionary) -> String:
	var item := KLibOfBPT.display(raw)
	var genre := int(item.get("genre", 0))
	var name := str(item.get("name", ""))
	if genre == GENRE_BROKEN or int(item.get("durability", -1)) == 0:
		name = core_string("G_ITEM_28") + name   # "<trang bị tổn hại>"
	if genre == GENRE_EQUIP:
		name += core_string("G_ITEM_22") % int(item.get("level", 0))
	return name


# KItemList::GetEquipEnhance for a piece of Game.items: 0 unless it is worn; then one when the
# character's element feeds the piece's, one more for each activating part worn with an element
# that feeds it; the horse and the parts after it (10..) always 3.  (CoreShell passes this as
# nActive for the equipment window, 0 for the bag.)
static func equip_enhance(item: Dictionary) -> int:
	if int(item.get("room", 0)) != ROOM_BODY:
		return 0
	var part := int(item.get("x", 0))
	if part >= EQUIP_HORSE:
		return 3
	var me = Game.entities.get(Game.entity_id)
	var series := int(item.get("series", -1))
	var n := 0
	if me != null and _accrues(int(me.get("series", -1)), series):
		n += 1
	if part >= 0 and part < ACTIVED_EQUIP.size():
		for other in ACTIVED_EQUIP[part]:
			var worn_id: int = Game.item_worn(int(other))
			if worn_id != 0 and _accrues(int(Game.items[worn_id].get("series", -1)), series):
				n += 1
	return n


static func _accrues(src: int, des: int) -> bool:
	return src >= 0 and src < ACCRUE.size() and int(ACCRUE[src]) == des


# The "[min-max]" the 2.0 client prints after a magic line: KLibOfBPT::GetMagicRange over the
# rows of that kind the piece could have drawn (position, type, series, level)
static func magic_range(item: Dictionary, slot: int, type: int) -> Array:
	return KMagicRange.range_of(int(item.get("version", 0)), type, (slot & 1) == 0, int(item.get("detail", 0)),
		int(item.get("series", 0)), int(item.get("level", 0)))


# KItem::GetDesc(buf, nUiType = 0, nPriceScale = 1, nActive, ...) of the 2.0 client for a piece in
# the bag: no price line, `active` suffixes lit (0 in the bag; a worn piece counts its element
# links, M12), the magic block unless the window is one of the shop / trade ones (19..28).
static func describe_text(raw: Dictionary, active: int = 0, ui_type: int = 0) -> String:
	var item := KLibOfBPT.display(raw)   # the client's own name and description (KItem copies its table row)
	var genre := int(item.get("genre", 0))
	var detail := int(item.get("detail", 0))
	var durability := int(item.get("durability", -1))
	var s := name_tag(item) + title_of(item) + "\n"
	# (bind / lock / stamp lines: the piece carries none of those states yet)
	if genre == GENRE_EQUIP and detail != DETAIL_MASK:
		var series := int(item.get("series", -1))
		if series >= 0 and series <= 4:
			s += core_string("G_ITEM_%d" % (3 + series))   # "<color=White>Thuộc tính Ngũ hành: <color=Metal>Kim "
		s += "\n"
	s += "<color=White>" + KTextEncode.str_wrap(str(item.get("intro", "")), INTRO_WRAP)
	if genre == GENRE_SCRIPT:
		return s   # a script item ends with its own description (0x00630cd0): with the script items
	else:
		# the seven base attributes (0x00630ae0)
		for a in item.get("base", []):
			var type := int(a.get("type", 0))
			if type <= 0:
				continue
			if type == MAGIC_DURABILITY:
				if genre != GENRE_EQUIP or detail == DETAIL_MASK:
					continue
				if durability == -1:
					s += core_string("G_ITEM_8")   # "<color=Yellow>Không thể phá hủy<color>"
				else:
					var key := "G_ITEM_9_1" if durability > 0 else "G_ITEM_9_2"
					s += core_string(key) % [durability, int(item.get("max_durability", 0))]
				s += "\n"
				continue
			var t := KMagicDesc.describe(a)
			if t == "":
				continue
			if durability == 0:
				s += "<color=red>%s<color>\n" % t
			else:
				s += t + "\n"
	# the requirements (0x0062ea70): white when met, red when not
	for r in item.get("require", []):
		if int(r.get("type", 0)) <= 0:
			continue
		var t := KMagicDesc.describe(r)
		if t == "":
			continue
		s += ("<color=White>" if enough(r) else "<color=Red>") + t + "\n"
	# the prefixes and suffixes (0x00635430)
	if ui_type < 19 or ui_type >= 29:
		s += magic_text(item, active)
	return s


# The magic block: slot i even = prefix, odd = suffix; a suffix is lit while (i >> 1) < active
# (every one on a mask); the colour by quality, the "[min-max]" range after the sentence for
# quality 0 / 3 / 5 pieces; an empty socket of a quality-2 piece says "Chưa khảm".
static func magic_text(item: Dictionary, active: int) -> String:
	var s := ""
	var genre := int(item.get("genre", 0))
	var quality := int(item.get("ex_type", 0))
	var detail := int(item.get("detail", 0))
	var durability := int(item.get("durability", -1))
	var magic: Array = item.get("magic", [])
	var levels: Array = item.get("magic_levels", [])
	if genre == GENRE_EQUIP and detail == DETAIL_MASK:
		active = 3
	for i in range(mini(6, magic.size())):
		var a: Dictionary = magic[i]
		var type := int(a.get("type", 0))
		if type <= 0:
			if quality == 2 and i < levels.size() and int(levels[i]) == -1:
				s += "<color=Yellow>" + core_string("G_ITEM_27") + "\n"   # "Chưa khảm"
			continue
		var t := KMagicDesc.describe(a)
		if t == "":
			continue
		var range := magic_range(item, i, type)
		var lit: bool = (i & 1) == 0 or (i >> 1) < active
		var tag := ""
		var range_text := ""
		if durability == 0:
			tag = "<color=Red>"
		elif quality == 1 or quality == 4:
			tag = "<color=Yellow>" if lit else "<color=DYellow>"
		elif quality == 2:
			tag = "<color=Violet>" if lit else "<color=DViolet>"
		elif quality == 5:
			# a roll at the top of its range is orange (0x0062f020); the rest yellow
			var value: Array = a.get("value", [0])
			var perfect: bool = range.size() == 2 and int(range[1]) > 0 and int(value[0]) >= int(range[1])
			range_text = ("<color=0xc0c0c0>[%d-%d]" if lit else "<color=DBlue>[%d-%d]") % range
			tag = ("<color=0xff8c27>" if perfect else "<color=Yellow>") if lit else ("<color=0xaa7f14>" if perfect else "<color=DYellow>")
		else:
			range_text = ("<color=0xc0c0c0>[%d-%d]" if lit else "<color=DBlue>[%d-%d]") % range
			tag = "<color=HBlue>" if lit else "<color=DBlue>"
		s += tag + t + range_text + "\n"
	return s


# The description as lines of coloured runs, for windows and tests: [{text, color, parts}]
static func describe(item: Dictionary, active: int = 0) -> Array:
	var lines: Array = []
	var text := describe_text(item, active)
	if text.ends_with("\n"):
		text = text.substr(0, text.length() - 1)
	for line in text.split("\n"):
		var runs := KTextEncode.runs_of(line, Color.WHITE)
		var plain := ""
		for r in runs:
			plain += str(r.text)
		lines.append({"text": plain, "color": runs[0].color, "parts": runs})
	return lines
