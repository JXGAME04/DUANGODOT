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


# the give-item box of GiveItemUI (给予界面.ini [Items]: 6 x 4 cells): a piece of w x h cells dropped with its top-left on
# `cell` fits when it lies inside and covers no piece already there; the cell, or null
static func give_box_place(objects: Array, cell: Vector2i, w: int, h: int, box_w: int, box_h: int):
	if cell.x < 0 or cell.y < 0 or cell.x + w > box_w or cell.y + h > box_h:
		return null
	for o in objects:
		if cell.x < int(o.x) + int(o.w) and cell.x + w > int(o.x) and cell.y < int(o.y) + int(o.h) and cell.y + h > int(o.y):
			return null
	return cell


# the cell code the server keeps for a piece (0x080ABA96: y * 6 + x + 1, what GetGiveItemUnitWithPos answers)
static func give_cell_code(x: int, y: int) -> int:
	return y * 6 + x + 1


# the 0x89 list from the pieces in the box: where each lies in the bag ({room, x, y} of the item) and its cell
static func give_entries(placed: Array, items: Dictionary) -> Array:
	var out: Array = []
	for p in placed:
		var it = items.get(int(p.id))
		if it == null:
			continue
		out.append({"room": int(it.get("room", 0)), "x": int(it.get("x", 0)), "y": int(it.get("y", 0)), "cell_x": int(p.x), "cell_y": int(p.y)})
	return out


# KTaskDataFile::InsertSystemRecord (2004 UiTaskDataFile.cpp): a new system record of the journal goes in front of the
# others; returns the new list
static func journal_insert(records: Array, record: Dictionary) -> Array:
	var out: Array = [record]
	for r in records:
		out.append(r)
	return out


# KUiTaskNote_System::UpdateView: one line a record, the text as it came (AddOneMessage of the record's buffer)
static func journal_lines(records: Array) -> Array:
	var out: Array = []
	for r in records:
		out.append(str(r.get("text", "")))
	return out
