# KUiScheme - builds one of the old client's windows from an exported layout.
#
# The old client read a window out of \Ui\<scheme>\<name>.ini: one section per widget with its
# rectangle, its sprite and the frame each state uses (KUiLogin::LoadScheme -> KWndButton::Init).
# jxassets export-ui turned those files into assets/ui/<name>.json; this builds them.
#
# Coordinates are the old ones, on a fixed 800x600 canvas.  fit() scales that canvas to the window
# the same way the old client did when it ran at another resolution: one factor for both axes, so
# nothing is stretched, and centred.
extends RefCounted

const CANVAS := Vector2(800, 600)

var screen: Dictionary = {}
var root: Control = null       # the 800x600 canvas; every widget is a child of it
var nodes: Dictionary = {}     # section name (lower case) -> Control
var missing: Array = []        # sections whose picture could not be loaded


# Reads assets/ui/<name>.json and builds it under `parent`.  Returns false when the layout is not
# there, so the caller can fall back to a plain screen instead of showing nothing.
func build(parent: Control, name: String) -> bool:
	var data = Assets.ui_screen(name)
	if data == null or not (data is Dictionary):
		Log.warn("ui", "layout missing, falling back", {"screen": name})
		return false
	screen = data
	var canvas := Vector2(float(screen.get("width", 800)), float(screen.get("height", 600)))
	root = Control.new()
	root.name = "Canvas"
	root.custom_minimum_size = canvas
	root.size = canvas
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	parent.add_child(root)
	_add_backdrop()
	for w in screen.get("widgets", []):
		_add_widget(w)
	fit(parent)
	if not parent.resized.is_connected(_on_parent_resized):
		parent.resized.connect(_on_parent_resized.bind(parent))
	Log.info("ui", "layout built", {"screen": name, "widgets": nodes.size(), "missing": missing.size()})
	return true


# Scales the 800x600 canvas into `parent` and centres it.
func fit(parent: Control) -> void:
	if root == null:
		return
	var view := parent.size
	if view.x <= 0.0 or view.y <= 0.0:
		view = Vector2(parent.get_viewport_rect().size)
	var canvas := root.size
	var s: float = minf(view.x / canvas.x, view.y / canvas.y)
	root.scale = Vector2(s, s)
	root.position = ((view - canvas * s) * 0.5).round()  # round, not floor: 800 * 1.2 is 960.000...1


func _on_parent_resized(parent: Control) -> void:
	fit(parent)


# Run the client with `-- --shot` to save what a screen looks like and stop there, or
# `-- --shot=<name>` to wait for one screen in particular:
#   godot --path client -- --shot
#   godot --path client -- --auto --account=... --shot=select_screen
# The picture lands in user://logs/ui_<name>.png, which is the quickest way to compare a screen
# with the old client side by side.
static func shot_if_asked(node: Node, name: String) -> void:
	var asked := false
	var want := ""
	for a in OS.get_cmdline_user_args():
		if a == "--shot":
			asked = true
		elif a.begins_with("--shot="):
			asked = true
			want = a.substr(7)
	if not asked or (want != "" and want != name):
		return
	if DisplayServer.get_name() == "headless":
		node.get_tree().quit(0)
		return
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	DirAccess.make_dir_recursive_absolute("user://logs")
	var path := "user://logs/ui_%s.png" % name
	var err := node.get_viewport().get_texture().get_image().save_png(path)
	print("shot %s -> %s%s" % [name, ProjectSettings.globalize_path(path), "" if err == OK else " FAILED"])
	node.get_tree().quit(0 if err == OK else 1)


# ---------------------------------------------------------------- widgets

func widget(name: String) -> Control:
	return nodes.get(name, null)


func rect(name: String) -> Rect2:
	var c: Control = nodes.get(name, null)
	if c == null:
		return Rect2()
	return Rect2(c.position, c.size)


# The section as the .ini had it, for the keys this builder does not turn into a node.
func section(name: String) -> Dictionary:
	for w in screen.get("widgets", []):
		if w.get("name", "") == name:
			return w
	return {}


func extra(name: String, key: String, def: String = "") -> String:
	return str(section(name).get("extra", {}).get(key, def))


# A character picture the old screen built by name: "<series>_<sex>_<n>", series metal..earth,
# sex male/female, n = 0 the small portrait, 1 the near figure, 2 the far one.
func portrait(series: int, sex: int, n: int) -> Texture2D:
	const SERIES := ["metal", "wood", "water", "fire", "earth"]
	const SEX := ["male", "female"]
	var key := "%s_%s_%d" % [SERIES[clampi(series, 0, 4)], SEX[clampi(sex, 0, 1)], clampi(n, 0, 2)]
	var id = screen.get("portraits", {}).get(key, "")
	if id == "":
		return null
	var atlas = Assets.sprite(id)
	if atlas == null:
		return null
	return atlas.frame_texture(0)


