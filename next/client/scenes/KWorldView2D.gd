# The 2D world of the 2.0 client (KWorldView): the exported map bundle drawn by KScenePlaceC (or a
# grid when the zone has no bundle), entities as KNpc / KObj sprites in the map's sorting tree,
# missiles as KMissle, a Camera2D that follows the character.  Screen = (scene x, scene y / 2)
# (KRepresentShell2::CoordinateTransform).
extends "res://scenes/KWorldView.gd"

const NpcScript := preload("res://scenes/KNpc.gd")
const ObjScript := preload("res://scenes/KObj.gd")
const MissleScript := preload("res://scenes/KMissle.gd")
const MissleEffectScript := preload("res://scenes/KMissleEffect.gd")
const KWavSound := preload("res://scenes/KWavSound.gd")
const ScenePlaceScript := preload("res://scenes/KScenePlaceC.gd")
const GridScript := preload("res://scenes/KSceneGrid.gd")

var root: Node2D                 # every canvas item of the world hangs here
var _camera: Camera2D
var _map: Node2D                 # KScenePlaceC
var _entity_layer: Node2D        # y-sorted parent of entity nodes (the map's objects layer, or a local one)
var _grid: Node2D
var _sounds: Node2D              # KWavSound in the entity layer
var _zoom := 1.0
var _scene_w := 8192
var _scene_h := 8192


func _ready() -> void:
	root = Node2D.new()
	root.name = "Scene"
	add_child(root)
	_camera = Camera2D.new()
	_camera.name = "Camera"
	_camera.zoom = Vector2(_zoom, _zoom)
	root.add_child(_camera)
	_map = Node2D.new()
	_map.set_script(ScenePlaceScript)
	_map.name = "Map"
	root.add_child(_map)


static func to_screen(p: Vector2) -> Vector2:
	return Vector2(p.x, p.y * 0.5)


# Loads the map bundle of Game.map_id (or the plain grid) and sizes the camera; called on entering
# the world and again after a ChangeMap.
func load_map() -> bool:
	_scene_w = Game.scene_w if Game.scene_w > 0 else 8192
	_scene_h = Game.scene_h if Game.scene_h > 0 else 8192
	if _grid != null:
		_grid.queue_free()
		_grid = null
	if _sounds != null and is_instance_valid(_sounds):
		_sounds.get_parent().remove_child(_sounds)
	if _entity_layer != null and _entity_layer != _map.objects and is_instance_valid(_entity_layer):
		_entity_layer.queue_free()
	_entity_layer = null
	var has_map: bool = Game.map_id > 0 and Assets.has_map(Game.map_id) and bool(_map.load_map(Game.map_id))
	if has_map:
		_entity_layer = _map.objects   # ordered by the old sorting tree, see KScenePlaceC
	else:
		_map.clear()
		_grid = Node2D.new()
		_grid.name = "Grid"
		_grid.set_script(GridScript)
		_grid.size = Vector2(_scene_w, _scene_h * 0.5)
		root.add_child(_grid)
		_entity_layer = Node2D.new()
		_entity_layer.y_sort_enabled = true
		_entity_layer.z_index = 1
		root.add_child(_entity_layer)
		if Game.map_id > 0:
			Log.warn("map", "map bundle missing, drawing grid", {"map_id": Game.map_id, "dir": Assets.assets_root()})
	if _sounds == null:
		_sounds = KWavSound.new()
		_sounds.name = "Sounds"
		_sounds.stream_provider = Assets.sound
	_entity_layer.add_child(_sounds)
	_camera.limit_left = 0
	_camera.limit_top = 0
	_camera.limit_right = maxi(_scene_w, 1280)
	_camera.limit_bottom = maxi(int(_scene_h / 2), 720)
	return has_map


func map_name() -> String:
	return str(_map.info.get("name", "")) if _map.map_id > 0 else ""


func region_count() -> int:
	return _map.region_count()


func anim_count() -> int:
	return _map.anim_count()


func sounds():
	return _sounds


# ---- entities --------------------------------------------------------------------------------------

func add_entity(d: Dictionary, own: bool, existing: Node = null) -> Node:
	var node: Node2D = existing
	if node == null:
		node = Node2D.new()
		node.set_script(ObjScript if int(d.get("type", 0)) == ENTITY_DROP else NpcScript)
		_entity_layer.add_child(node)
	node.setup(d, own)
	if _map.map_id > 0:
		_map.add_entity(node)
	return node


func remove_entity(node: Node) -> void:
	if _map.map_id > 0 and node is Node2D:
		_map.remove_entity(node)


# ---- missiles --------------------------------------------------------------------------------------

func add_missle(d: Dictionary, row: Dictionary) -> Node:
	var node = MissleScript.new()
	node.sounds = _sounds
	_entity_layer.add_child(node)
	node.setup(d, row)
	return node


func add_missle_effect(anim: Dictionary, dir64: int, scene_pos: Vector2, z: int) -> void:
	var fx = MissleEffectScript.new()
	_entity_layer.add_child(fx)
	fx.setup(anim, dir64, scene_pos, z)


# ---- camera and cursor ---------------------------------------------------------------------------

func follow(own: Node, snap: bool) -> void:
	if own == null or not (own is Node2D):
		return
	# whole pixels only: a fractional camera position makes nearest-filtered tiles shimmer
	var target: Vector2 = own.position if snap else _camera.position.lerp(own.position, 0.3)
	_camera.position = target.round()


func center_on(scene: Vector2) -> void:
	_camera.position = to_screen(scene)


func _view_rect() -> Rect2:
	var size := get_viewport().get_visible_rect().size / _camera.zoom
	return Rect2(_camera.get_screen_center_position() - size * 0.5, size)


# viewport pixels -> canvas (screen-projected world) coordinates
func _canvas_point(screen: Vector2) -> Vector2:
	return get_viewport().get_canvas_transform().affine_inverse() * screen


func screen_to_scene(screen: Vector2) -> Vector2:
	var p := _canvas_point(screen)
	return Vector2(p.x, p.y * 2.0)


func scene_to_screen(scene: Vector2, height_px: float = 0.0) -> Vector2:
	return get_viewport().get_canvas_transform() * (to_screen(scene) - Vector2(0, height_px))


# The entity drawn under a viewport point (the one on top wins) - UiGame hands over its table.
func pick(screen: Vector2, entities: Dictionary) -> Node:
	var world := _canvas_point(screen)
	var best: Node2D = null
	for node in entities.values():
		if node.hit_test(node.to_local(world)) and (best == null or node.z_index > best.z_index):
			best = node
	return best


func zoom_step(steps: int) -> void:
	_zoom = clampf(_zoom * pow(1.15, steps), 0.25, 3.0)
	_camera.zoom = Vector2(_zoom, _zoom)


func update(delta: float) -> void:
	if _map.map_id > 0:
		_map.update_view(_view_rect(), delta)


func camera_state() -> String:
	return str(_camera.position)
