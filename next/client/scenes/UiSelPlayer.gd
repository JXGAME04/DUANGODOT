# Character lobby: the old client's window, laid out from assets/ui/select_role.json.
#
# \Ui\Ui3\选游戏存档人物.ini gives the buttons and the small info panel; the figures themselves are
# built by name (PlayerImgPrefix, KUiSelPlayer::GetRoleImageName), and the ini says where each one
# stands: Player2Pos_0/1 for two characters, Player3Pos_0/1/2 for three.  Clicking a figure picks
# that character, exactly as before.
extends Control

const KLogin := preload("res://net/KLogin.gd")
const KUiScheme := preload("res://scenes/KUiScheme.gd")
const SERIES_NAMES := ["Kim", "Mộc", "Thủy", "Hỏa", "Thổ"]
const SEX_NAMES := ["Nam", "Nữ"]

var _scheme: KUiScheme = null
var _figures: Array[TextureButton] = []
var _list: ItemList              # only in the fallback screen
var _name: Label
var _level: Label
var _enter: BaseButton
var _new: BaseButton
var _del: BaseButton
var _status: Label
var _chars: Array = []
var _picked := 0


func _ready() -> void:
	if not _build_scheme():
		_build_plain_ui()
	Game.char_list.connect(_on_char_list)
	Game.char_created.connect(_on_char_created)
	Game.entered_world.connect(_on_entered_world)
	Game.enter_failed.connect(_on_enter_failed)
	Game.connection_lost.connect(_on_connection_lost)
	Game.kicked.connect(_on_kicked)
	Log.info("ui", "char select screen")
	Game.request_char_list()


# ------------------------------------------------------------ the old window

func _build_scheme() -> bool:
	var s := KUiScheme.new()
	if not s.build(self, "select_role"):
		return false
	_scheme = s
	_enter = s.widget("ok") as BaseButton
	_new = s.widget("new") as BaseButton
	_del = s.widget("del") as BaseButton
	var cancel := s.widget("cancel") as BaseButton
	if _enter == null or _new == null:
		Log.warn("ui", "select layout has no ok/new button")
		s.root.queue_free()
		return false
	_enter.pressed.connect(_on_enter_pressed)
	_new.pressed.connect(func(): get_tree().change_scene_to_file("res://scenes/UiNewPlayer.tscn"))
	if _del != null:
		_del.pressed.connect(_on_delete_pressed)
	if cancel != null:
		cancel.pressed.connect(func():
			Game.logout("back to login")
			get_tree().change_scene_to_file("res://scenes/UiLogin.tscn"))
	# character transfer is a service of the old game we do not run
	var transfer := s.widget("transfer")
	if transfer != null:
		transfer.visible = false
	_name = _text(s, "name")
	_level = _text(s, "level")
	_status = _text(s, "refuselogin")
	if _status == null:
		_status = _text(s, "lifetime")
	return true


# A section the old client filled with text (Name, Level, RefuseLogin...) is a label here; the
# builder only makes one when the section gave a font, so fall back to an added label.
func _text(s: KUiScheme, section: String) -> Label:
	var l := s.widget(section) as Label
	if l != null:
		return l
	var r := s.rect(section)
	if r.size == Vector2.ZERO:
		return null
	l = Label.new()
	l.position = r.position
	l.size = r.size
	l.custom_minimum_size = r.size
	l.clip_text = true
	l.add_theme_font_size_override("font_size", 13)
	l.add_theme_color_override("font_outline_color", Color8(20, 12, 6))
	l.add_theme_constant_override("outline_size", 3)
	l.mouse_filter = Control.MOUSE_FILTER_IGNORE
	s.root.add_child(l)
	return l


# One clickable figure per character, standing where the ini says they stand.
func _build_figures() -> void:
	for f in _figures:
		f.queue_free()
	_figures.clear()
	if _scheme == null:
		return
	var place: Control = _scheme.widget("player")
	if place == null:
		return
	var offsets := _figure_offsets(_chars.size())
	for i in _chars.size():
		var c = _chars[i]
		var tex := _scheme.portrait(int(c.series), int(c.sex), 1)
		if tex == null:
			continue
		var b := TextureButton.new()
		b.texture_normal = tex
		b.name = "Figure%d" % i
		# the ini gives the sideways offset of each character; the figure picture is a full screen
		b.position = place.position + Vector2(offsets[i], 0)
		b.size = tex.get_size()
		b.pressed.connect(_pick.bind(i))
		_scheme.root.add_child(b)
		_figures.append(b)
	_show_picked()


# Player2Pos_0/1 and Player3Pos_0/1/2 in the .ini: where each figure stands for that many.
func _figure_offsets(count: int) -> Array:
	if _scheme == null or count <= 1:
		return [0.0]
	var out := []
	for i in count:
		var key := "player%dpos_%d" % [count, i]
		out.append(float(_scheme.extra("selrole", key, "0")))
	return out


func _pick(i: int) -> void:
	_picked = i
	_show_picked()


