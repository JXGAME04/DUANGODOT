# KMissleEffect - one special movie of a missile at a spot (KMissle::CreateSpecialEffect, Core/Src/KMissle.cpp 1998, and
# KSkillSpecial): the collision movie (AnimFile4, MS_DoCollision) when a blow lands, drawn at (x, y - 5, z) of the missile,
# a frame every `interval` logic frames through the direction block, gone after interval x frames.  docs/CLIENT-2.0.md §11
extends Node2D

const KMissleResMath := preload("res://scenes/KMissleResMath.gd")
const KNpcResNode := preload("res://scenes/KNpcResNode.gd")
const TICK := 1.0 / 18.0

var anim := {}
var dir64 := 0
var elapsed := 0
var _sprite: Sprite2D = null
var _tick_acc := 0.0


func setup(a: Dictionary, direction: int, scene_pos: Vector2, z: int) -> void:
	anim = a
	dir64 = clampi(direction, 0, 63)
	position = Vector2(scene_pos.x, scene_pos.y * 0.5 - 5.0 - float(z))   # nSrcY - 5 of CreateSpecialEffect 2022


func _ready() -> void:
	_sprite = Sprite2D.new()
	_sprite.centered = false   # the old renderer draws a frame from its top-left: spot - centre + frame offset (KRepresentShell2.cpp 2004)
	add_child(_sprite)
	_refresh()


func _process(delta: float) -> void:
	_tick_acc += delta
	while _tick_acc >= TICK:
		_tick_acc -= TICK
		elapsed += 1
		_refresh()


func _refresh() -> void:
	if _sprite == null:
		return
	var frame := KMissleResMath.special_frame(int(anim.get("frames", 0)), int(anim.get("dirs", 0)), int(anim.get("interval", 1)), dir64, elapsed)
	if frame < 0 or str(anim.get("sprite", "")) == "":
		queue_free()
		return
	var atlas = Assets.sprite(str(anim.sprite))
	if atlas == null or atlas.frame_count() == 0:
		queue_free()
		return
	var f := clampi(frame, 0, atlas.frame_count() - 1)
	var tex: Texture2D = atlas.frame_texture(f)
	if _sprite.texture != tex:
		_sprite.texture = tex
	_sprite.position = -KNpcResNode.ref_spot(atlas.width, atlas.center_x, atlas.center_y) + atlas.frame_offset(f)
