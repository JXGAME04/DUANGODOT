# UiNews - the news ticker of the 2.0 client (the window [0x8bf730] of gamecl.exe: ctor 0x004D5850, vtable 0x795444,
# 0x004D59F0 names "新闻消息来了.ini"; AddGlobalNews / AddGlobalCountNews of the scripts reach it as the 0x63 packet ui 5 ->
# ui message 0x20 GDCNI_NEWS_MESSAGE -> 0x004D60E0).  [Main] of the ini: 400x16 at (312,30), Font 14, IndentH 11, IndentV 2,
# TextColor 255,253,122, ScrollInterval 125 ms, ShowInterval 20000 ms, MaxIdleTime 60000 ms.  Init 0x004D5E10 derives the two
# scroll steps (Font+1)/2 and Font/2 (+0x4c4 / +0x4c8), the text area Width - 2*IndentH (+0x4bc) and the characters that fit
# (+0x4c0).  A news is a node {type, text, count, shown} put at the FRONT of the list (0x004D5C20); Breathe 0x004D69B0 runs the
# fixed-step timer 0x00450DD0 every ScrollInterval and moves the text left by the alternating steps (0x004D6660), starting at the
# right edge of the area (0x004D6600) until the whole text has left it; then the node goes to the end of the list (0x004D5C80 +
# 0x004D5CF0) and the next one is picked from the head (0x004D6860: type 0 shows once, type 1 `count` times, a used-up node is
# dropped); with nothing queued for MaxIdleTime a random line of \Ui\DefaultMessage.ini ([Main] Count, 0..Count-1) is shown
# (vfunc+0x50 0x004D5A00).  Between two ticks the text keeps moving (HANDOVER §0 rule 13); the bar stays centred near the top
# whatever the screen (rule 14).  docs/CLIENT-2.0.md §30.
extends "res://ui/elem/KWndWindow.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KUiDialogMath := preload("res://ui/KUiDialogMath.gd")
const KFont := preload("res://ui/KFont.gd")

const SCHEME := "tin-toan-cuc"
const DEFAULT_SCHEME := "tin-mac-dinh"

var scroll_interval_ms := 250   # ScrollInterval (the Init's default 0xfa)
var show_interval_ms := 30000   # ShowInterval (default 0x7530; the dated news of type 2 use it - not built)
var max_idle_ms := 30000        # MaxIdleTime (default 0x7530)
var indent := Vector2i(0, 0)    # IndentH / IndentV (+0x4a8 / +0x4ac)
var font_size := 14             # Font (+0x4b0, never below 8: 0x004D5F11)
var steps: Array = [7, 7]       # (Font+1)/2 and Font/2: the pixels one tick moves the text, alternating (+0x4c4 / +0x4c8)
var area_width := 0             # Width - 2 * IndentH (+0x4bc)
var default_lines: Array = []   # \Ui\DefaultMessage.ini [Main] 0..Count-1

