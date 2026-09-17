# KIpotBranch.h/.cpp of the old client: a node of the object sorting tree.  A branch is a
# line in scene space (the base line of one or more tree-sorted objects lying on it); what
# lies behind the line (RELATION_UP) goes to side 0, what lies in front (RELATION_DOWN) to
# side 1.  Each side is either another branch or a list of leaves.  Painting is side 0, then
# the branch's own objects, then side 1.
extends RefCounted

const KSceneMath := preload("res://scenes/KSceneMath.gd")
const KIpotLeaf := preload("res://scenes/KIpotLeaf.gd")

const FLAGS := [1, 2]            # IPOT_BF_HAVE_LEFT_BRANCH, IPOT_BF_HAVE_RIGHT_BRANCH
const WIDTH_EXPAND := 13
const HEIGHT_UP_EXPAND := 3
const HEIGHT_DOWN_EXPAND := 128
const FSIZE := 7

var flag := 0
var parent = null
var sub: Array = [null, null]    # KIpotBranch per side when FLAGS[i] is set
var leafs: Array = [null, null]  # leaf list head per side otherwise
var head_point := Vector2i.ZERO  # the branch line
var end_point := Vector2i.ZERO
var first_object: KIpotLeaf = null
var object_list: Array = []      # further tree objects on the same line


func is_branch(i: int) -> bool:
	return (flag & FLAGS[i]) != 0


func set_line(p1: Vector2i, p2: Vector2i) -> void:
	head_point = p1
	end_point = p2


# KIpotBranch::PaintObjectLayer: appends everything under this branch in draw order.
func paint_object_layer(out: Array) -> void:
	for i in 2:
		if is_branch(i):
			sub[i].paint_object_layer(out)
		elif leafs[i]:
			KIpotLeaf.paint_list(leafs[i], out)
		if i == 0 and first_object:
			out.append(first_object)
			for o in object_list:
				out.append(o)


# KIpotBranch::AddAObject: another tree object lying on this branch's line.
func add_a_object(obj: KIpotLeaf) -> void:
	if first_object == null:
		first_object = obj
		head_point = obj.position
		end_point = obj.end_pos
		return
	object_list.append(obj)
	if head_point.x > obj.position.x:
		head_point = obj.position
	if end_point.x < obj.end_pos.x:
		end_point = obj.end_pos


# KIpotBranch::AddBranch: places a tree object behind, in front of or on this line; one that
# crosses the line is cut at the crossing and both parts are placed.
func add_branch(obj: KIpotLeaf) -> void:
	if obj == null or first_object == null:
		return
	var p1 := obj.position
	var p2 := obj.end_pos
	if p1.x == p2.x or head_point.x == end_point.x:
		return
	var rel: int
	var poi := Vector2i.ZERO
	if first_object.angle == obj.angle and first_object.nodical == obj.nodical:
		rel = KSceneMath.ON
	else:
		var r := KSceneMath.relation_line_line_check_cut(p1, p2, head_point, end_point)
		rel = r.rel
		poi = r.poi
	if rel != KSceneMath.CROSS:
		if rel == KSceneMath.UP:
			add_sub_branch(0, obj)
		elif rel == KSceneMath.DOWN:
			add_sub_branch(1, obj)
		else:
			add_a_object(obj)
		return
	var clone = obj.clone_at(poi)
	rel = KSceneMath.relation_point_line(p1, head_point, end_point)
	var orig_index := 1 if rel == KSceneMath.DOWN else 0
	if obj.position.x != obj.end_pos.x:
		add_sub_branch(orig_index, obj)
	if clone:
		add_sub_branch(1 - orig_index, clone)


# KIpotBranch::AddSubBranch
func add_sub_branch(i: int, obj: KIpotLeaf) -> void:
	if not is_branch(i):
		var b = get_script().new()
		b.parent = self
		b.add_a_object(obj)
		sub[i] = b
		leafs[i] = null
		flag |= FLAGS[i]
	else:
		sub[i].add_branch(obj)


# KIpotBranch::AddLeafLine: a line-sorted object goes to the side its farther end lies on.
func add_leaf_line(leaf: KIpotLeaf) -> void:
	var rel: int
	if KSceneMath.distance_point_line(leaf.position, head_point, end_point) > KSceneMath.distance_point_line(leaf.end_pos, head_point, end_point):
		rel = KSceneMath.relation_point_line(leaf.position, head_point, end_point)
	else:
		rel = KSceneMath.relation_point_line(leaf.end_pos, head_point, end_point)
	var i := 0 if rel == KSceneMath.UP else 1
	if is_branch(i):
		sub[i].add_leaf_line(leaf)
	else:
		leafs[i] = add_line_leaf_to_list(leafs[i], leaf)


# KIpotBranch::AddLeafPoint: a point-sorted object or a character.
func add_leaf_point(leaf: KIpotLeaf) -> void:
	var rel := KSceneMath.relation_point_line(leaf.position, head_point, end_point)
	var i := 0 if rel == KSceneMath.UP else 1
	if is_branch(i):
		sub[i].add_leaf_point(leaf)
	else:
		leafs[i] = _add_point_leaf_to_list(leafs[i], leaf, null)


