# UiSysMsg - the system message pane at the bottom right of the 2.0 client (the window 0x846eac, 0x4c54 bytes: ctor
# 0x004C45C0, Init 0x004C4760, 0x004C3D1D loads "%s\系统消息.ini"; UiSysMsgCentre.cpp of 2004 only names the kinds).
# The layout: [Main] 240x250 at (790,593) with NormalMsgColor 255,0,0 and SysMsgDisappearInterval (default 30000 ms,
# 0x004C47E0), [MsgText] the line template (0,25) 240x14 font 12 colour 0,255,0, [MsgIcon_1..8] 24x24 icons at
# (200, (n-1)*25): 系统 system, 升级 level, 组队 team, 聊天 chat, 任务 task, 帮会 guild, 交易 trade, 定位 locate.
# A message is {text[0x104], type +0x104, blink +0x105, priority +0x106, extra +0x107}: OpenWindow 0x004C4060 drops a
# duplicate (0x004C3820 compares type, blink, priority and text), keeps the copy in the list of its type (0x004C39F0:
# eight lists at +0x4c8, sorted by priority) and stamps it (0x00608270); the paint 0x004C3A90 draws one icon per type
# that holds messages, the icon's frames turning while blink is 1 (0x004C3B5F).  TaskTip (the 0xb6 packet with the
# 0x10 byte -> ui message 0x5d, 0x0042A10A) is type 1, blink 1, priority 3.  The text of the newest message of a type
# is written on the [MsgText] line under the icons here; the pane's WndProc (hover / click of an icon) is not read yet.
# The window is anchored to the bottom right (HANDOVER §0 rule 14): its distance to those edges on the theme's
# 1024x768 stays whatever the screen becomes.  docs/CLIENT-2.0.md §26.
extends "res://ui/elem/KWndWindow.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndImage := preload("res://ui/elem/KWndImage.gd")
const KUiDialogMath := preload("res://ui/KUiDialogMath.gd")

const SCHEME := "thong-diep-he-thong"
const TYPES := 8
const ICON_BLINK_MS := 500   # the frames of a blinking icon turn every half second

var disappear_ms := 30000    # SysMsgDisappearInterval
var _icons: Array = []       # KWndImage per type
var _lines: Array = []       # KWndText per type: the newest message of the type
var _messages: Array = []    # {type, text, blink, priority, at_ms}
var _theme := Vector2i(1024, 768)
var _theme_pos := Vector2.ZERO
var _theme_size := Vector2.ZERO
var _blink_ms := 0


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_from(ini, "Main"):
		return false
	name = "UiSysMsg"
	disappear_ms = ini.get_integer("Main", "SysMsgDisappearInterval", 30000)
	_theme = ini.screen_size()
	_theme_pos = position
	_theme_size = size
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	var line_top := ini.get_integer("MsgText", "Top", 25)
	for i in TYPES:
		var icon := KWndImage.new()
		add_child(icon)
		icon.init_from(ini, "MsgIcon_%d" % (i + 1))
		icon.visible = false
		_icons.append(icon)
		var line := KWndText.new()
		add_child(line)
		line.init_from(ini, "MsgText")
		line.position = Vector2(0, icon.position.y + line_top)
		line.visible = false
		_lines.append(line)
	_place(screen)
	var vp := get_viewport()
	if vp != null:
		vp.size_changed.connect(func(): _place(Vector2i(get_viewport().get_visible_rect().size)))
	set_process(true)
	return true


# the pane keeps its distance to the right and bottom edges of the theme's screen (rule 14)
func _place(screen: Vector2i) -> void:
	position = KUiDialogMath.bottom_right_anchor(_theme_pos, _theme_size, Vector2(_theme), Vector2(screen))


# OpenWindow 0x004C4060: a duplicate is dropped, else the message joins its type's list (sorted by priority) with a stamp
func add_message(text: String, type: int = 1, blink: bool = true, priority: int = 3) -> bool:
	var now := Time.get_ticks_msec()
	if not KUiDialogMath.sys_msg_add(_messages, {"type": type, "text": text, "blink": blink, "priority": priority}, now, TYPES):
		return false
	_refresh()
	return true


func message_count() -> int:
	return _messages.size()


func _process(_delta: float) -> void:
	var now := Time.get_ticks_msec()
	if KUiDialogMath.sys_msg_prune(_messages, now, disappear_ms):
		_refresh()
	if now - _blink_ms >= ICON_BLINK_MS:
		_blink_ms = now
		for i in TYPES:
			var icon: KWndImage = _icons[i]
			if icon.visible and KUiDialogMath.sys_msg_blinks(_messages, i + 1):
				icon.next_frame()


func _refresh() -> void:
	for i in TYPES:
		var latest: Dictionary = KUiDialogMath.sys_msg_latest(_messages, i + 1)
		var icon: KWndImage = _icons[i]
		var line: KWndText = _lines[i]
		icon.visible = not latest.is_empty()
		line.visible = not latest.is_empty()
		if not latest.is_empty():
			line.set_text(str(latest.get("text", "")))
	visible = not _messages.is_empty()