var _queue: Array = []          # {type, text, count, shown}: the list at +0x4a0
var _current: Dictionary = {}   # +0x4a4
var _x := 0.0                   # the text's left edge inside the area at the last tick (+0x6f4)
var _toggle := 0                # which of the two steps the next tick takes (+0x6f8)
var _last_tick_ms := 0          # the fixed-step timer (+0x6e8 / 0x00450DD0)
var _idle_since_ms := 0         # when the last news ended (+0x4d8)
var _text_width := 0
var _text: KWndText = null
var _theme := Vector2i(1024, 768)
var _theme_pos := Vector2.ZERO
var _theme_size := Vector2.ZERO


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_from(ini, "Main"):
		return false
	name = "UiNews"
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	clip_contents = true
	indent = Vector2i(ini.get_integer("Main", "IndentH", 0), ini.get_integer("Main", "IndentV", 0))
	font_size = maxi(ini.get_integer("Main", "Font", 12), 8)
	steps = [(font_size + 1) / 2, font_size / 2]
	area_width = int(size.x) - 2 * indent.x
	max_idle_ms = ini.get_integer("Main", "MaxIdleTime", 30000)
	show_interval_ms = ini.get_integer("Main", "ShowInterval", 30000)
	scroll_interval_ms = maxi(ini.get_integer("Main", "ScrollInterval", 250), 1)
	_theme = ini.screen_size()
	_theme_pos = position
	_theme_size = size
	_text = KWndText.new()
	add_child(_text)
	_text.font_size = font_size
	_text.text_color = ini.get_color("Main", "TextColor", Color.BLACK)
	_text.border_color = ini.get_color("Main", "TextBorderColor", Color.BLACK)   # 0x004D5F95: black when the ini names none
	_text.multi_line = false
	_text.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_text.size = Vector2(4096, maxf(size.y - indent.y, font_size + 1))
	_text.position = Vector2(indent.x + area_width, indent.y)
	_text.visible = false
	var defaults: KUiScheme = KUiScheme.open(DEFAULT_SCHEME)
	if defaults != null:
		var count := defaults.get_integer("Main", "Count", 0)
		for i in count:
			var line := defaults.get_string("Main", str(i), "")
			if line != "":
				default_lines.append(line)
	_idle_since_ms = Time.get_ticks_msec()
	_place(screen)
	var vp := get_viewport()
	if vp != null:
		vp.size_changed.connect(func(): _place(Vector2i(get_viewport().get_visible_rect().size)))
	visible = false
	set_process(true)
	return true


# the bar keeps its place near the top, centred (rule 14): 400x16 at (312,30) sits at the centre of the theme's 1024
func _place(screen: Vector2i) -> void:
	position = KUiDialogMath.top_center_anchor(_theme_pos, _theme_size, Vector2(_theme), Vector2(screen))


# ui 5 of OnScriptAction -> 0x004D60E0 -> 0x004D5C20: type 0 (AddGlobalNews) shows once, type 1 (AddGlobalCountNews) `count`
# times (0 -> 3, 0x004D61ED); type 2 (a dated news) is taken as type 0 here
func add_news(text: String, type: int = 0, count: int = 0) -> void:
	if text == "":
		return
	KUiDialogMath.news_add(_queue, text, type, count)
	visible = true


func current_text() -> String:
	return str(_current.get("text", ""))


func queue_size() -> int:
	return _queue.size()


func text_x() -> int:
	return int(_text.position.x) if _text != null else 0


func _process(_delta: float) -> void:
	if _text == null:
		return
	var now := Time.get_ticks_msec()
	if _current.is_empty():
		_current = KUiDialogMath.news_pick(_queue)
		if _current.is_empty() and not default_lines.is_empty() and now - _idle_since_ms >= max_idle_ms:
			# 0x004D6860 -> vfunc+0x50 0x004D5A00: rand() % Count of DefaultMessage.ini, shown like a type-0 news
			KUiDialogMath.news_add(_queue, str(default_lines[randi() % default_lines.size()]), 0, 0)
			_current = KUiDialogMath.news_pick(_queue)
		if _current.is_empty():
			_text.visible = false
			return
		_text.set_text(str(_current.text))
		_text_width = KFont.of(font_size).width_of(str(_current.text))
		_x = float(area_width)   # 0x004D6600: the right edge of the area
		_toggle = 0
		_last_tick_ms = now
		_text.visible = true
	# the fixed-step timer of 0x00450DD0: every ScrollInterval one step, the two step widths alternating (0x004D6660)
	while now - _last_tick_ms >= scroll_interval_ms:
		_last_tick_ms += scroll_interval_ms
		_x -= float(steps[_toggle])
		_toggle = 1 - _toggle
	# between two ticks the text keeps moving (rule 13)
	var fraction := clampf(float(now - _last_tick_ms) / float(scroll_interval_ms), 0.0, 1.0)
	var x := _x - float(steps[_toggle]) * fraction
	_text.position = Vector2(indent.x + x, indent.y)
	if x + float(_text_width) <= 0.0:   # the whole text has left the area: the pass is over
		KUiDialogMath.news_pass_done(_queue, _current)
		_current = {}
		_idle_since_ms = now
		_text.visible = false
