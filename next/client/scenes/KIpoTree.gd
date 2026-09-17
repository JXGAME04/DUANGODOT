# KIpoTree.h/.cpp of the old client (without its lighting): the object sorting tree that
# decides what is drawn over what.  The main branch is grown from the tree-sorted objects;
# a scene without any uses the default branch, whose "permanent" horizontal line lies well
# above the view so everything hangs on its front side.
extends RefCounted

const KIpotBranch := preload("res://scenes/KIpotBranch.gd")
const KIpotLeaf := preload("res://scenes/KIpotLeaf.gd")

var main_branch = null
var default_branch = KIpotBranch.new()


# KIpoTree::SetPermanentBranchPos
func set_permanent_branch_pos(left_x: int, right_x: int, y: int) -> void:
	var p1 := Vector2i(mini(left_x, right_x), y)
	var p2 := Vector2i(maxi(left_x, right_x), y)
	if left_x == right_x:
		p2.x = right_x + 2048
	default_branch.set_line(p1, p2)


# KIpoTree::AddBranch
func add_branch(obj) -> void:
	if main_branch:
		main_branch.add_branch(obj)
	elif obj:
		main_branch = KIpotBranch.new()
		main_branch.add_a_object(obj)


func add_leaf_line(leaf) -> void:
	_root().add_leaf_line(leaf)


func add_leaf_point(leaf) -> void:
	_root().add_leaf_point(leaf)


# KIpoTree::PluckRto
func pluck_rto(leaf) -> void:
	leaf.pluck()


# KIpoTree::Fell: drops the whole tree (the scene re-adds its runtime leaves afterwards).
func fell() -> void:
	if main_branch:
		main_branch.clear()
		main_branch = null
	default_branch.clear()


# KIpoTree::Paint(IPOT_RL_OBJECT): every leaf and tree object in draw order.
func paint(out: Array) -> void:
	_root().paint_object_layer(out)


func _root():
	return main_branch if main_branch else default_branch
