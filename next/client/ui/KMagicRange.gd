# KMagicRange - the "[min-max]" the 2.0 client prints after a prefix / suffix line:
# KLibOfBPT::GetMagicRange (gamecl.exe 0x0063c260) walks the m_CMAIT list of the piece's position,
# equipment type, series and level (the rows of magicattrib.txt that could have been drawn:
# a rate for that type, that series or any, a level up to the piece's) and, over the rows of the
# same attribute kind, takes the lowest min and the highest max of the first parameter.  Nothing
# found is [0-0].  The rows come from items/magic.json (jxassets export-items), per table set.
extends RefCounted

const MAGIC_TYPES := 12   # MATF_CBDR of the JX2 build: the drop-rate columns

static var _sets = null    # {"v004": [[kind, pos, class, level, min, max, rate0..rate11], ...]}


static func _load() -> void:
	if _sets != null:
		return
	_sets = {}
	var raw = Assets.load_json("%s/items/magic.json" % Assets.assets_root())
	if raw is Dictionary:
		_sets = raw.get("sets", {})


static func forget() -> void:
	_sets = null


# The rows of table set `version` (0 = the newest exported)
static func rows(version: int) -> Array:
	_load()
	var key := "v%03d" % version
	if _sets.has(key):
		return _sets[key]
	# version 0 with only "base": the plain settings/item; else the newest set
	var best := ""
	for k in _sets.keys():
		if str(k).begins_with("v") and (best == "" or str(k) > best):
			best = str(k)
	if best == "":
		best = "base" if _sets.has("base") else ""
	return _sets.get(best, [])


static func range_of(version: int, kind: int, prefix: bool, detail: int, series: int, level: int) -> Array:
	var lo := 0
	var hi := 0
	var found := false
	if detail < 0 or detail >= MAGIC_TYPES or series < 0 or series > 4 or level < 1 or level > 10:
		return [0, 0]   # GetCMIT returns NULL outside the table
	for r in rows(version):
		if r.size() < 6 + detail + 1:
			continue
		if int(r[1]) != (1 if prefix else 0) or int(r[0]) != kind:
			continue
		if int(r[6 + detail]) == 0:
			continue
		var cls := int(r[2])
		if cls != -1 and cls != series:
			continue
		if int(r[3]) > level:
			continue
		var mn := int(r[4])
		var mx := int(r[5])
		if not found or mn < lo:
			lo = mn
		if mx > hi:
			hi = mx
		found = true
	return [lo, hi]
