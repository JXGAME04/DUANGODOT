# KTextEncode - the rich text of the old engine (Engine\Src\Text.cpp: TEncodeText), as the 2.0
# client's engineFree.dll has it.  A description is one string with "\n" line breaks and tags:
#   <color=Name>   the named colours of the engine's table (dumped from enginefree.dll, below)
#   <color=0xRRGGBB>  a hex colour (strtoul: R = >> 16, G = >> 8, B)
#   <color>        back to the colour the text started with (KTC_COLOR_RESTORE)
#   <bclr=Name> / <bclr>   the outline colour (KTC_BORDER_COLOR); kept, not drawn yet
#   <enter>        a line break the tables write (g_StrWrap splits on it)
# A tag the engine does not know ("<trang bị tổn hại>", "<color=14579391>") stays visible text.
#
# Widths are the old ones: TGetEncodedTextLineCount counts every byte of TCVN3 text as one
# half-width character (m_bTCVN), so a line's width is its number of visible characters.
extends RefCounted

# s_ColorTable of the 2.0 client's engine (enginefree.dll): name[8] + r, g, b
const COLORS := {
	"black": Color8(0, 0, 0), "white": Color8(255, 255, 255), "red": Color8(255, 0, 0), "green": Color8(0, 255, 0),
	"dgreen": Color8(0, 127, 0), "blue": Color8(100, 100, 255), "yellow": Color8(255, 255, 0), "dyellow": Color8(127, 127, 0),
	"gold": Color8(243, 194, 90), "orange": Color8(255, 199, 0), "pink": Color8(255, 0, 255), "cyan": Color8(0, 255, 255),
	"metal": Color8(246, 255, 117), "wood": Color8(0, 255, 120), "water": Color8(78, 124, 255), "fire": Color8(255, 90, 0),
	"earth": Color8(254, 207, 179), "dblue": Color8(120, 120, 120), "hblue": Color8(100, 100, 255),
	"violet": Color8(188, 64, 255), "dviolet": Color8(111, 40, 156),
}


# The colour a <color=...> parameter names, or null when the engine would not know it.
static func color_of(param: String):
	var p := param.strip_edges()
	if p.begins_with("0x") or p.begins_with("0X"):
		var v := p.substr(2).hex_to_int()
		return Color8((v >> 16) & 0xff, (v >> 8) & 0xff, v & 0xff)
	return COLORS.get(p.to_lower(), null)


# One line of rich text -> [{text, color}] runs, the way TEncodeText + the renderer see it.
# `base` is the colour the line starts with and <color> returns to.
static func runs_of(line: String, base: Color) -> Array:
	var out: Array = []
	var color := base
	var cur := ""
	var i := 0
	while i < line.length():
		var c := line[i]
		if c == "<":
			var close := line.find(">", i)
			if close > i:
				var body := line.substr(i + 1, close - i - 1)
				var name := body
				var param := ""
				var eq := body.find("=")
				if eq >= 0:
					name = body.substr(0, eq)
					param = body.substr(eq + 1)
				var lname := name.strip_edges().to_lower()
				if lname == "color":
					if cur != "":
						out.append({"text": cur, "color": color})
						cur = ""
					if param == "":
						color = base
					else:
						var col = color_of(param)
						if col != null:
							color = col
					i = close + 1
					continue
				elif lname == "bclr" or lname == "enter":
					i = close + 1   # the outline colour is not drawn yet; <enter> is split by the caller
					continue
		cur += c
		i += 1
	if cur != "" or out.is_empty():
		out.append({"text": cur, "color": color})
	return out


# The visible characters of a rich line (tags dropped): what the old counting called its length
static func visible_length(line: String) -> int:
	var n := 0
	for r in runs_of(line, Color.WHITE):
		n += str(r.text).length()
	return n


# g_StrWrap(dst, src, width) of engineFree.dll (0x10035fc0), which KItem::GetDesc uses for the
# description text: `src` is cut at "<enter>"; a piece whose visible width fits `width` is
# appended as it is, a longer one is spread evenly over width / n + 1 lines - the break falls
# after `per` = width_of_piece / lines characters, in the middle of a word if that is where it
# lands.  Every line ends with "\n".  The engine counts a byte above 0x80 as a double-byte
# character and steps two bytes, so in TCVN3 text an accented letter and the letter after it
# always travel together: that pair counts 2 here too.
static func str_wrap(src: String, width: int) -> String:
	var out := ""
	for piece in src.split("<enter>"):
		var units: Array = []   # [text, count]
		var i := 0
		while i < piece.length():
			var c: String = piece[i]
			if c == "<":
				var close := piece.find(">", i)
				if close > i:
					units.append([piece.substr(i, close - i + 1), 0])
					i = close + 1
					continue
			if c.unicode_at(0) >= 0x80 and i + 1 < piece.length():
				units.append([piece.substr(i, 2), 2])
				i += 2
			else:
				units.append([c, 2 if c.unicode_at(0) >= 0x80 else 1])
				i += 1
		var n := 0
		for u in units:
			n += u[1]
		if n <= width:
			out += piece + "\n"
			continue
		var lines: int = n / width + 1
		var per: int = n / lines
		var acc := 0
		for u in units:
			acc += u[1]
			if acc > per:
				out += "\n"
				acc = 0
			out += u[0]
		out += "\n"
	return out