# LoginBg= names a picture of the login_bg window: the screen is drawn on top of it.  The select
# screen has no background of its own and says LoginBg=Login2, so without this it comes up bare.
func _add_backdrop() -> void:
	var widgets: Array = screen.get("widgets", [])
	if widgets.is_empty():
		return
	var which := str(widgets[0].get("extra", {}).get("loginbg", "")).to_lower()
	if which == "":
		return
	var bg = Assets.ui_screen("login_bg")
	if bg == null:
		return
	for w in bg.get("widgets", []):
		if str(w.get("name", "")) != which:
			continue
		var tex := Assets.ui_image(str(w.get("image", "")))
		if tex == null:
			return
		var tr := TextureRect.new()
		tr.name = "Backdrop"
		tr.texture = tex
		tr.expand_mode = TextureRect.EXPAND_IGNORE_SIZE
		tr.stretch_mode = TextureRect.STRETCH_SCALE
		tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
		tr.size = root.size
		tr.custom_minimum_size = root.size
		root.add_child(tr)
		return


func _add_widget(w: Dictionary) -> void:
	var name := str(w.get("name", ""))
	var r := Rect2(float(w.get("left", 0)), float(w.get("top", 0)), float(w.get("width", 0)), float(w.get("height", 0)))
	var node: Control = null
	var picture := _picture(w)
	# A picture keeps its own size: the old renderer drew the sprite as it is, at the window's
	# corner, and never stretched it into the rectangle the .ini declares.  The login panel is
	# 542x362 inside an 800x600 window; stretching it moved every box off its label.
	if picture != null and _is_button(w):
		node = _make_button(w, picture)
		r.size = picture.get_size()
	elif picture != null:
		var tr := TextureRect.new()
		tr.texture = picture
		tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
		node = tr
		r.size = picture.get_size()
	elif _is_edit(w):
		node = _make_edit(w)
	elif int(w.get("font", 0)) > 0:
		node = _make_label(w)
	else:
		node = Control.new()
		node.mouse_filter = Control.MOUSE_FILTER_IGNORE
	node.name = name.capitalize().replace(" ", "")
	node.position = r.position
	if r.size.x > 0.0 and r.size.y > 0.0:
		node.size = r.size
		node.custom_minimum_size = r.size
	root.add_child(node)
	nodes[name] = node


func _picture(w: Dictionary) -> Texture2D:
	var file := str(w.get("image", ""))
	if file != "":
		var tex := Assets.ui_image(file)
		if tex == null:
			missing.append(w.get("name", ""))
		return tex
	var id := str(w.get("sprite", ""))
	if id == "":
		return null
	var atlas = Assets.sprite(id)
	if atlas == null:
		missing.append(w.get("name", ""))
		return null
	return atlas.frame_texture(_frame(w, "up", 0))


func _is_button(w: Dictionary) -> bool:
	return w.has("up") or w.has("down") or w.has("over")


func _is_edit(w: Dictionary) -> bool:
	# KWndEdit sections say Type=; a plain text output has none
	return int(w.get("type", 0)) > 0 or int(w.get("max_len", 0)) > 0


func _frame(w: Dictionary, key: String, def: int) -> int:
	return int(w.get(key, def))


func _make_button(w: Dictionary, up: Texture2D) -> TextureButton:
	var b := TextureButton.new()
	b.texture_normal = up
	var id := str(w.get("sprite", ""))
	var atlas = Assets.sprite(id) if id != "" else null
	if atlas != null:
		if w.has("down"):
			b.texture_pressed = atlas.frame_texture(_frame(w, "down", 1))
		if w.has("over"):
			b.texture_hover = atlas.frame_texture(_frame(w, "over", 2))
	# Checkbox=1 in the .ini is a two-state button (Remember password, the five elements)
	if str(w.get("extra", {}).get("checkbox", "0")) == "1":
		b.toggle_mode = true
		if b.texture_pressed != null:
			b.texture_focused = b.texture_pressed
	return b


func _make_edit(w: Dictionary) -> LineEdit:
	var e := LineEdit.new()
	e.flat = true
	e.max_length = int(w.get("max_len", 0))
	e.secret = bool(w.get("password", false))
	e.alignment = _align(int(w.get("halign", 0)))
	_style_text(e, w)
	return e


func _make_label(w: Dictionary) -> Label:
	var l := Label.new()
	l.horizontal_alignment = _align(int(w.get("halign", 0)))
	l.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
	l.clip_text = true
	if bool(w.get("multi_line", false)):
		l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		l.vertical_alignment = VERTICAL_ALIGNMENT_TOP
	l.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_style_text(l, w)
	return l


func _style_text(c: Control, w: Dictionary) -> void:
	var size := int(w.get("font", 0))
	if size > 0:
		c.add_theme_font_size_override("font_size", size)
	var col = w.get("color", null)
	if col != null:
		c.add_theme_color_override("font_color", _color(col))
	var border = w.get("border_color", null)
	if border != null:
		# the old edit box drew a one-pixel outline in this colour under the text
		c.add_theme_color_override("font_outline_color", _color(border))
		c.add_theme_constant_override("outline_size", 1)


func _color(c: Dictionary) -> Color:
	return Color8(int(c.get("r", 255)), int(c.get("g", 255)), int(c.get("b", 255)))


func _align(halign: int) -> int:
	# the .ini says 0 left, 1 centre, 2 right
	match halign:
		1:
			return HORIZONTAL_ALIGNMENT_CENTER
		2:
			return HORIZONTAL_ALIGNMENT_RIGHT
		_:
			return HORIZONTAL_ALIGNMENT_LEFT
