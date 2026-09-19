# KUiDialogMath - the pure parts of the npc dialog windows (UiMsgSel, UiInformation2), kept free of autoloads so the
# headless tests can run them (docs/CLIENT-2.0.md §24).
extends RefCounted


# KUiMsgSel::Show (2004 UiMsgSel.cpp, 2.0 0x0051D0F0): the answers as the lines of the list, or the one closing line
# (G_UiMsgSel_0 "Kết thúc đối thoại") when the script gave none
static func lines_for(answers: Array, closing_line: String) -> Array:
	if answers.is_empty():
		return [closing_line]
	var out: Array = []
	for a in answers:
		out.append(str(a))
	return out


# the button of page `index` of `count` (KPlayer::OnScriptAction 0x00600EB2 / 0x00600EC8): "Tiếp tục" (G_PLAYER_14)
# until the last page, "Hoàn thành" (G_PLAYER_15) on it
static func page_label(index: int, count: int, cont: String, done: String) -> String:
	return done if index >= count - 1 else cont


# the bottom-right anchor of a pane laid out on the theme's screen (HANDOVER §0 rule 14): the pane keeps its distance to
# the right and bottom edges whatever the screen becomes (系统消息.ini: 240x250 at (790,593) of 1024x768)
static func bottom_right_anchor(theme_pos: Vector2, theme_size: Vector2, theme_screen: Vector2, screen: Vector2) -> Vector2:
	var right := theme_screen.x - (theme_pos.x + theme_size.x)
	var bottom := theme_screen.y - (theme_pos.y + theme_size.y)
	return Vector2(screen.x - right - theme_size.x, screen.y - bottom - theme_size.y)


# the system message pane's OpenWindow 0x004C4060: a message of a type 1..`types` joins `messages` unless the same
# (type, blink, priority, text) is already there (0x004C3820); it goes after the entries of the same or a lower priority
# (0x004C3A4D..0x004C3A6F) and remembers when it came (0x00608270).  Returns whether it joined.
static func sys_msg_add(messages: Array, m: Dictionary, now_ms: int, types: int) -> bool:
	var type := int(m.get("type", 1))
	if type < 1 or type > types:
		return false
	var entry := {"type": type, "text": str(m.get("text", "")), "blink": bool(m.get("blink", true)), "priority": int(m.get("priority", 3)), "at_ms": now_ms}
	for e in messages:
		if int(e.type) == entry.type and bool(e.blink) == entry.blink and int(e.priority) == entry.priority and str(e.text) == entry.text:
			return false
	var at := messages.size()
	while at > 0 and int(messages[at - 1].priority) > entry.priority:
		at -= 1
	messages.insert(at, entry)
	return true


# the messages older than SysMsgDisappearInterval leave; true when any did
static func sys_msg_prune(messages: Array, now_ms: int, disappear_ms: int) -> bool:
	var kept: Array = []
	for e in messages:
		if now_ms - int(e.at_ms) < disappear_ms:
			kept.append(e)
	if kept.size() == messages.size():
		return false
	messages.clear()
	for e in kept:
		messages.append(e)
	return true


# the newest message of a type ({} when the type holds none)
static func sys_msg_latest(messages: Array, type: int) -> Dictionary:
	var latest := {}
	for e in messages:
		if int(e.type) == type and (latest.is_empty() or int(e.at_ms) >= int(latest.at_ms)):
			latest = e
	return latest


# the icon of a type turns its frames while one of its messages blinks (0x004C3B5F: the first entry's +0x105 == 1)
static func sys_msg_blinks(messages: Array, type: int) -> bool:
	for e in messages:
		if int(e.type) == type and bool(e.blink):
			return true
	return false
