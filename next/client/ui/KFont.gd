# KFont - the old client's bitmap fonts (Represent\iRepresent\Font\KFont2.cpp).
#
# jxassets export-ui writes every font of the theme as TWO BMFont files with identical metrics:
# assets/ui/font/chu-14.fnt holds the letters, chu-14-vien.fnt the outline that is baked into each
# glyph.  The old blitter (Engine\Src\KDrawFont.cpp, g_DrawFontWithBorder) painted the letter pixels
# in the text colour and the outline pixels in the border colour, so a string is drawn twice here:
# outline first, letters on top.
#
# The pitch is the old one: every single-byte character advances half a cell (7 px for Font=14),
# whatever the letter - the exporter writes that into xadvance.  Text is measured the way the old
# windows did it, `length * size / 2`, never by asking the font.
#
# A character the game font does not have (a capital with a tone mark, anything outside TCVN3)
# falls back to the engine's default font so nothing the player types disappears.
extends RefCounted

const FONT_DIR := "/ui/font/"

static var _cache: Dictionary = {}      # size -> KFont (null when that size is not exported)
static var _index: Dictionary = {}      # size -> entry of font.json
static var _index_loaded := false

var size := 12                 # the number layouts use in Font=
var cell := Vector2i(12, 12)   # the box one glyph is drawn in
var advance := 6               # pixels per single-byte character
var letters: FontFile = null
var outline: FontFile = null


# The font for Font=<size>.  Sizes that share the glyphs of another ("13 -> 12" in the 2.0 client)
# resolve to that one; an unknown size takes the nearest smaller one.  Null when no font has been
# exported at all - callers then draw with the default font.
static func of(font_size: int):
	if _cache.has(font_size):
		return _cache[font_size]
	_load_index()
	var f = _open(font_size)
	_cache[font_size] = f
	return f


static func _load_index() -> void:
	if _index_loaded:
		return
	_index_loaded = true
	var path: String = Assets.assets_root() + FONT_DIR + "font.json"
	if not FileAccess.file_exists(path):
		Log.warn("ui", "game fonts not exported, text falls back to the default font", {"path": path})
		return
	var data = Assets.load_json(path)
	if data == null:
		return
	for entry in data.get("fonts", []):
		_index[int(entry.get("size", 0))] = entry


static func _open(font_size: int):
	if _index.is_empty():
		return null
	var want := font_size
	if not _index.has(want):
		var best := -1
		for s in _index.keys():
			if int(s) <= font_size and int(s) > best:
				best = int(s)
		if best < 0:
			for s in _index.keys():
				if best < 0 or int(s) < best:
					best = int(s)
		want = best
	var entry: Dictionary = _index[want]
	var same := int(entry.get("same_as", 0))
	if same > 0 and _index.has(same):
		entry = _index[same]
	if str(entry.get("letters", "")) == "":
		return null
	var script: GDScript = load("res://ui/KFont.gd")
	var f = script.new()
	f.size = int(entry.get("size", font_size))
	f.cell = Vector2i(int(entry.get("cell_w", f.size)), int(entry.get("cell_h", f.size)))
	f.advance = int(entry.get("advance", f.size / 2))
	var dir: String = Assets.assets_root() + FONT_DIR
	f.letters = _load_bmfont(dir + str(entry.get("letters", "")))
	f.outline = _load_bmfont(dir + str(entry.get("outline", "")))
	if f.letters == null:
		return null
	Log.debug("ui", "game font loaded", {"font": font_size, "glyphs_of": f.size, "cell": "%dx%d" % [f.cell.x, f.cell.y], "advance": f.advance})
	return f


static func _load_bmfont(path: String) -> FontFile:
	if not FileAccess.file_exists(path):
		return null
	var font := FontFile.new()
	var err := font.load_bitmap_font(path)
	if err != OK:
		Log.error("ui", "bitmap font load failed", {"path": path, "error": error_string(err)})
		return null
	return font


# Drops the cache (tests, or after a new export while the client runs).
static func forget() -> void:
	_cache.clear()
	_index.clear()
	_index_loaded = false


# Width of a run of text the way the old windows measured it: bytes * size / 2.  One Unicode
# character is one TCVN3 byte.
func width_of(text: String) -> int:
	return text.length() * advance


# Draws `text` with its top-left corner at `pos` (the old OutputText took the corner of the first
# cell, not a base line).
func draw(ci: CanvasItem, pos: Vector2, text: String, color: Color, border: Color) -> void:
	if text == "":
		return
	var base := Vector2(pos.x, pos.y + letters.get_ascent(size))
	if outline != null and border.a > 0.0:
		ci.draw_string(outline, base, text, HORIZONTAL_ALIGNMENT_LEFT, -1, size, border)
	ci.draw_string(letters, base, text, HORIZONTAL_ALIGNMENT_LEFT, -1, size, color)
