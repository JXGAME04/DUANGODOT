# Assets - run-time loader for the exported bundles under assets/ (see docs/MAPS.md).
# The folder carries a .gdignore so the editor never imports it; PNG/JSON are read directly,
# which works in the editor, in `godot --path` runs and in exports that ship the folder.
extends Node

const ASSETS_DIR := "res://assets"
const ASSETS3D_DIR := "res://assets3d"   # the 3D bundles (ADR-008): maps/<id>/map3d.json, npc/, weapon/, sfx/

class SpriteAtlas:
	var id: String
	var texture: Texture2D
	var frames: Array = []        # [{x,y,w,h,ox,oy}, ...]
	var width := 0
	var height := 0
	var center_x := 0
	var center_y := 0
	var directions := 1
	var interval := 1
	var _atlas_cache := {}

	func frame_count() -> int:
		return frames.size()

	# AtlasTexture for one frame (cached).
	func frame_texture(frame: int) -> AtlasTexture:
		if frames.is_empty():
			return null
		frame = clampi(frame, 0, frames.size() - 1)
		if _atlas_cache.has(frame):
			return _atlas_cache[frame]
		var f: Dictionary = frames[frame]
		var at := AtlasTexture.new()
		at.atlas = texture
		at.region = Rect2(f.x, f.y, f.w, f.h)
		_atlas_cache[frame] = at
		return at

	func frame_offset(frame: int) -> Vector2:
		if frames.is_empty():
			return Vector2.ZERO
		var f: Dictionary = frames[clampi(frame, 0, frames.size() - 1)]
		return Vector2(f.ox, f.oy)

	# The columns [x0, x1) of one frame (a tree object cut by the sorting tree), cached.
	func frame_part_texture(frame: int, x0: int, x1: int) -> AtlasTexture:
		if frames.is_empty():
			return null
		frame = clampi(frame, 0, frames.size() - 1)
		var key := "%d:%d:%d" % [frame, x0, x1]
		if _part_cache.has(key):
			return _part_cache[key]
		var f: Dictionary = frames[frame]
		var at := AtlasTexture.new()
		at.atlas = texture
		at.region = Rect2(f.x + x0, f.y, maxi(x1 - x0, 0), f.h)
		_part_cache[key] = at
		return at

	var _part_cache := {}


var _sprites := {}          # id -> SpriteAtlas
var _missing := {}
var loaded_bytes := 0
var _sounds := {}           # game path (lower case) -> AudioStream (KSoundCache of the old engine)
var _sound_files := {}      # sounds/sounds.json: game path -> file name
var _sound_index_loaded := false


# Drop the texture cache before the renderer goes away (otherwise Godot reports leaked textures).
func _exit_tree() -> void:
	_sprites.clear()
	_sounds.clear()
	_ui_images.clear()
	_ui_pixels.clear()


func assets_root() -> String:
	return ProjectSettings.globalize_path(ASSETS_DIR)


func has_map(map_id: int) -> bool:
	return FileAccess.file_exists("%s/maps/%d/map.json" % [assets_root(), map_id])


func assets3d_root() -> String:
	return ProjectSettings.globalize_path(ASSETS3D_DIR)


# A 3D bundle of the map (docs/3D-QUY-UOC.md §6): the world is drawn by KWorldView3D
func has_map3d(map_id: int) -> bool:
	return FileAccess.file_exists("%s/maps/%d/map3d.json" % [assets3d_root(), map_id])


func map3d_info(map_id: int):
	return load_json("%s/maps/%d/map3d.json" % [assets3d_root(), map_id])


func load_json(path: String):
	var f := FileAccess.open(path, FileAccess.READ)
	if f == null:
		Log.warn("asset", "json missing", {"path": path})
		return null
	var data = JSON.parse_string(f.get_as_text())
	f.close()
	if data == null:
		Log.error("asset", "json parse failed", {"path": path})
	return data


func map_info(map_id: int):
	return load_json("%s/maps/%d/map.json" % [assets_root(), map_id])


func region(map_id: int, key: String):
	return load_json("%s/maps/%d/r%s.json" % [assets_root(), map_id, key])


# Loads sprites/<id>.png + .json once; returns null when the sprite is missing.
func sprite(id: String) -> SpriteAtlas:
	if _sprites.has(id):
		return _sprites[id]
	if _missing.has(id):
		return null
	var base := "%s/sprites/%s" % [assets_root(), id]
	var meta = load_json(base + ".json")
	if meta == null:
		_missing[id] = true
		return null
	var img := Image.new()
	var err := img.load(base + ".png")
	if err != OK:
		Log.error("asset", "png load failed", {"path": base + ".png", "error": error_string(err)})
		_missing[id] = true
		return null
	var s := SpriteAtlas.new()
	s.id = id
	s.texture = ImageTexture.create_from_image(img)
	s.frames = meta.get("frames", [])
	s.width = int(meta.get("width", 0))
	s.height = int(meta.get("height", 0))
	s.center_x = int(meta.get("center_x", 0))
	s.center_y = int(meta.get("center_y", 0))
	s.directions = maxi(1, int(meta.get("directions", 1)))
	s.interval = maxi(1, int(meta.get("interval", 1)))
	_sprites[id] = s
	loaded_bytes += img.get_data_size()
	Log.trace("asset", "sprite loaded", {"id": id, "frames": s.frames.size(), "size": "%dx%d" % [img.get_width(), img.get_height()]})
	return s


