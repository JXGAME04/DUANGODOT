# Fallback ground when the zone has no map bundle: a grid of AOI cells in screen pixels.
extends Node2D

const CELL := 512

var size := Vector2(8192, 4096)


func _draw() -> void:
	var grid := Color(1, 1, 1, 0.12)
	var x := 0.0
	while x <= size.x:
		draw_line(Vector2(x, 0), Vector2(x, size.y), grid, 1.0)
		x += CELL
	var y := 0.0
	while y <= size.y:
		draw_line(Vector2(0, y), Vector2(size.x, y), grid, 1.0)
		y += CELL
	draw_rect(Rect2(Vector2.ZERO, size), Color(1, 1, 1, 0.4), false, 3.0)
