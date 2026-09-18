# KUiSkillDesc - the tip of a skill, line by line as KSkill::GetDesc 0x006FBC90 of the 2.0 client writes it (JX1:
# Core/Src/KSkills.cpp KSkill::GetDesc / GetDescAboutLevel; docs/CLIENT-2.0.md §10).  The texts are the G_* strings of
# \lang\vn\stringtable_core.txt, [Descript] of magicdesc.ini and [SkillAttrib] / [WeaponLimit] of gamesetting.ini
# (jxassets export-skill-desc -> text/skill_desc.json); the numbers of a level come from the zone (G2C_SKILL_DESC).
#   0x006FBD3D  "<color=Yellow>" + SkillName; +0x7a4 == 75 (a seriesdamage_p attribute) and Series >= 0 -> " " + G_S_<series>
#   0x006FBEEA  "\n<bclr=Black><color>"
#   0x006FBF2F  ReqLevel > the character's level -> G_SkillList_6 % ReqLevel
#   0x006FBF92  "\n" + SkillDesc + "\n\n"
#   0x006FC022  [SkillAttrib] of the Attrib column + "\n"
#   0x006FC085  Attrib 1 / 2 (the plain attacks): IsMelee -> G_Skills_35 " (Cong kich gan)" else G_Skills_36
#   0x006FC114  not an aura: G_Skills_37 % level (G_Skills_38 in blue with a level bonus - none here); an aura skips to the limits
#   0x006FC27F  the enhance percent (G_Skills_39) and the [SkillType] bonus (G_Skills_76): not here (the zone has no such numbers)
#   0x006FC473  IsExpSkill -> G_Skills_40 % the exp percent
#   0x006FB140  GetDescAboutLevel of the level shown: "\n", cost (G_Skills_45..47 by SkillCostType), range (G_Skills_48),
#               the attribute lines through KMagicDesc (0x006F82F0: immediate, damage, state), addskilldamage (G_Skills_49)
#   0x006FC527  EqtLimit (+0xac): -2 nothing; F<n> for a negative, the number else -> G_Skills_41 + [WeaponLimit] text
#   0x006FC617  HorseLimit 1 -> G_Skills_42, 2 -> G_Skills_43
#   0x006FC697  not an aura, a next level -> G_Skills_44 + GetDescAboutLevel of the next level
extends RefCounted

const KMagicDesc := preload("res://ui/KMagicDesc.gd")


static func _s(strings: Dictionary, key: String, def: String) -> String:
	return str(strings.get(key, def))


static func _cell(row: Dictionary, key: String, def: String = "") -> String:
	return str(row.get(key, def))


static func _cell_int(row: Dictionary, key: String, def: int = 0) -> int:
	var v := _cell(row, key, "").strip_edges()
	return v.to_int() if v != "" else def


static func context(text: Dictionary, name_of: Callable) -> Dictionary:
	var strings: Dictionary = text.get("strings", {})
	return {
		"series": [_s(strings, "G_S_GOLD", "Kim"), _s(strings, "G_S_WOOD", "Mộc"), _s(strings, "G_S_WATER", "Thuỷ"), _s(strings, "G_S_FIRE", "Hoả"), _s(strings, "G_S_EARTH", "Thổ")],
		"series_none": _s(strings, "G_S_NONE", "Vô hệ"),
		"cost_types": ["Nội lực", "Sinh lực", "Thể lực", "Tiền"],   # KMagicDesc.cpp 'k' (the JX1 words, translated)
		"sex": ["Nam", "Nữ"],
		"factions": {},
		"skill_name": name_of,
		"own_skill": "Võ công vốn có",
	}


