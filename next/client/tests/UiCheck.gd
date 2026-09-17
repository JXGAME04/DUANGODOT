# Checks that the old client's windows still rebuild from assets/ui, and that the numbers the
# .ini gave survive the trip.  A scene, not a SceneTree script, because these windows need the
# Assets and Log autoloads and `godot -s` does not load them:
#
#   godot --headless --path client tests/UiCheck.tscn
#
# Exit code 0 when every check passes, 1 otherwise, 0 with a note when nothing has been exported.
extends Node

const KUiScheme := preload("res://scenes/KUiScheme.gd")

var _failed := 0
var _passed := 0


func check(cond: bool, what: String) -> void:
	if cond:
		_passed += 1
	else:
		_failed += 1
		printerr("FAIL: " + what)


func _ready() -> void:
	if not FileAccess.file_exists(Assets.assets_root() + "/ui/tao-nhan-vat/bo-cuc.json"):
		print("ui layouts not exported (jxassets export-ui), nothing to check")
		get_tree().quit(0)
		return
	# the three screens must at least compile; `godot -s` cannot check them because they use the
	# Game / Assets / Log autoloads, which only exist once a scene is running
	for path in ["res://scenes/UiLogin.gd", "res://scenes/UiSelPlayer.gd", "res://scenes/UiNewPlayer.gd"]:
		check(ResourceLoader.load(path, "GDScript", ResourceLoader.CACHE_MODE_IGNORE) != null, "%s compiles" % path)

	var host := Control.new()
	host.size = Vector2(1280, 720)
	add_child(host)

	_check_create(host)
	await _shot(host, "tao-nhan-vat")
	_check_login(host)
	await _shot(host, "dang-nhap")
	_check_select(host)
	await _shot(host, "chon-nhan-vat")

	print("ui checks: %d passed, %d failed" % [_passed, _failed])
	get_tree().quit(0 if _failed == 0 else 1)


# With a window (not --headless), save what each screen looks like: the fastest way for a person
# to see whether the old layout really came through.  The screen just built is the last child.
func _shot(host: Control, shot_name: String) -> void:
	if DisplayServer.get_name() == "headless" or host.get_child_count() == 0:
		return
	for i in host.get_child_count():
		host.get_child(i).visible = i == host.get_child_count() - 1
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	var path := "user://logs/ui_%s.png" % shot_name
	DirAccess.make_dir_recursive_absolute("user://logs")
	var err := get_viewport().get_texture().get_image().save_png(path)
	print("shot %s -> %s%s" % [shot_name, ProjectSettings.globalize_path(path), "" if err == OK else " FAILED"])


# The VLTK 2.0 create screen, the one screen the 2.0 client really carries: a 1024x768 canvas,
# five upright tabs 103x38 down the left edge, the two figures at -38 and +262.
func _check_create(host: Control) -> void:
	var s := KUiScheme.new()
	check(s.build(host, "tao-nhan-vat"), "the create window builds")
	check(int(s.screen.get("width", 0)) == 1024 and int(s.screen.get("height", 0)) == 768,
		"the 2.0 create window is 1024x768, not the 800x600 of JX1: %sx%s" % [s.screen.get("width"), s.screen.get("height")])
	var cname := s.widget("name") as LineEdit
	check(cname != null and cname.max_length == 16, "the name box keeps MaxLen=16")
	var tops := {"gold": 78, "wood": 116, "water": 154, "fire": 192, "earth": 230}
	for element in tops:
		var b := s.widget(element) as TextureButton
		check(b != null and b.toggle_mode, "%s is a two-state tab" % element)
		if b != null:
			check(b.position == Vector2(0, tops[element]), "%s sits at 0,%d: %s" % [element, tops[element], b.position])
			check(b.size == Vector2(103, 38), "%s is 103x38: %s" % [element, b.size])
	var male := s.widget("male")
	var female := s.widget("female")
	check(male != null and male.position.x == -38, "the man stands at -38")
	check(female != null and female.position.x == 262, "the woman stands at 262")
	check(s.portrait(2, 1, 1) != null, "the water female figure is there")
	check(s.portrait(0, 0, 1) != null, "the metal male figure is there")
	check(s.root.get_node_or_null("Backdrop") != null, "the create window sits on the login backdrop")
	check(s.missing.is_empty(), "every picture of the create window loaded: %s" % str(s.missing))


func _check_login(host: Control) -> void:
	var s := KUiScheme.new()
	if not s.build(host, "dang-nhap"):
		print("khong co bo cuc dang nhap trong client nay, bo qua")
		return
	var account := s.widget("account") as LineEdit
	check(account != null, "the login window has an account box")
	if account != null:
		check(account.position == Vector2(351, 238), "the account box keeps the old spot: %s" % account.position)
		check(account.max_length == 80, "the account box keeps MaxLen=80: %d" % account.max_length)
	var password := s.widget("password") as LineEdit
	check(password != null and password.secret, "the password box hides what is typed")
	var button := s.widget("login") as TextureButton
	check(button != null and button.texture_normal != null, "the login button has its picture")
	if button != null:
		check(button.texture_pressed != null and button.texture_hover != null, "the login button has all three states")
	check(s.missing.is_empty(), "every picture of the login window loaded: %s" % str(s.missing))


func _check_select(host: Control) -> void:
	var s := KUiScheme.new()
	if not s.build(host, "chon-nhan-vat"):
		print("khong co bo cuc chon nhan vat trong client nay, bo qua")
		return
	check(s.portrait(0, 0, 1) != null, "the metal male figure is there")
	check(s.portrait(4, 1, 0) != null, "the earth female portrait is there")
	check(s.widget("player") != null, "the list window has a place for the figures")
	for b in ["ok", "cancel", "new", "del"]:
		check(s.widget(b) is TextureButton, "%s is a button" % b)
