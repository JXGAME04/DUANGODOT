# The drawing of the world behind one interface (ADR-008): UiGame keeps the game logic of the 2.0
# client (what an entity does, who is the target, when to ask the zone) and asks a world view to
# show it.  Two views exist: KWorldView2D (the map bundle, sprites, the (x, y/2) projection of the
# 2.0 client) and KWorldView3D (glTF map, models, a camera in space).  Every position handed across
# this line is in *scene units* of the zone; a screen point is in viewport pixels.
#
# The entity nodes the views hand back keep the 2.0 client's state machine (KNpc.gd / KObj.gd /
# KMissle.gd, Node2D): a 3D view makes them invisible (no_2d) and attaches its own picture.
extends Node

const ENTITY_DROP := 4


# True for the 3D view (UiGame swaps views when a map change crosses the 2D / 3D line).
func is_3d() -> bool:
	return false


# ---- the map ---------------------------------------------------------------------------------------

# Loads the world of Game.map_id (the bundle, or a plain grid when none exists) and sizes the camera.
# Returns true when a real map was found.
func load_map() -> bool:
	push_error("KWorldView.load_map not implemented")
	return false


# The map's display name ("" without a bundle) and the count of loaded regions / running animations (the HUD).
func map_name() -> String:
	return ""


func region_count() -> int:
	return 0


func anim_count() -> int:
	return 0


# ---- the entities ----------------------------------------------------------------------------------

# A node for an entity of the zone (EntitySpawn): a KNpc (or a KObj for a thing on the ground), set up and placed
# in the world.  The same node comes back for a second spawn of the same id (the view keeps none: UiGame does).
func add_entity(d: Dictionary, own: bool, existing: Node = null) -> Node:
	push_error("KWorldView.add_entity not implemented")
	return existing


# The entity leaves the world (EntityDespawn / a map change): the view forgets it, the caller frees it.
func remove_entity(_node: Node) -> void:
	pass


# ---- the missiles (G2C_MISSLE) --------------------------------------------------------------------

# A missile node (KMissle) flying in the world, and the movie of a blow that landed (KMissleEffect) at a spot.
func add_missle(d: Dictionary, row: Dictionary) -> Node:
	push_error("KWorldView.add_missle not implemented")
	return null


func add_missle_effect(_anim: Dictionary, _dir64: int, _scene_pos: Vector2, _z: int) -> void:
	pass


# ---- the camera and the cursor -------------------------------------------------------------------

# The camera follows the character: snap on entering, ease afterwards by time (rule 13: the same feel at 60 and 144 fps);
# UiGame calls it every frame with the frame's delta.
func follow(_own: Node, _snap: bool, _delta: float = 0.0) -> void:
	pass


# The camera lands on a scene point (after a map change, before our own EntitySpawn comes).
func center_on(_scene: Vector2) -> void:
	pass


# The entity under a viewport point (the one drawn on top wins) among UiGame's table (id -> node), null when
# the ground is there.
func pick(_screen: Vector2, _entities: Dictionary) -> Node:
	return null


# The scene point under a viewport point (the ground the cursor rests on), unclamped.
func screen_to_scene(_screen: Vector2) -> Vector2:
	return Vector2.ZERO


# Where a scene point (and a height over it, screen pixels of the 2.0 client) lands on the viewport.
func scene_to_screen(scene: Vector2, _height_px: float = 0.0) -> Vector2:
	return scene


# The mouse wheel: closer (+1) or farther (-1).
func zoom_step(_steps: int) -> void:
	pass


# ---- per frame --------------------------------------------------------------------------------------

# Streams the map around the camera, animates it (delta in seconds).
func update(_delta: float) -> void:
	pass


# The sounds placed in the world (KWavSound), null when the view has none.
func sounds():
	return null


# The camera's position for the --auto stability probe (a Vector2 or Vector3 as a string).
func camera_state() -> String:
	return ""
