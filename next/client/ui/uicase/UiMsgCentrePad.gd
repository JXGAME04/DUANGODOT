# UiMsgCentrePad - the chat channels of the VLTK 2.0 client (KUiMsgCentrePad; \ui\ui3_1024\消息集合面板_左.ini loaded by
# gamecl.exe 0x004B5E64 -> 0x004B4A30 / 0x004B50D0..; the 2004 Ui/UiCase/UiMsgCentrePad.cpp names the pieces), read line
# by line - docs/CLIENT-2.0.md §23:
#   - [Channels] Channel0..14 name the sections in the order the zone's Proto.ChatChannel keeps (0 CH_NEARBY, 1 CH_TEAM,
#     2 CH_WORLD, 3 CH_FACTION, 4 CH_SYSTEM, 5 CH_CITY, 6 CH_TONG, ...); DefaultChannel / DefaultChannelSendName;
#   - each [CH_*]: ShortName0.. (the "&<short> text" prefix of the input line, KUiPlayerBar::SendChat 0x00475A10 -> 0x00473030
#     finds the channel by it), FormatName (the relay's channel name; \F<Faction#> etc.), TextColor / TextBorderColor (the
#     line's colour), MenuText (the channel menu 0x00472620, coloured by 0x004B6530), TextImage (the little picture before a
#     line), SendMsgInterval / SendMsgNum (at most SendMsgNum lines every SendMsgInterval ms), Sound;
#   - [Main] NameTextColor / NameTextBorderColor: the speaker's name; [MSNRoom] TextColorSelf / Unknown / Friend: a whisper
#     by whom it comes from (KUiMsgCentrePad::MSNMessageArrival 2004: self / friend / stranger).
# A received line is composed like KUiMsgCentrePad::ChannelMessageArrival (2004): [picture] name: text - the name in
# NameTextColor, the text in the channel's TextColor.  The pad's own window (tabs, the message list) is not built yet: the
# game scene's chat log shows the lines.
extends RefCounted

const KText := preload("res://ui/KText.gd")

const SCHEME := "khung-chat"
const CHANNEL_MAX := 15            # Channel0..14
const NAME_COLOR_DEFAULT := Color(220.0 / 255.0, 220.0 / 255.0, 220.0 / 255.0)   # [Main] NameTextColor of the real file

# Proto.ChatChannel numbers (the order of [Channels]); CH_WHISPER is the zone's someone chat, not a section
const CH_NEARBY := 0
const CH_TEAM := 1
const CH_WORLD := 2
const CH_FACTION := 3
const CH_SYSTEM := 4
const CH_CITY := 5
const CH_TONG := 6
const CH_WHISPER := 7

# the channels a player may speak in from the bar (the zone refuses the rest): CH_TONG waits for the tongs
const SPEAKABLE := [CH_NEARBY, CH_TEAM, CH_WORLD, CH_FACTION, CH_CITY]

var channels: Array = []          # index = channel number: {key, short: [..], format, color, border, menu_text, interval, num, image}
var name_color := NAME_COLOR_DEFAULT
var name_border := Color.BLACK
var self_color := Color(1.0, 226.0 / 255.0, 168.0 / 255.0)      # [MSNRoom] TextColorSelf
var unknown_color := Color(252.0 / 255.0, 151.0 / 255.0, 1.0)   # TextColorUnknown
var friend_color := Color(1.0, 202.0 / 255.0, 1.0)              # TextColorFriend
var default_channel := CH_SYSTEM
var current := CH_NEARBY          # KUiPlayerBar+0x8c48: the channel the bar sends on
var _last_sent: Dictionary = {}   # channel -> [msec, msec, ..] of the last lines (SendMsgNum / SendMsgInterval)


# the exported layout (client/assets/ui/khung-chat/bo-cuc.json); false when it is not there
func load_scheme() -> bool:
	# loaded here, not preloaded: KUiScheme needs the Assets autoload, which the headless tests run without
	var scheme_script: GDScript = load("res://ui/KUiScheme.gd")
	var ini = scheme_script.open(SCHEME)
	if ini == null:
		return false
	var sections := {}
	for i in CHANNEL_MAX:
		var key: String = ini.get_string("Channels", "Channel%d" % i, "")
		if key == "":
			continue
		var values := {}
		for k in ["ShortName0", "ShortName1", "ShortName2", "FormatName", "TextColor", "TextBorderColor", "MenuText", "TextImage",
				"SendMsgInterval", "SendMsgNum"]:
			if ini.has_key(key, k):
				values[k] = ini.get_string(key, k, "")
		sections[key] = values
	var order: Array = []
	for i in CHANNEL_MAX:
		order.append(ini.get_string("Channels", "Channel%d" % i, ""))
	from_sections(order, sections)
	for i in channels.size():
		var img = ini.image(str(channels[i].key), "TextImage")
		channels[i]["texture"] = img.texture if img != null else null
	name_color = ini.get_color("Main", "NameTextColor", NAME_COLOR_DEFAULT)
	name_border = ini.get_color("Main", "NameTextBorderColor", Color.BLACK)
	self_color = ini.get_color("MSNRoom", "TextColorSelf", self_color)
	unknown_color = ini.get_color("MSNRoom", "TextColorUnknown", unknown_color)
	friend_color = ini.get_color("MSNRoom", "TextColorFriend", friend_color)
	var def: String = ini.get_string("Channels", "DefaultChannel", "")
	default_channel = maxi(index_of(def), 0)
	return not channels.is_empty()