func _show_picked() -> void:
	if _picked >= _chars.size():
		_picked = 0
	for i in _figures.size():
		# the picked character stands in front
		_figures[i].modulate = Color(1, 1, 1, 1) if i == _picked else Color(1, 1, 1, 0.55)
	if _list != null and _picked < _list.item_count:
		_list.select(_picked)
	if _chars.is_empty():
		if _name != null:
			_name.text = ""
		if _level != null:
			_level.text = ""
		return
	var c = _chars[_picked]
	if _name != null:
		_name.text = str(c.name)
	if _level != null:
		_level.text = "Lv %d  %s  %s" % [int(c.level), SERIES_NAMES[clampi(int(c.series), 0, 4)], SEX_NAMES[clampi(int(c.sex), 0, 1)]]


# ------------------------------------------------------------ fallback

func _build_plain_ui() -> void:
	var center := CenterContainer.new()
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(center)
	var box := VBoxContainer.new()
	box.custom_minimum_size = Vector2(480, 0)
	box.add_theme_constant_override("separation", 8)
	center.add_child(box)

	var title := Label.new()
	title.text = "Chọn nhân vật"
	title.add_theme_font_size_override("font_size", 28)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	box.add_child(title)

	_list = ItemList.new()
	_list.custom_minimum_size = Vector2(0, 160)
	_list.item_selected.connect(_pick)
	_list.item_activated.connect(func(_i): _on_enter_pressed())
	box.add_child(_list)

	var b := Button.new()
	b.text = "Vào game"
	b.pressed.connect(_on_enter_pressed)
	box.add_child(b)
	_enter = b

	var nb := Button.new()
	nb.text = "Tạo nhân vật"
	nb.pressed.connect(func(): get_tree().change_scene_to_file("res://scenes/UiNewPlayer.tscn"))
	box.add_child(nb)
	_new = nb

	_status = Label.new()
	_status.autowrap_mode = TextServer.AUTOWRAP_WORD
	box.add_child(_status)

	var back := Button.new()
	back.text = "Thoát"
	back.pressed.connect(func():
		Game.logout("back to login")
		get_tree().change_scene_to_file("res://scenes/UiLogin.tscn"))
	box.add_child(back)


# ------------------------------------------------------------ behaviour

func _on_char_list(chars: Array) -> void:
	_chars = chars
	if _list != null:
		_list.clear()
		for c in chars:
			_list.add_item("%s  (Lv %d, %s, %s)" % [c.name, c.level, SERIES_NAMES[clampi(c.series, 0, 4)], SEX_NAMES[clampi(c.sex, 0, 1)]])
	_picked = 0
	_build_figures()
	_show_picked()
	if _status != null:
		_status.text = "%d nhân vật." % chars.size() if chars.size() > 0 else "Chưa có nhân vật - hãy tạo một nhân vật."
	if _del != null:
		_del.disabled = chars.is_empty()
	if _enter != null:
		_enter.disabled = chars.is_empty()
	KUiScheme.shot_if_asked(self, "select_screen")
	if "--auto" in OS.get_cmdline_user_args():
		if chars.size() > 0:
			if DisplayServer.get_name() != "headless":
				await RenderingServer.frame_post_draw
				await RenderingServer.frame_post_draw
				get_viewport().get_texture().get_image().save_png("user://logs/auto_charselect.png")
			Log.info("auto", "auto enter", {"pid": chars[0].pid, "name": chars[0].name})
			_on_enter_pressed()
		else:
			Log.info("auto", "auto create")
			get_tree().change_scene_to_file("res://scenes/UiNewPlayer.tscn")


func _on_char_created(ok: bool, _result: int, summary: Dictionary) -> void:
	if ok and _status != null:
		_status.text = "Đã tạo %s." % summary.name
	Game.request_char_list()


func _on_delete_pressed() -> void:
	# deleting a character is not in the protocol yet; say so instead of doing nothing
	if _status != null:
		_status.text = "Chưa xoá được nhân vật (máy chủ chưa có lệnh này)."


func _on_enter_pressed() -> void:
	if _chars.is_empty() or _picked >= _chars.size():
		if _status != null:
			_status.text = "Chọn một nhân vật."
		return
	_enter.disabled = true
	if _status != null:
		_status.text = "Đang vào game..."
	Game.enter_world(int(_chars[_picked].pid))


func _on_entered_world(_info: Dictionary) -> void:
	get_tree().change_scene_to_file("res://scenes/UiGame.tscn")


func _on_enter_failed(result: int) -> void:
	_enter.disabled = false
	if _status != null:
		_status.text = "Không vào được game (%d)%s" % [result, " - zone chưa sẵn sàng" if result == 9 else ""]


func _on_kicked(reason: int, text: String) -> void:
	_enter.disabled = false
	if _status != null:
		_status.text = "Bị ngắt: " + KLogin.result_text(reason, text)
	if KLogin.session_ends(reason):
		Game.logout("kicked")
		get_tree().change_scene_to_file("res://scenes/UiLogin.tscn")


func _on_connection_lost(reason: String) -> void:
	Log.warn("ui", "connection lost in lobby", {"reason": reason})
	get_tree().change_scene_to_file("res://scenes/UiLogin.tscn")
