# KSkillHang3D - a holder that follows a hang point of a character (a bone hinge such as sys_bd = Bip001 Spine [TK
# HangItemMgr]) by position only: sfx_object col 9 = 2 "链接到父级不同步旋转" (linked to the parent, no rotation sync) -
# the effect under it moves with the animation but keeps its own world rotation.  Gone when the hinge is (the character
# left), when its effect has ended, or when nothing was ever put under it (a delayed spawn that never came) after 5 s.
extends Node3D

var target: Node3D = null
var _had_child := false
var _age := 0.0


func _ready() -> void:
	top_level = true


func _process(delta: float) -> void:
	_age += delta
	if get_child_count() > 0:
		_had_child = true
	if target == null or not is_instance_valid(target) or not target.is_inside_tree() or (get_child_count() == 0 and (_had_child or _age > 5.0)):
		queue_free()
		return
	global_position = target.global_position