# KIpotBranch::AddLineLeafToList; returns the (possibly new) list head.  A line lying inside
# the footprint of a listed line becomes its child, otherwise lines are kept in y order.
static func add_line_leaf_to_list(first, leaf: KIpotLeaf):
	if first == null:
		return leaf
	var lp1: Vector2i = leaf.position
	var lp2: Vector2i = leaf.end_pos
	var rel := KSceneMath.UP
	var l = first
	while l:
		var op1: Vector2i = l.position
		var op2: Vector2i = l.end_pos
		var left := mini(op1.x, op2.x) - FSIZE
		var right := maxi(op1.x, op2.x) + FSIZE
		var top := mini(op1.y, op2.y)
		var bottom := maxi(op1.y, op2.y)
		if lp1.x >= left and lp1.x <= right and lp1.y >= top and lp1.y <= bottom \
				and lp2.x >= left and lp2.x <= right and lp2.y >= top and lp2.y <= bottom:
			if KSceneMath.distance_point_line(lp1, op1, op2) > KSceneMath.distance_point_line(lp2, op1, op2):
				rel = KSceneMath.relation_point_line(lp1, op1, op2)
			else:
				rel = KSceneMath.relation_point_line(lp2, op1, op2)
			break
		l = l.brother
	if l:
		if rel == KSceneMath.DOWN:
			l.rchild = add_line_leaf_to_list(l.rchild, leaf)
		else:
			l.lchild = add_line_leaf_to_list(l.lchild, leaf)
		return first
	var y := mini(lp1.y, lp2.y)
	var prev = null
	l = first
	while l:
		if y < l.position.y and y < l.end_pos.y:
			break
		prev = l
		l = l.brother
	leaf.brother = l
	if prev == null:
		return leaf
	prev.brother = leaf
	return first


# KIpotBranch::AddPointLeafToList; returns the list head.  A point inside the footprint of a
# line leaf becomes that leaf's child (behind or in front of it); otherwise it is inserted by y.
func _add_point_leaf_to_list(first, leaf: KIpotLeaf, parent_leaf):
	if first == null:
		_hook_runtime(leaf, parent_leaf, null)
		return leaf
	var lp: Vector2i = leaf.position
	var match_l = null
	var match_rel := KSceneMath.UP
	var l = first
	while l:
		if l.type == KIpotLeaf.Type.BUILDIN and l.sort == KIpotLeaf.Sort.LINE:
			var op1: Vector2i = l.position
			var op2: Vector2i = l.end_pos
			var rel := KSceneMath.relation_point_line(lp, op1, op2)
			var left := mini(op1.x, op2.x) - WIDTH_EXPAND
			var right := maxi(op1.x, op2.x) + WIDTH_EXPAND
			var top := mini(op1.y, op2.y)
			var bottom := maxi(op1.y, op2.y)
			if rel != KSceneMath.UP:
				top -= HEIGHT_UP_EXPAND
				bottom += HEIGHT_DOWN_EXPAND
			if lp.x > left and lp.x < right and lp.y >= top and lp.y <= bottom:
				match_l = l
				match_rel = rel
				if rel == KSceneMath.UP:
					break
		l = l.brother
	if match_l:
		if match_rel == KSceneMath.DOWN:
			match_l.rchild = _add_point_leaf_to_list(match_l.rchild, leaf, match_l)
		else:
			match_l.lchild = _add_point_leaf_to_list(match_l.lchild, leaf, match_l)
		return first
	var prev = null
	l = first
	while l:
		if l.type == KIpotLeaf.Type.BUILDIN and l.sort != KIpotLeaf.Sort.POINT:
			if lp.y < l.position.y and lp.y < l.end_pos.y:
				break
		elif lp.y < l.position.y:
			break
		prev = l
		l = l.brother
	leaf.brother = l
	if prev:
		prev.brother = leaf
	var head = first if prev else leaf
	_hook_runtime(leaf, parent_leaf, prev)
	if l and l.type == KIpotLeaf.Type.RUNTIME:
		l.ahead_brother = leaf
	if head.type == KIpotLeaf.Type.RUNTIME:
		head.ahead_brother = null
	return head


func _hook_runtime(leaf: KIpotLeaf, parent_leaf, ahead) -> void:
	if leaf.type != KIpotLeaf.Type.RUNTIME:
		return
	if parent_leaf:
		leaf.parent_leaf = parent_leaf
	else:
		leaf.parent_branch = self
	leaf.ahead_brother = ahead


# KIpotBranch::Clear: drops every link (RefCounted cycles must be broken by hand).
func clear() -> void:
	parent = null
	first_object = null
	object_list.clear()
	for i in 2:
		if is_branch(i):
			sub[i].clear()
			sub[i] = null
		elif leafs[i]:
			KIpotLeaf.clear_list(leafs[i])
			leafs[i] = null
	flag = 0
