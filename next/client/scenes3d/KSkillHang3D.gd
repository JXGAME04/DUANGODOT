# KSkillHang3D - a holder that follows a hang point of a character (a bone hinge such as sys_bd = Bip001 Spine [TK
# HangItemMgr]) by position only: sfx_object col 9 = 2 "链接到父级不同步旋转" (linked to the parent, no rotation sync) -
# the effect under it moves with the animation but keeps its own world rotation.  Gone when the hinge is (the character
# left) or when its effect has ended.
extends Node3D

var target: Node3D = null


func _ready() -> void:
	top_level = true


func _process(_delta: float) -> void:
	if target == null or not is_instance_valid(target) or not target.is_inside_tree() or get_child_count() == 0:
		queue_free()
		return
	global_position = target.global_position
