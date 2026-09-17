# KIpotLeaf.h/.cpp of the old client: a leaf of the object sorting tree.  A leaf is a map
# object sorted by its foot point (POINT) or its base line (LINE / TREE), or a runtime object
# (a character, KIpotRuntimeObj).  Leaves hang in singly linked "brother" lists; a line leaf
# owns two child lists: behind its base line (lchild) and in front of it (rchild).
extends RefCounted

enum Type { BUILDIN, RUNTIME }   # IPOTLEAF_TYPE
enum Sort { POINT, LINE, TREE }  # SPBIO_P_SORTMANNER_*
const Y_ADJUST := 6              # POINT_LEAF_Y_ADJUST_VALUE

var type: int = Type.BUILDIN
var sort: int = Sort.POINT
var position := Vector2i.ZERO    # oPos1 in scene units (+ Y_ADJUST for point leaves / characters)
var end_pos := Vector2i.ZERO     # oPos2 (line and tree leaves)
var line_start := Vector2i.ZERO  # the record's full base line: a cut part keeps it for the image cut
var line_end := Vector2i.ZERO
var angle := 0.0                 # fAngleXY (tree leaves)
var nodical := 0.0               # fNodicalY
var item = null                  # what this leaf orders: the Sprite2D / entity node (or anything in tests)
var image = null                 # SpriteAtlas of a map object (uImage in the old struct)
var frame := 0
var base := Vector2.ZERO         # top-left of the full image in screen px
var brother = null               # next leaf in the list
var lchild = null                # RELATION_UP side of a line leaf
var rchild = null                # RELATION_DOWN side
var is_clone := false
var img_part := false            # tree object cut at a crossing: draw only [position.x, end_pos.x]
# runtime leaves remember where they hang so pluck() can unhook them (KIpotRuntimeObj)
var parent_branch = null
var parent_leaf = null
var ahead_brother = null


# Restores the record's base line before the tree is rebuilt (a previous build may have cut it).
func reset() -> void:
	position = line_start
	if type == Type.BUILDIN and sort == Sort.POINT:
		position.y += Y_ADJUST
	end_pos = line_end
	img_part = false
	brother = null
	lchild = null
	rchild = null
	parent_branch = null
	parent_leaf = null
	ahead_brother = null


# KIpotBuildinObj::Clone: splits a tree object at `division`; self keeps the left part and
# the returned clone the right part (null when there is nothing to the right).
func clone_at(division: Vector2i):
	if division.x == end_pos.x:
		return null
	var c = get_script().new()
	c.type = type
	c.sort = sort
	c.angle = angle
	c.nodical = nodical
	c.item = item
	c.image = image
	c.frame = frame
	c.base = base
	c.line_start = line_start
	c.line_end = line_end
	c.position = division
	c.end_pos = end_pos
	c.is_clone = true
	c.img_part = true
	end_pos = division
	img_part = true
	return c


# KIpotRuntimeObj::Pluck: unhooks a runtime leaf from wherever it hangs.
func pluck() -> void:
	if ahead_brother:
		ahead_brother.brother = brother
	if parent_branch:
		if parent_branch.leafs[0] == self:
			parent_branch.leafs[0] = brother
		elif parent_branch.leafs[1] == self:
			parent_branch.leafs[1] = brother
		parent_branch = null
	if parent_leaf:
		if parent_leaf.lchild == self:
			parent_leaf.lchild = brother
		elif parent_leaf.rchild == self:
			parent_leaf.rchild = brother
		parent_leaf = null
	if brother and brother.type == Type.RUNTIME:
		brother.ahead_brother = ahead_brother
	ahead_brother = null
	brother = null


# KIpotLeaf_PaintObjectLayer: appends the leaves of a list in draw order.
static func paint_list(leaf, out: Array) -> void:
	while leaf:
		if leaf.lchild:
			paint_list(leaf.lchild, out)
		out.append(leaf)
		if leaf.rchild:
			paint_list(leaf.rchild, out)
		leaf = leaf.brother


# KIpotLeaf_Clear: drops the links of a list so the RefCounted leaves can be freed.
static func clear_list(leaf) -> void:
	while leaf:
		if leaf.lchild:
			clear_list(leaf.lchild)
			leaf.lchild = null
		if leaf.rchild:
			clear_list(leaf.rchild)
			leaf.rchild = null
		leaf.parent_branch = null
		leaf.parent_leaf = null
		leaf.ahead_brother = null
		var next = leaf.brother
		leaf.brother = null
		leaf = next
