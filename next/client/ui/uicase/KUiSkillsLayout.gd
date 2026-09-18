# KUiSkillsLayout - the numbers of the 2.0 skill book (gamecl.exe KUiSkills / KUiFightSkill / KUiCommonSkill,
# docs/CLIENT-2.0.md §6), kept apart from UiSkills.gd so the headless tests can check them without the game's
# autoloads.
extends RefCounted

const BRANCHES := 3            # 0x004953C0: three fight pages, three branch buttons
const PAGE_COMMON := 3         # the fourth page
const TIER_COLS := 10          # 0x00494030: ten columns
const SLOT_ROWS := 3           # three rows
const COL_PITCH := 51          # 0x33
const ROW_PITCH := 58          # 0x3a (mode 0 of the box pad 0x004933A0: the fight pages)
const ROW_PITCH_COMMON := 61   # 0x3d (mode 1: the common page)
const BRANCH_BUTTON_PITCH := 103   # 0x67: the three branch buttons, then the common one in the fourth slot


# 0x00494AAF: the x of branch button i, from the [FightBtn] position (i = 3 is where [CommonBtn] lies in the file)
static func branch_button_x(base_x: int, index: int) -> int:
	return base_x + BRANCH_BUTTON_PITCH * index


# 0x00493982 / 0x0049398B: the box index of a place - slot row, tier column
static func box_index(slot: int, tier: int) -> int:
	return slot * TIER_COLS + tier


# 0x00493A40 (0x00493B01 outer tier, 0x00493B10 inner slot): the order the common page fills its boxes in -
# column by column, the three slots of a tier before the next tier; the box index of the n-th free cell
static func common_fill_index(n: int) -> int:
	@warning_ignore("integer_division")
	var tier: int = n / SLOT_ROWS
	var slot: int = n % SLOT_ROWS
	return box_index(slot, tier)
