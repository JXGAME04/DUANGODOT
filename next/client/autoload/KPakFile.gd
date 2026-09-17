# Assets - run-time loader for the exported bundles under assets/ (see docs/MAPS.md).
# The folder carries a .gdignore so the editor never imports it; PNG/JSON are read directly,
# which works in the editor, in `godot --path` runs and in exports that ship the folder.
extends Node

const ASSETS_DIR := "res://assets"

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


func assets_root() -> String:
	return ProjectSettings.globalize_path(ASSETS_DIR)


func has_map(map_id: int) -> bool:
	return FileAccess.file_exists("%s/maps/%d/map.json" % [assets_root(), map_id])


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
