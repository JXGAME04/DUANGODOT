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
#   0x006FC114  not a weapon skill (vtable +0x4c of the client's KSkill = IsWeaponSkill [+0x34]; IsAura is +0x48): G_Skills_37 % level,
#               or in blue G_Skills_38 % (level, level - inc, inc) when the skill list's increments (the zone's level_inc) are not 0;
#               a weapon skill (a plain attack) skips to the limits
#   0x006FC27F  the enhance map of the skill list (the zone's enhance, the addskilldamage of the skills held) -> G_Skills_39; the
#               [SkillType] 1 / 2 equipment bonus (Player+0x1278 + +0x1148 -> G_Skills_76 "%s%d%%") and the one attribute of
#               Player+0x12ad8 (0x005EC4F0): not here - the zone has no such player fields
#   0x006FC473  IsExpSkill -> G_Skills_40 % the exp percent
#   0x006FB140  GetDescAboutLevel of the level shown: "\n", cost (G_Skills_45..47 by SkillCostType), range (G_Skills_48),
#               the attribute lines through KMagicDesc (0x006F82F0: immediate, damage, state), the skills the level names
#               (0x006FAA00 -> 0x006F7F70: "Chieu N:" G_Skills_50.. from a counter that starts at 1, the name in yellow,
#               G_ITEM_22 "[Cap N]" in blue, then that skill's own attribute lines - the zone's `related` in print order),
#               addskilldamage (G_Skills_49) for the skills whose row has ShowAddition
#   0x006FC527  EqtLimit (+0xac): -2 nothing; F<n> for a negative, the number else -> G_Skills_41 + [WeaponLimit] text
#   0x006FC617  HorseLimit 1 -> G_Skills_42, 2 -> G_Skills_43
#   0x006FC697  not a weapon skill, a next level -> G_Skills_44 + GetDescAboutLevel of the next level
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
		"skill_row": Callable(),
		"own_skill": "Võ công vốn có",
	}


# 0x006F82F0: the immediate, the damage and the state attributes, each through [Descript] (an empty text is skipped)
static func attrib_lines(attribs: Array, descript: Dictionary, ctx: Dictionary) -> String:
	var out := ""
	for group in [0, 1, 2]:
		for a in attribs:
			if int(a.get("group", 0)) != group:
				continue
			var fmt := str(descript.get(str(a.get("name", "")).to_lower(), ""))
			if fmt == "":
				continue
			var line := KMagicDesc.describe_line(fmt, [int(a.get("v0", 0)), int(a.get("v1", 0)), int(a.get("v2", 0))], ctx)
			if line != "":
				out += line + "\n"
	return out


# GetDescAboutLevel 0x006FB140 for one level's numbers {cost, cost_type, attack_radius, attribs, related, appends}
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
	out += attrib_lines(lv.get("attribs", []), descript, ctx)
	# 0x006FAA00 -> 0x006F7F70: the skills the level names (the append skills held, the ShowEvent skills), each as
	# "Chieu N:" (G_Skills_50 + counter; the counter starts at 1, so the first is "Chieu 2:" - the skill itself being the
	# first move) unless flags & 1, the name in yellow unless flags & 2, G_ITEM_22 "[Cap N]" in blue unless flags & 4, a
	# line break unless all three, then its own attribute lines; its own named skills follow it in the zone's list
	var name_of = ctx.get("skill_name")
	var counter := 1
	for r in lv.get("related", []):
		var flags := int(r.get("flags", 0))
		var id := int(r.get("skill_id", 0))
		if flags & 1 == 0:
			out += _s(strings, "G_Skills_%d" % (50 + mini(counter, 19)), "<color=Blue> Chiêu %d: <color>" % (counter + 1))
			counter += 1
		if flags & 2 == 0:
			out += "<color=yellow>" + (str(name_of.call(id)) if name_of is Callable else str(id)) + "<color>"
		if flags & 4 == 0:
			out += "<color=blue>" + (_s(strings, "G_ITEM_22", "[Cấp %d]") % int(r.get("level", 0))) + "<color>"
		if flags & 7 != 7:
			out += "\n"
		out += attrib_lines(r.get("attribs", []), descript, ctx)
	# 0x006FB383: addskilldamage1..6 -> G_Skills_49 with the other skill's name, when that skill has a name and its row's
	# ShowAddition (+0x4f0) is not 0 (0x006FB3DF)
	var row_of = ctx.get("skill_row")
	for p in lv.get("appends", []):
		var id := int(p.get("skill_id", 0))
		if id <= 0 or int(p.get("value", 0)) == 0:
			continue
		var name := str(name_of.call(id)) if name_of is Callable else str(id)
		if name == "":
			continue
		if row_of is Callable and row_of.is_valid():
			var target = row_of.call(id)
			if not (target is Dictionary) or _cell_int(target, "ShowAddition", 0) == 0:
				continue
		out += _s(strings, "G_Skills_49", "Tăng cho kỹ năng %s: %d%%\n") % [name, int(p.get("value", 0))]
	return out