# GetDescAboutLevel 0x006FB140 for one level's numbers {cost, cost_type, attack_radius, attribs, appends}
static func level_lines(lv: Dictionary, text: Dictionary, ctx: Dictionary) -> String:
	var strings: Dictionary = text.get("strings", {})
	var descript: Dictionary = text.get("descript", {})
	var out := "\n"
	var cost := int(lv.get("cost", 0))
	if cost != 0:
		match int(lv.get("cost_type", 0)):
			0:
				out += _s(strings, "G_Skills_45", "Tiêu hao nội lực: %d\n") % cost
			1:
				out += _s(strings, "G_Skills_46", "Tiêu hao thể lực: %d\n") % cost
			2:
				out += _s(strings, "G_Skills_47", "Tiêu hao sinh lực: %d\n") % cost
	var radius := int(lv.get("attack_radius", 0))
	if radius != 0:
		out += _s(strings, "G_Skills_48", "Khoảng cách hiệu quả: %d\n") % radius
	# 0x006F82F0: the immediate, the damage and the state attributes, each through [Descript] (an empty text is skipped)
	for group in [0, 1, 2]:
		for a in lv.get("attribs", []):
			if int(a.get("group", 0)) != group:
				continue
			var fmt := str(descript.get(str(a.get("name", "")).to_lower(), ""))
			if fmt == "":
				continue
			var line := KMagicDesc.describe_line(fmt, [int(a.get("v0", 0)), int(a.get("v1", 0)), int(a.get("v2", 0))], ctx)
			if line != "":
				out += line + "\n"
	# 0x006FB383: addskilldamage1..6 -> G_Skills_49 with the other skill's name
	var name_of = ctx.get("skill_name")
	for p in lv.get("appends", []):
		var id := int(p.get("skill_id", 0))
		if id <= 0:
			continue
		var name := str(name_of.call(id)) if name_of is Callable else str(id)
		out += _s(strings, "G_Skills_49", "Tăng cho kỹ năng %s: %d%%\n") % [name, int(p.get("value", 0))]
	return out


# row: the skills.json cells; held: the character's entry of Game.skills ({} when not held); desc: the zone's answer
# ({} while it is on its way); text: text/skill_desc.json; name_of: Callable(skill id) -> name
static func build(row: Dictionary, held: Dictionary, player_level: int, desc: Dictionary, text: Dictionary, name_of: Callable) -> String:
	var strings: Dictionary = text.get("strings", {})
	var ctx := context(text, name_of)
	var level := int(held.get("level", 0))
	var aura := _cell_int(row, "IsAura", 0) != 0
	var cur: Dictionary = desc.get("cur", {}) if desc.get("has_cur", false) else {}
	var out := "<color=Yellow>" + _cell(row, "SkillName", "")
	var series := _cell_int(row, "Series", -1)
	if series >= 0 and series <= 4 and _has_attrib(cur, "seriesdamage_p"):
		out += " " + str(ctx.series[series])
	out += "\n<bclr=Black><color>"
	var req := _cell_int(row, "ReqLevel", 0)
	if req > player_level:
		out += _s(strings, "G_SkillList_6", "Đẳng cấp yêu cầu: %d") % req
	out += "\n" + _cell(row, "SkillDesc", "") + "\n\n"
	var attrib := _cell_int(row, "Attrib", 0)
	var attrib_text := str(text.get("skill_attrib", {}).get(str(attrib), ""))
	if attrib_text != "":
		out += attrib_text + "\n"
	if attrib == 1 or attrib == 2:
		out += _s(strings, "G_Skills_35", " (Công kích gần) \n") if _cell_int(row, "IsMelee", 0) != 0 else _s(strings, "G_Skills_36", " (Công kích xa) \n")
	if not aura:
		out += (_s(strings, "G_Skills_37", "Cấp hiện tại: %d") % level) + "\n"
		if _cell_int(row, "IsExpSkill", 0) != 0:
			out += _s(strings, "G_Skills_40", "Độ tu luyện: %d%%\n") % int(held.get("exp_percent", 0))
	if not cur.is_empty():
		out += level_lines(cur, text, ctx)
	var eqt := _cell_int(row, "EqtLimit", -2)
	if eqt != -2:
		var key := ("f%d" % (-eqt)) if eqt < 0 else str(eqt)
		var limit := str(text.get("weapon_limit", {}).get(key, ""))
		if limit != "":
			out += _s(strings, "G_Skills_41", "Hạn chế vũ khí:") + limit + "\n"
	match _cell_int(row, "HorseLimit", 0):
		1:
			out += _s(strings, "G_Skills_42", "Trong lúc cưỡi ngựa không thể thi triển \n")
		2:
			out += _s(strings, "G_Skills_43", "Cần phải cưỡi ngựa để thi triển \n")
	if not aura and desc.get("has_next", false):
		out += _s(strings, "G_Skills_44", "\n<color=Red> Đẳng cấp tiếp theo \n")
		out += level_lines(desc.get("next", {}), text, ctx)
	return out


static func _has_attrib(lv: Dictionary, name: String) -> bool:
	for a in lv.get("attribs", []):
		if str(a.get("name", "")) == name:
			return true
	return false
