# KText - the control tags of the old client's text (Engine\Src\Text.cpp, TEncodeText).
#
# A string of the game may carry  <color=Gold> ... <color>   text colour, and back to the window's
#                                 <bclr=Black> ... <bclr>    outline colour, and back
#                                 <enter>                    a line break
# A colour is a name of the engine's table or 0xRRGGBB.  The table below is the one inside
# enginefree.dll of the VLTK 2.0 client (file offset 0x1A8660, 21 rows of char[8] + R,G,B): it has
# Gold, Orange, Violet... which the JX1 source does not know, and the 2.0 data uses them.
extends RefCounted

const COLORS := {
	"black": Color8(0, 0, 0), "white": Color8(255, 255, 255), "red": Color8(255, 0, 0),
	"green": Color8(0, 255, 0), "dgreen": Color8(0, 127, 0), "blue": Color8(100, 100, 255),
	"yellow": Color8(255, 255, 0), "dyellow": Color8(127, 127, 0), "gold": Color8(243, 194, 90),
	"orange": Color8(255, 199, 0), "pink": Color8(255, 0, 255), "cyan": Color8(0, 255, 255),
	"metal": Color8(246, 255, 117), "wood": Color8(0, 255, 120), "water": Color8(78, 124, 255),
	"fire": Color8(255, 90, 0), "earth": Color8(254, 207, 179), "dblue": Color8(120, 120, 120),
	"hblue": Color8(100, 100, 255), "violet": Color8(188, 64, 255), "dviolet": Color8(111, 40, 156),
}


# "255,253,122" -> Color, the way GetColor of the old UI read a colour key.  `def` when the value
# is missing or malformed.
static func color_of(value: String, def: Color) -> Color:
	var parts := value.split(",")
	if parts.size() < 3:
		return def
	return Color8(clampi(parts[0].to_int(), 0, 255), clampi(parts[1].to_int(), 0, 255), clampi(parts[2].to_int(), 0, 255))


static func named_color(name: String, def: Color) -> Color:
	var key := name.strip_edges().to_lower()
	if key.begins_with("0x"):
		var v := key.hex_to_int()
		return Color8((v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff)
	return COLORS.get(key, def)


# Splits a tagged string into runs.  Each run is {"text": String, "color": Color or null,
# "border": Color or null, "br": bool}; a run with br = true ends its line.  null means "the
# window's own colour" (KTC_COLOR_RESTORE / KTC_BORDER_RESTORE).
static func parse(source: String) -> Array:
	var runs: Array = []
	var color = null
	var border = null
	var text := ""
	var i := 0
	var n := source.length()
	while i < n:
		var ch := source[i]
		if ch == "\n":
			runs.append({"text": text, "color": color, "border": border, "br": true})
			text = ""
			i += 1
			continue
		if ch != "<":
			text += ch
			i += 1
			continue
		var end := source.find(">", i)
		if end < 0 or end - i > 24:
			text += ch
			i += 1
			continue
		var tag := source.substr(i + 1, end - i - 1)
		var name := tag
		var arg := ""
		var eq := tag.find("=")
		if eq >= 0:
			name = tag.substr(0, eq)
			arg = tag.substr(eq + 1)
		name = name.strip_edges().to_lower()
		if name != "color" and name != "bclr" and name != "enter":
			# not a tag of ours (a "<" the text really contains): keep it
			text += ch
			i += 1
			continue
		if text != "" or name == "enter":
			runs.append({"text": text, "color": color, "border": border, "br": name == "enter"})
			text = ""
		if name == "color":
			color = null if arg == "" else named_color(arg, Color.WHITE)
		elif name == "bclr":
			border = null if arg == "" else named_color(arg, Color.BLACK)
		i = end + 1
	if text != "":
		runs.append({"text": text, "color": color, "border": border, "br": false})
	return runs


# The text without its tags.
static func plain(source: String) -> String:
	var out := ""
	for r in parse(source):
		out += str(r["text"])
		if r["br"]:
			out += "\n"
	return out
