# KUiSkillTreeLayout - the grid of the 2.0 mouse-skill tree (KUiSkillTree, gamecl.exe 0x00495A40 layout /
# 0x00496170 paint / 0x00495B80 hit test), kept apart from the window script so the headless tests reach it.
#   Every entry carries a group (GDI 0x3f7 / 0x3f8: the current mouse skill is entry 0 of group 0, the held skills
#   then get group = index / 8, 0x00623B0C).  Walking the entries: an entry of the same group as the last one stays
#   on the row while the row holds fewer than MaxBtnCountPerRow; otherwise (a new group, or the row is full) a new
#   row starts.  The rows lie from the BOTTOM of the window up: the first row is the lowest (0x0049620A).
#   The window: left = LeftBtnPos.x, top = LeftBtnPos.y - rows * h + h, width = w * widest row, height = h * rows
#   (0x00495A97..0x00495AD1).
extends RefCounted


# {cells: [{row, col}] per entry (row 0 = the bottom row), rows, cols}
static func place(groups: Array, max_per_row: int) -> Dictionary:
	var cells: Array = []
	var rows := 0
	var cols := 0       # the widest row
	var col := 0        # items on the current row
	var last_group := -1
	for g in groups:
		var group := int(g)
		if group == last_group and col < max_per_row:
			col += 1
		else:
			rows += 1
			cols = maxi(cols, col)
			col = 1
			last_group = group
		cells.append({"row": rows - 1, "col": col - 1})
	cols = maxi(cols, col)
	return {"cells": cells, "rows": rows, "cols": cols}


# the window's rect from the anchor ([Main] LeftBtnPos: the lowest, leftmost button) and the grid
static func window_rect(anchor: Vector2i, btn: Vector2i, rows: int, cols: int) -> Rect2i:
	return Rect2i(anchor.x, anchor.y - btn.y * rows + btn.y, btn.x * cols, btn.y * rows)


# the top-left of an entry inside the window (rows counted from the bottom edge)
static func cell_pos(cell: Dictionary, btn: Vector2i, rows: int) -> Vector2i:
	return Vector2i(btn.x * int(cell.col), btn.y * (rows - 1 - int(cell.row)))


# 0x00495B80: the entry under a point inside the window, -1 for none
static func hit(local: Vector2i, btn: Vector2i, rows: int, cells: Array) -> int:
	if btn.x <= 0 or btn.y <= 0:
		return -1
	@warning_ignore("integer_division")
	var col: int = local.x / btn.x
	@warning_ignore("integer_division")
	var row: int = rows - 1 - local.y / btn.y
	if local.x < 0 or local.y < 0:
		return -1
	for i in cells.size():
		if int(cells[i].row) == row and int(cells[i].col) == col:
			return i
	return -1


# GDI 0x3f7 (left, 0x006239F0) / 0x3f8 (right, 0x00623B70): does a held skill of this style / aura flag belong in
# the tree?  Left: styles 5..12 always, 0..4 and 14 when not an aura, 13 never; right: 0..4 and 14 when not an aura.
static func listed(style: int, aura: bool, right: bool) -> bool:
	if right:
		return (style >= 0 and style <= 4 or style == 14) and not aura
	if style >= 5 and style <= 12:
		return true
	if style == 13:
		return false
	return (style >= 0 and style <= 4 or style == 14) and not aura
