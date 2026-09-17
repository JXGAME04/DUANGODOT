# SceneMath.cpp of the old client: the point/line relations its object sorting tree
# (KIpoTree) is built from.  Coordinates are integer *scene units* (y NOT halved), exactly as
# in the old code, so thresholds such as TOGGLE_VALUE_0_1 keep their meaning.
extends RefCounted

enum { UP, ON, DOWN, CROSS }   # RELATION_ENUM: UP = behind the line (smaller y), DOWN = in front

const TOGGLE := 10             # TOGGLE_VALUE_0_1
const LINKABLE_SLOPE := 0.01   # LINE_LINKABLE_SLOPE_RANGE
const LINKABLE_NODICAL := 16.0 # LINE_LINKABLE_NODICALY_RANGE


# SM_Distance_PointLine
static func distance_point_line(p: Vector2i, l1: Vector2i, l2: Vector2i) -> int:
	var x1 := l1.x - p.x
	var y1 := l1.y - p.y
	var x2 := l2.x - p.x
	var y2 := l2.y - p.y
	var len2 := (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)
	if len2 == 0:
		return 0
	return int(absf(float(x1 * y2 - x2 * y1)) / sqrt(float(len2)))


# SM_Relation_PointLine: which side of the line l1 -> l2 the point lies on.
static func relation_point_line(p: Vector2i, l1: Vector2i, l2: Vector2i) -> int:
	var p1 := l1 - p
	if p1.x * p1.x <= TOGGLE and p1.y * p1.y <= TOGGLE:
		return ON
	var p2 := l2 - p
	if p2.x * p2.x <= TOGGLE and p2.y * p2.y <= TOGGLE:
		return ON
	if p1.x > p2.x:
		var t := p1
		p1 = p2
		p2 = t
	var f := p1.x * p2.y - p2.x * p1.y
	if f <= TOGGLE and f >= -TOGGLE:
		return ON
	return UP if f < 0 else DOWN


# SM_Relation_LineLine_CheckCut: relation of the line a1 -> a2 to the line b1 -> b2, with the
# crossing point when they cut.  Returns {"rel": int, "poi": Vector2i}.
static func relation_line_line_check_cut(a1: Vector2i, a2: Vector2i, b1: Vector2i, b2: Vector2i) -> Dictionary:
	var r1 := relation_point_line(a1, b1, b2)
	var r2 := relation_point_line(a2, b1, b2)
	var poi := Vector2i.ZERO
	var rel: int
	if r1 == r2:
		rel = r1
	elif r1 == ON:
		rel = r2
	elif r2 == ON:
		rel = r1
	else:
		rel = CROSS
		# the old code moves the origin to a1 (x1 = y1 = 0) and solves the two line equations
		var x2 := a2.x - a1.x
		var y2 := a2.y - a1.y
		var x3 := b1.x - a1.x
		var y3 := b1.y - a1.y
		var x4 := b2.x - a1.x
		var y4 := b2.y - a1.y
		if x2 == 0 and x4 == x3:
			poi = Vector2i(0, a2.y)
		elif x2 == 0:
			poi = Vector2i(0, _div(y3 * x4 - y4 * x3, x4 - x3))
		elif x4 == x3:
			poi = Vector2i(x3, _div(x3 * y2, x2))
		elif y2 == 0 and y4 == y3:
			poi = Vector2i(a2.x, 0)
		elif y2 == 0:
			poi = Vector2i(_div(x3 * y4 - x4 * y3, y4 - y3), 0)
		elif y4 == y3:
			poi = Vector2i(_div(y3 * x2, y2), y3)
		else:
			var den := y2 * (x4 - x3) - (y4 - y3) * x2
			var num := -(x3 * y4 - x4 * y3)
			poi = Vector2i(_div(num * x2, den), _div(num * y2, den))
		poi += a1
	return {"rel": rel, "poi": poi}


# SM_IsLineLinkable: two base lines close enough in slope and intercept to count as one line.
static func is_line_linkable(slope1: float, nodical1: float, slope2: float, nodical2: float) -> bool:
	return absf(slope1 - slope2) < LINKABLE_SLOPE and absf(nodical1 - nodical2) < LINKABLE_NODICAL


# C integer division (truncates toward zero); a zero divisor yields 0 instead of crashing.
@warning_ignore("integer_division")
static func _div(a: int, b: int) -> int:
	if b == 0:
		return 0
	return a / b
