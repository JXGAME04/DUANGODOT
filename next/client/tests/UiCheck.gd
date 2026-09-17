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
	if not FileAccess.file_exists(Assets.assets_root() + "/ui/login.json"):
		print("ui layouts not exported (jxassets export-ui), nothing to check")
		get_tree().quit(0)
		return
	var host := Control.new()
	host.size = Vector2(1280, 720)
	add_child(host)

	# the three screens must at least compile; `godot -s` cannot check them because they use the
	# Game / Assets / Log autoloads, which only exist once a scene is running
	for path in ["res://scenes/UiLogin.gd", "res://scenes/UiSelPlayer.gd", "res://scenes/UiNewPlayer.gd"]:
		check(ResourceLoader.load(path, "GDScript", ResourceLoader.CACHE_MODE_IGNORE) != null, "%s compiles" % path)

	_check_login(host)
	await _shot(host, "login")
	_check_select(host)
	await _shot(host, "select_role")
	_check_create(host)
	await _shot(host, "new_role")

	print("ui checks: %d passed, %d failed" % [_passed, _failed])
	get_tree().quit(0 if _failed == 0 else 1)


# With a window (not --headless), save what each screen looks like: the fastest way for a person
# to see whether the old layout really came through.  The screen just built is the last child.
func _shot(host: Control, name: String) -> void:
	if DisplayServer.get_name() == "headless" or host.get_child_count() == 0:
		return
	for i in host.get_child_count():
		host.get_child(i).visible = i == host.get_child_count() - 1
	await RenderingServer.frame_post_draw
	await RenderingServer.frame_post_draw
	var path := "user://logs/ui_%s.png" % name
	DirAccess.make_dir_recursive_absolute("user://logs")
	var err := get_viewport().get_texture().get_image().save_png(path)
	print("shot %s -> %s%s" % [name, ProjectSettings.globalize_path(path), "" if err == OK else " FAILED"])


func _check_login(host: Control) -> void:
	var s := KUiScheme.new()
	check(s.build(host, "login"), "the login window builds")
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
	var remember := s.widget("remember") as TextureButton
	check(remember != null and remember.toggle_mode, "remember-my-account is a two-state button")
	check(s.missing.is_empty(), "every picture of the login window loaded: %s" % str(s.missing))
	# 800x600 into 1280x720 is 1.2, centred: the old window, not stretched
	check(is_equal_approx(s.root.scale.x, 1.2), "the window is scaled to the screen: %f" % s.root.scale.x)
	check(s.root.position == Vector2(160, 0), "the window is centred: %s" % s.root.position)


func _check_select(host: Control) -> void:
	var s := KUiScheme.new()
	check(s.build(host, "select_role"), "the character list window builds")
	check(s.portrait(0, 0, 1) != null, "the metal male figure is there")
	check(s.portrait(4, 1, 0) != null, "the earth female portrait is there")
	check(s.widget("player") != null, "the list window has a place for the figures")
	for b in ["ok", "cancel", "new", "del"]:
		check(s.widget(b) is TextureButton, "%s is a button" % b)
	# the list screen has no picture of its own: it is drawn on the login backdrop (LoginBg=Login2)
	check(s.root.get_node_or_null("Backdrop") != null, "the list window sits on the login backdrop")


func _check_create(host: Control) -> void:
	var s := KUiScheme.new()
	check(s.build(host, "new_role"), "the create window builds")
	var cname := s.widget("name") as LineEdit
	check(cname != null and cname.max_length == 16, "the name box keeps MaxLen=16")
	for element in ["gold", "wood", "water", "fire", "earth"]:
		var b := s.widget(element) as TextureButton
		check(b != null and b.toggle_mode, "%s is a two-state button" % element)
	check(s.widget("male") != null and s.widget("female") != null, "both figures have a place")
	check(s.portrait(2, 1, 1) != null, "the water female figure is there")
