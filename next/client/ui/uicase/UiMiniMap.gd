# UiMiniMap - the minimap of the VLTK 2.0 client (KUiMiniMap, 小地图_小.ini; gamecl.exe Initialize 0x004C4BB0 reads
# [MiniMap] [NameShadow] [SceneName] [ScenePos] [SwitchBtn] [WorldMapBtn] [CaveMapBtn] [BtnFlag] [CityInfo1/2] [WayFinding]
# [Line] Color [MapRect] Left/Top/Width/Height; the drawing is the core's KScenePlaceMapC (the same code base as the
# owner's SwordOnline source: ScenePlaceMapC.cpp) with the exe's "%s24.jpg" 0x7b580c):
#   - the picture is "<map root>24.jpg" of data/minimap.pak (jxassets export-minimap -> maps/<id>/minimap.jpg); one
#     region of the map is 32 x 32 of its pixels (MAP_A_REGION_NUM_MAP_PIXEL_H/V): a pixel = 16 scene units across,
#     32 down; the picture's top-left region = [MAIN] MapLTRegionIndex, else the rect of the .wor (map.json region_left/top);
#   - [MapRect] (19,1) 128 x 128 shows the picture around the focus (the character), the focus clamped so the window
#     never leaves the picture (m_FocusLimit), a black 1 px frame around it;
#   - 3 x 3 spots (RU_T_SHADOW) at (px - 1, py - 1): self SelfColor 255,255,0, teammates TeammateColor 0,255,0, other
#     players PlayerColor 255,72,0, fighting npcs (kind_normal) FightNpcColor 165,48,255, dialogue npcs NormalNpcColor
#     255,255,255 (\Ui\Setting.ini [Map]); a green 0xff00ff00 line to the flagged target (not yet: no flag);
#   - [SceneName] the map name in green above, [ScenePos] "x/y" of the focus (the 2.0 coordinate = world scene units / 32,
#     the way NewWorld(map, x, y) counts; the client's scene_pos is local to the map's first region).
# A 3D map (assets3d/maps/<id>/map3d.json "minimap": {file, left, top, right, bottom} in scene units) shows its own
# picture with the same window; a map without a picture shows the spots on black.
extends "res://ui/elem/KWndWindow.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndImage := preload("res://ui/elem/KWndImage.gd")

const SCHEME := "ban-do-nho"
const REGION_W := 512
const REGION_H := 1024
const PX_PER_REGION := 32          # MAP_A_REGION_NUM_MAP_PIXEL_H / _V
const UNITS_PER_PX_X := REGION_W / PX_PER_REGION   # 16
const UNITS_PER_PX_Y := REGION_H / PX_PER_REGION   # 32
# \Ui\Setting.ini [Map] of the 2.0 client
const COLOR_SELF := Color8(255, 255, 0)
const COLOR_TEAMMATE := Color8(0, 255, 0)
const COLOR_PLAYER := Color8(255, 72, 0)
const COLOR_FIGHT_NPC := Color8(165, 48, 255)
const COLOR_NORMAL_NPC := Color8(255, 255, 255)
const ENTITY_PLAYER := 1
const ENTITY_NPC := 2
const ENTITY_MONSTER := 3

var _ini: KUiScheme = null
var _frame: KWndImage = null
var _scene_name: KWndText = null
var _scene_pos: KWndText = null
var _view: Control = null          # the [MapRect] drawing area
var _map_rect := Rect2i(19, 1, 128, 128)
var _texture: Texture2D = null
var _pic_origin := Vector2.ZERO    # scene units of the picture's top-left
var _pic_scale := Vector2(UNITS_PER_PX_X, UNITS_PER_PX_Y)   # scene units per picture pixel
var _map_id := -1
var _scene_origin := Vector2.ZERO  # scene units of the map's first region: the 2.0 coordinate text counts from the world's origin
var _entities: Dictionary = {}     # entity_id -> node (KNpc / KObj), the scene's table
var _own: Node = null
var _tick := 0.0


func load_scheme(screen: Vector2i) -> bool:
	_ini = KUiScheme.open(SCHEME)
	if _ini == null or not init_from(_ini, "MiniMap"):
		return false
	name = "UiMiniMap"
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	# [MiniMap] Left=876;+35 Top=15 on the 1024 x 768 theme: the window keeps its distance from the right edge
	# ("+35" of the 2.0 syntax not decoded [tự chọn: anchor right])
	position = Vector2(screen.x - (_ini.screen_size().x - position.x), position.y)
	_frame = KWndImage.new()
	add_child(_frame)
	_frame.init_from(_ini, "MiniMap")
	_frame.position = Vector2.ZERO
	_frame.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_map_rect = Rect2i(_ini.get_integer("MapRect", "Left", 19), _ini.get_integer("MapRect", "Top", 1),
		_ini.get_integer("MapRect", "Width", 128), _ini.get_integer("MapRect", "Height", 128))
	_view = Control.new()
	_view.name = "MapRect"
	_view.position = Vector2(_map_rect.position)
	_view.size = Vector2(_map_rect.size)
	_view.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_view.draw.connect(_draw_map)
	add_child(_view)   # over the frame: KUiMiniMap::PaintWindow paints the frame first, then the map (GSMOI_PAINT_SCENE_MAP)
	_scene_name = KWndText.new()
	add_child(_scene_name)
	_scene_name.init_from(_ini, "SceneName")
	_scene_name.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_scene_pos = KWndText.new()
	add_child(_scene_pos)
	_scene_pos.init_from(_ini, "ScenePos")
	_scene_pos.mouse_filter = Control.MOUSE_FILTER_IGNORE
	for btn in ["SwitchBtn", "WorldMapBtn", "CaveMapBtn", "BtnFlag", "WayFinding"]:
		if _ini.has_section(btn):
			var b := KWndImage.new()
			add_child(b)
			b.init_from(_ini, btn)
			b.mouse_filter = Control.MOUSE_FILTER_IGNORE
	Log.info("ui", "minimap window", {"rect": _map_rect, "at": position})
	return true


