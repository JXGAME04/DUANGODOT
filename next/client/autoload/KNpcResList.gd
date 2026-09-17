# KNpcResList of the old client (g_NpcResList): the npc templates (npcs.txt) and the appearance
# tables exported to assets/npcres (see jxassets export-npcres), loaded on demand and cached.
extends Node

var _bundle: Dictionary = {}   # npcres/npcs.json
var _res := {}                 # name -> res json
var _missing := {}
var _loaded := false


func _load_bundle() -> void:
	if _loaded:
		return
	_loaded = true
	var data = Assets.load_json(Assets.assets_root() + "/npcres/npcs.json")
	if data == null:
		Log.warn("npcres", "npcs.json missing, characters draw as markers", {"dir": Assets.assets_root()})
		_bundle = {}
		return
	_bundle = data
	Log.info("npcres", "npc templates loaded", {"templates": _bundle.get("templates", {}).size(),
		"actions": _bundle.get("actions", []).size()})


# A row of npcs.txt: name, res, stand_frame, walk_frame, run_frame ... ({} when unknown).
func template(id: int) -> Dictionary:
	_load_bundle()
	return _bundle.get("templates", {}).get(str(id), {})


# [Male] / [Female] of BaseValue.ini.
func player_frames(sex: int) -> Dictionary:
	_load_bundle()
	return _bundle.get("player", {}).get("female" if sex == 1 else "male", {"stand_frame": 15, "walk_frame": 15, "run_frame": 15})


func action_no(name: String) -> int:
	_load_bundle()
	return _bundle.get("actions", []).find(name)


# KNpcResList::AddNpcRes: the resource file of a character kind ({} when missing).
func res(name: String) -> Dictionary:
	if name == "":
		return {}
	if _res.has(name):
		return _res[name]
	if _missing.has(name):
		return {}
	var data = Assets.load_json(Assets.assets_root() + "/npcres/res/" + name + ".json")
	if data == null:
		_missing[name] = true
		Log.warn("npcres", "resource missing", {"name": name})
		return {}
	_res[name] = data
	return data