# the table from the [Channels] order and the [CH_*] values (what load_scheme reads; the tests feed it directly)
func from_sections(order: Array, sections: Dictionary) -> void:
	channels.clear()
	for i in order.size():
		var key := str(order[i])
		var v: Dictionary = sections.get(key, {})
		var short: Array = []
		for n in 3:
			var s := str(v.get("ShortName%d" % n, "")).strip_edges()
			if s != "":
				short.append(s)
		channels.append({
			"key": key,
			"short": short,
			"format": str(v.get("FormatName", "")),
			"color": KText.color_of(str(v.get("TextColor", "")), Color.WHITE),
			"border": KText.color_of(str(v.get("TextBorderColor", "")), Color.BLACK),
			"menu_text": str(v.get("MenuText", "")),
			"image": str(v.get("TextImage", "")),
			"interval": int(str(v.get("SendMsgInterval", "0"))),
			"num": int(str(v.get("SendMsgNum", "0"))),
		})


func channel(index: int) -> Dictionary:
	return channels[index] if index >= 0 and index < channels.size() else {}


func index_of(key: String) -> int:
	for i in channels.size():
		if channels[i].key == key:
			return i
	return -1


func color_of(index: int) -> Color:
	var c := channel(index)
	return c.get("color", Color.WHITE) if not c.is_empty() else Color.WHITE


# KUiMsgCentrePad::GetChannelIndex(name) of the input prefix "&<short> ...": the channel one of whose ShortName is the
# word (case as written; the 2.0 client compares bytes); -1 when none
func index_by_short(word: String) -> int:
	if word == "":
		return -1
	for i in channels.size():
		for s in channels[i].short:
			if str(s) == word:
				return i
	return -1


# the short name shown on the ChannelBtn: ShortName0 (the sections always have one)
func short_name(index: int) -> String:
	var c := channel(index)
	var s: Array = c.get("short", [])
	return str(s[0]) if not s.is_empty() else ""


# the entries of the channel menu (0x00472620: one line per channel the bar can send on, each in its own colour)
func menu_entries() -> Array:
	var out: Array = []
	for idx in SPEAKABLE:
		var c := channel(idx)
		if c.is_empty():
			continue
		out.append({"index": idx, "text": str(c.menu_text), "color": c.color})
	return out


# KUiPlayerBar::SendChat 0x00475A10 on the text of the input line: a leading '/' names the player to whisper to
# ("/name text" -> Lua Say(name, text)), a leading '&' names the channel by its short name ("&T text" -> Chat(channel,
# text)), anything else goes to the current channel.  The word ends at the first space (0x00475BE0).  -> {channel,
# target, text}; channel -1 = the '&' word names no channel (the 2.0 client then sends on the current one)
func parse_input(text: String) -> Dictionary:
	var out := {"channel": current, "target": "", "text": text}
	if text.is_empty():
		return out
	var lead := text[0]
	if lead != "/" and lead != "&":
		return out
	var sp := text.find(" ")
	var word := text.substr(1, (sp - 1) if sp > 0 else text.length() - 1)
	var rest := text.substr(sp + 1) if sp > 0 else ""
	if lead == "/":
		out.channel = CH_WHISPER
		out.target = word
		out.text = rest
		return out
	var idx := index_by_short(word)
	out.channel = idx if idx >= 0 else current
	out.text = rest if idx >= 0 else text
	return out


# SendMsgNum lines every SendMsgInterval ms (KUiMsgCentrePad::PushChannelData: the excess waits - here it is refused
# with G_PLAYERBAR_3 "%d giây"); -> 0 ok, else the seconds to wait
func throttle(index: int, now_ms: int) -> int:
	var c := channel(index)
	if c.is_empty() or int(c.interval) <= 0 or int(c.num) <= 0:
		return 0
	var times: Array = _last_sent.get(index, [])
	var keep: Array = []
	for t in times:
		if now_ms - int(t) < int(c.interval):
			keep.append(t)
	if keep.size() >= int(c.num):
		var wait := int(c.interval) - (now_ms - int(keep[0]))
		_last_sent[index] = keep
		return maxi(1, int(ceil(wait / 1000.0)))
	keep.append(now_ms)
	_last_sent[index] = keep
	return 0


# KUiMsgCentrePad::ChannelMessageArrival (2004): [picture] name: text - the name in NameTextColor, the text in the
# channel's TextColor; a whisper (MSNMessageArrival) in TextColorSelf when it is my own, TextColorUnknown otherwise.
# -> {"texture": the channel's TextImage (or null), "bbcode": the rest of the line}
func line(msg: Dictionary, own_name: String) -> Dictionary:
	var ch := int(msg.get("channel", CH_NEARBY))
	var name := str(msg.get("name", ""))
	var text := str(msg.get("text", "")).replace("[", "[lb]")
	var color: Color
	if ch == CH_WHISPER:
		color = self_color if name == own_name else unknown_color
	else:
		color = color_of(ch)
	var head := ""
	if name != "":
		head = "[color=#%s]%s:[/color] " % [name_color.to_html(false), name.replace("[", "[lb]")]
	var c := channel(ch)
	return {"texture": c.get("texture", null), "bbcode": "%s[color=#%s]%s[/color]" % [head, color.to_html(false), text]}