# The scene's tables: who is where (UiGame keeps them)
func bind(entities: Dictionary) -> void:
	_entities = entities


func set_own(node: Node) -> void:
	_own = node


# A new map: its picture (2.0: maps/<id>/minimap.jpg; 3D: map3d.json "minimap"), its name
func set_map(map_id: int, map_name: String, info: Dictionary) -> void:
	_map_id = map_id
	_texture = null
	_pic_origin = Vector2.ZERO
	_pic_scale = Vector2(UNITS_PER_PX_X, UNITS_PER_PX_Y)
	var mm = info.get("minimap", null)
	if mm is Dictionary and mm.has("file"):
		# a 3D map: the picture spans [left, top]..[right, bottom] scene units
		var p := "%s/maps/%d/%s" % [Assets.assets3d_root(), map_id, str(mm["file"])]
		var img := Image.load_from_file(p)
		if img != null and img.get_width() > 0:
			_texture = ImageTexture.create_from_image(img)
			_pic_origin = Vector2(float(mm.get("left", 0.0)), float(mm.get("top", 0.0)))
			_pic_scale = Vector2((float(mm.get("right", 0.0)) - _pic_origin.x) / img.get_width(),
				(float(mm.get("bottom", 0.0)) - _pic_origin.y) / img.get_height())
	else:
		var p := "%s/maps/%d/minimap.jpg" % [Assets.assets_root(), map_id]
		if FileAccess.file_exists(p):
			var img := Image.load_from_file(p)
			if img != null and img.get_width() > 0:
				_texture = ImageTexture.create_from_image(img)
				# scene_pos of the client is local to the map's first region (KNpc.scene_pos): the picture's top-left
				# region is that same region (map.json rect; MapLTRegionIndex when the .wor names one) -> origin 0
				_pic_origin = Vector2.ZERO
	_scene_origin = Vector2(float(int(info.get("region_left", 0)) * REGION_W), float(int(info.get("region_top", 0)) * REGION_H))
	_scene_name.text = map_name
	_view.queue_redraw()


func _process(delta: float) -> void:
	_tick -= delta
	if _tick > 0.0:
		return
	_tick = 1.0 / 18.0   # a game frame, like the 2.0 Breathe
	if _own != null and is_instance_valid(_own):
		var sp: Vector2 = _own.scene_pos
		_scene_pos.text = "%d/%d" % [int(sp.x + _scene_origin.x) / 32, int(sp.y + _scene_origin.y) / 32]
	_view.queue_redraw()


func _to_px(scene: Vector2) -> Vector2:
	return Vector2((scene.x - _pic_origin.x) / _pic_scale.x, (scene.y - _pic_origin.y) / _pic_scale.y)


func _draw_map() -> void:
	var w := float(_map_rect.size.x)
	var h := float(_map_rect.size.y)
	var focus := Vector2.ZERO
	if _own != null and is_instance_valid(_own):
		focus = _to_px(_own.scene_pos)
	# the window around the focus, clamped inside the picture (m_FocusLimit)
	var lt := focus - Vector2(w, h) / 2.0
	if _texture != null:
		lt.x = clampf(lt.x, 0.0, maxf(0.0, _texture.get_width() - w))
		lt.y = clampf(lt.y, 0.0, maxf(0.0, _texture.get_height() - h))
	lt = lt.floor()
	_view.draw_rect(Rect2(0, 0, w, h), Color.BLACK)
	if _texture != null:
		var src := Rect2(lt, Vector2(w, h)).intersection(Rect2(0, 0, _texture.get_width(), _texture.get_height()))
		if src.size.x > 0 and src.size.y > 0:
			_view.draw_texture_rect_region(_texture, Rect2(src.position - lt, src.size), src)
	# the spots: others first (PaintCharacters), self last
	for id in _entities:
		var node = _entities[id]
		if node == null or not is_instance_valid(node) or node == _own or node.get("scene_pos") == null:
			continue
		var t := int(node.get("entity_type")) if node.get("entity_type") != null else 0
		var c := Color.TRANSPARENT
		match t:
			ENTITY_PLAYER:
				c = COLOR_TEAMMATE if bool(node.get("teammate")) else COLOR_PLAYER
			ENTITY_MONSTER:
				c = COLOR_FIGHT_NPC
			ENTITY_NPC:
				c = COLOR_NORMAL_NPC
			_:
				continue
		var p: Vector2 = _to_px(node.scene_pos) - lt
		if p.x < 0 or p.y < 0 or p.x >= w or p.y >= h:
			continue
		_view.draw_rect(Rect2(p.floor() - Vector2.ONE, Vector2(3, 3)), c)
	if _own != null and is_instance_valid(_own):
		var p: Vector2 = focus - lt
		_view.draw_rect(Rect2(p.floor() - Vector2.ONE, Vector2(3, 3)), COLOR_SELF)
	# the 1 px black frame (PaintWindow's RU_T_RECT around the map)
	_view.draw_rect(Rect2(-1, -1, w + 2, h + 2), Color.BLACK, false, 1.0)