# row: the skills.json cells; held: the character's entry of Game.skills ({} when not held); desc: the zone's answer
# ({} while it is on its way); text: text/skill_desc.json; name_of: Callable(skill id) -> name; row_of: Callable(skill id)
# -> the skills.json cells of another skill (the ShowAddition gate of the addskilldamage lines; none = every line)
static func build(row: Dictionary, held: Dictionary, player_level: int, desc: Dictionary, text: Dictionary, name_of: Callable, row_of: Callable = Callable()) -> String:
	var strings: Dictionary = text.get("strings", {})
	var ctx := context(text, name_of)
	ctx["skill_row"] = row_of
	var level := int(held.get("level", 0))
	if int(desc.get("held_level", 0)) > 0:
		level = int(desc.get("held_level", 0))   # 0x006233B0(list, id, 1): the current level, increments included
	var weapon := _cell_int(row, "WeaponSkill", 0) != 0   # vtable +0x4c IsWeaponSkill (B4d-1 correction: not IsAura)
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
	if not weapon:
		var inc := int(desc.get("level_inc", 0))
		if inc == 0:
			out += (_s(strings, "G_Skills_37", "Cấp hiện tại: %d") % level) + "\n"
		else:
			out += "<color=Blue>" + (_s(strings, "G_Skills_38", "Cấp hiện tại: %d (%d+%d)") % [level, level - inc, inc]) + "\n<bclr=Black><color>"
		# 0x006FC27F: the enhance map of the skill list (0x00602420) -> G_Skills_39
		var enhance := int(desc.get("enhance", 0))
		if enhance != 0:
			out += _s(strings, "G_Skills_39", "Tăng từ kỹ năng: %d%%\n") % enhance
		# 0x006FC300: [SkillType][Attrib] of gamesetting.ini is 1 or 2 -> the equipment's share (Npc+0x1278 + +0x1148: the zone's
		# equip_percent = magicdamage_p) -> "%s%d%%\n" with G_Skills_76; 0x005EC4F0: the state modifier aimed at this skill -> its line
		var skill_type := int(text.get("skill_type", {}).get(str(attrib), 0))
		var equip := int(desc.get("equip_percent", 0))
		if (skill_type == 1 or skill_type == 2) and equip != 0:
			out += "%s%d%%\n" % [_s(strings, "G_Skills_76", "Trang bị gồm có:"), equip]
		if desc.has("modifier"):
			var mod_line := attrib_lines([desc.modifier], text.get("descript", {}), ctx).strip_edges()
			if mod_line != "":
				out += _s(strings, "G_Skills_76", "Trang bị gồm có:") + mod_line + "\n"
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
	if not weapon and desc.get("has_next", false):
		out += _s(strings, "G_Skills_44", "\n<color=Red> Đẳng cấp tiếp theo \n")
		out += level_lines(desc.get("next", {}), text, ctx)
	return out


static func _has_attrib(lv: Dictionary, name: String) -> bool:
	for a in lv.get("attribs", []):
		if str(a.get("name", "")) == name:
			return true
	return false
