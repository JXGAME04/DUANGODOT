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
