# KWeaponSkillTable - the weapon -> plain-attack table of the 2.0 client (\settings\武器物理攻击对照表.txt, loaded
# in the core start 0x005CADC0 at 0x005CB396: DetailType 0 -> 0x9bcbf0[ParticularType], 1 -> 0x9bca60[ParticularType],
# -1 -> 0x9bca5c (bare hands); a particular of 0..99, a skill of 1..2998).  The zone reads the same file
# (KWeaponSkillTable of KSkill.cpp, jx_linux_y 0x0805F18D); jxassets export-weapon-skill writes weapon_skill.json.
#   - 0x0060D660 / 0x0060E3B0: the worn weapon's DetailType / ParticularType (-1 when no weapon is worn);
#   - 0x005EBBA0: the plain attack of an npc's weapon (the first entry of the skill tree, docs/CLIENT-2.0.md §8);
#   - 0x005FE820 (UpdateWeaponSkill, from KProtocolProcess::SyncEnd 0x00654F5B): both mouse skills become that
#     skill (SetLeftSkill 0x005F7550 / SetRightSkill 0x005FB280 take a skill the character holds at level >= 1).
extends RefCounted

const MAX_PARTICULAR := 99
const MAX_SKILL := 2998


# weapon_skill.json {rows: [{detail, particular, skill}]} -> {bare, melee: {particular: skill}, ranged: {...}}
static func parse(data) -> Dictionary:
	var t := {"bare": 0, "melee": {}, "ranged": {}}
	if not (data is Dictionary):
		return t
	for r in data.get("rows", []):
		if not (r is Dictionary):
			continue
		var detail := int(r.get("detail", 0))
		var particular := int(r.get("particular", 0))
		var skill := int(r.get("skill", 0))
		if skill < 1 or skill > MAX_SKILL:
			continue
		if detail == -1:
			t.bare = skill
		elif particular >= 0 and particular <= MAX_PARTICULAR:
			if detail == 0:
				t.melee[particular] = skill
			elif detail == 1:
				t.ranged[particular] = skill
	return t


# the plain attack for a weapon (detail -1 = none -> bare hands), 0 when the table has no row
static func skill_of(t: Dictionary, detail: int, particular: int) -> int:
	if t.is_empty():
		return 0
	if detail == -1:
		return int(t.get("bare", 0))
	if detail == 0:
		return int(t.get("melee", {}).get(particular, 0))
	if detail == 1:
		return int(t.get("ranged", {}).get(particular, 0))
	return 0