func stats() -> Dictionary:
	return {"sprites": _sprites.size(), "missing": _missing.size(), "mb": loaded_bytes / 1048576}


# One of the old client's windows, as jxassets export-ui wrote it: assets/ui/<man>/bo-cuc.json.
func ui_screen(name: String):
	return load_json("%s/ui/%s/bo-cuc.json" % [assets_root(), name])


# A table of the old client that is data, not a layout: assets/ui/du-lieu/<ten>.json
# (tan-thu-thon, ngu-hanh, thong-diep, chuoi-client).
func ui_data(name: String):
	return load_json("%s/ui/du-lieu/%s.json" % [assets_root(), name])


# One picture of a window, by the path the layout gives below assets/ui ("dang-nhap/nut-huy.png").
# A sprite's atlas or a .jpg; the file is named after the widget it belongs to, never after a hash.
func ui_texture(file: String) -> Texture2D:
	if file == "":
		return null
	if _ui_images.has(file):
		return _ui_images[file]
	var path := "%s/ui/%s" % [assets_root(), file]
	var img := Image.new()
	var err := img.load(path)
	if err != OK:
		Log.error("asset", "ui picture load failed", {"path": path, "error": error_string(err)})
		_ui_images[file] = null
		return null
	var tex := ImageTexture.create_from_image(img)
	_ui_images[file] = tex
	_ui_pixels[tex] = img
	loaded_bytes += img.get_data_size()
	Log.trace("asset", "ui picture loaded", {"file": file, "size": "%dx%d" % [img.get_width(), img.get_height()]})
	return tex


# The pixels behind a ui_texture, for windows that let the mouse through their transparent parts.
func ui_image_data(tex: Texture2D) -> Image:
	return _ui_pixels.get(tex, null)


# The picture of an item, by the game path its table row names (jxassets export-item-images:
# assets/items/images.json + items/images/...png).  A KUiImage like a window's picture, null when the
# old client had no such sprite (the bag then shows the name).
func item_image(game_path: String):
	if game_path == "":
		return null
	if _item_images.has(game_path):
		return _item_images[game_path]
	if _item_index == null:
		var raw = load_json("%s/items/images.json" % assets_root())
		_item_index = raw.get("images", {}) if raw is Dictionary else {}
	var entry = _item_index.get(game_path, null)
	var img = null
	if entry is Dictionary:
		var tex := _item_texture(str(entry.get("file", "")))
		if tex != null:
			img = preload("res://ui/KUiImage.gd").new()
			img.game_path = game_path
			img.texture = tex
			img.box = Vector2i(int(entry.get("width", 0)), int(entry.get("height", 0)))
			img.interval_ms = int(entry.get("interval", 0))
			img.frames = entry.get("frames", [])
			if img.frames.is_empty():
				var sz := tex.get_size()
				img.frames = [{"x": 0, "y": 0, "w": int(sz.x), "h": int(sz.y), "ox": 0, "oy": 0}]
	_item_images[game_path] = img
	return img


func _item_texture(file: String) -> Texture2D:
	if file == "":
		return null
	var path := "%s/items/%s" % [assets_root(), file]
	var img := Image.new()
	var err := img.load(path)
	if err != OK:
		Log.warn("asset", "item picture load failed", {"path": path, "error": error_string(err)})
		return null
	var tex := ImageTexture.create_from_image(img)
	_ui_pixels[tex] = img
	loaded_bytes += img.get_data_size()
	return tex


var _item_images := {}
var _item_index = null


# One row of ObjData.txt as jxassets export-objdata wrote it (assets/objdata.json): what a thing
# on the ground looks like.  {} when the table is not exported or the row is unknown.
func objdata_row(id: int) -> Dictionary:
	if _objdata == null:
		var raw = load_json("%s/objdata.json" % assets_root())
		_objdata = raw.get("objects", {}) if raw is Dictionary else {}
	var row = _objdata.get(str(id), null)
	return row if row is Dictionary else {}


var _objdata = null


var _ui_images := {}
var _ui_pixels := {}


# KSoundCache::GetNode: the .wav a game path names (jxassets export-sounds: sounds/sounds.json + sounds/<id>.wav),
# loaded once; null when the archives had no such file.
func sound(game_path: String) -> AudioStream:
	var key := game_path.strip_edges().to_lower()
	if key == "" or key == "0":
		return null
	if _sounds.has(key):
		return _sounds[key]
	if not _sound_index_loaded:
		_sound_index_loaded = true
		var d = load_json("%s/sounds/sounds.json" % assets_root())
		if d != null:
			_sound_files = d.get("files", {})
	var file := str(_sound_files.get(key, ""))
	var stream: AudioStream = null
	if file != "":
		var path := "%s/sounds/%s" % [assets_root(), file]
		if file.ends_with(".mp3"):
			stream = AudioStreamMP3.load_from_file(path)
		else:
			stream = AudioStreamWAV.load_from_file(path)
		if stream == null:
			Log.warn("asset", "sound load failed", {"path": path})
	elif not _sound_files.is_empty():
		Log.debug("asset", "sound not exported", {"path": game_path})
	_sounds[key] = stream
	return stream
