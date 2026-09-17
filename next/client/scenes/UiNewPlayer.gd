# Character creation: the old client's window, laid out from assets/ui/new_role.json.
#
# \Ui\Ui3\新建角色.ini gives the name box, the five element buttons and the two places a figure
# stands.  The figures are built by name, the way KUiNewPlayer::SetPlayerImage did it: the sex you
# picked stands in front (picture 1) and the other one behind (picture 2), and both change when
# you pick another element.
extends Control

const KLogin := preload("res://net/KLogin.gd")
const KUiScheme := preload("res://scenes/KUiScheme.gd")
const ELEMENTS := ["gold", "wood", "water", "fire", "earth"]
const ELEMENT_NAMES := ["Kim", "Mộc", "Thủy", "Hỏa", "Thổ"]
const ELEMENT_TEXT := [
	"Kim - cương mãnh, sát thương mạnh.",
	"Mộc - bền bỉ, hồi phục tốt.",
	"Thủy - linh hoạt, nội lực dồi dào.",
	"Hỏa - bùng nổ, đánh nhanh.",
	"Thổ - vững chãi, chịu đòn giỏi.",
]

var _scheme: KUiScheme = null
var _name: LineEdit
var _ok: BaseButton
var _status: Label
var _male: TextureRect
var _female: TextureRect
var _series := 0
var _sex := 0


func _ready() -> void:
	if not _build_scheme():
		_build_plain_ui()
	Game.char_created.connect(_on_char_created)
	Game.connection_lost.connect(_on_connection_lost)
	Game.kicked.connect(_on_kicked)
	Log.info("ui", "new character screen")
	_refresh()
	KUiScheme.shot_if_asked(self, "new_role_screen")
	if "--auto" in OS.get_cmdline_user_args():
		_name.text = "Auto %d" % (randi() % 100000)
		Log.info("auto", "auto create", {"name": _name.text})
		call_deferred("_on_ok_pressed")


# ------------------------------------------------------------ the old window

func _build_scheme() -> bool:
	var s := KUiScheme.new()
	if not s.build(self, "new_role"):
		return false
	_scheme = s
	_name = s.widget("name") as LineEdit
	_ok = s.widget("ok") as BaseButton
	var cancel := s.widget("cancel") as BaseButton
	if _name == null or _ok == null:
		Log.warn("ui", "create layout has no name box or ok button")
		s.root.queue_free()
		return false
	_name.placeholder_text = "Tên nhân vật"
	_name.text_submitted.connect(func(_t): _on_ok_pressed())
	_ok.pressed.connect(_on_ok_pressed)
	if cancel != null:
		cancel.pressed.connect(func(): get_tree().change_scene_to_file("res://scenes/UiSelPlayer.tscn"))
	for i in ELEMENTS.size():
		var b := s.widget(ELEMENTS[i]) as BaseButton
		if b != null:
			b.pressed.connect(_pick_series.bind(i))
	# the two figures: the sections are empty places in the .ini, the pictures go in them
	_male = _figure(s, "male")
	_female = _figure(s, "female")
	_status = s.widget("propertyshow") as Label
	return true


func _figure(s: KUiScheme, section: String) -> TextureRect:
	var place: Control = s.widget(section)
	if place == null:
		return null
	var tr := TextureRect.new()
	tr.name = section.capitalize()
	tr.mouse_filter = Control.MOUSE_FILTER_IGNORE
	place.add_child(tr)
	# clicking a figure picks that sex, the way the old screen did
	var b := Button.new()
	b.flat = true
	b.size = place.size
	b.custom_minimum_size = place.size
	b.pressed.connect(_pick_sex.bind(0 if section == "male" else 1))
	place.mouse_filter = Control.MOUSE_FILTER_PASS
	place.add_child(b)
	return tr


func _pick_series(i: int) -> void:
	_series = i
	_refresh()


func _pick_sex(i: int) -> void:
	_sex = i
	_refresh()


# Redraws what the choice looks like: the picked sex in front, the other behind, the buttons of the
# five elements showing which one is down, and the line of text under the figures.
func _refresh() -> void:
	if _scheme != null:
		if _male != null:
			var tex := _scheme.portrait(_series, 0, 1 if _sex == 0 else 2)
			_male.texture = tex
			if tex != null:
				_male.size = tex.get_size()
			_male.modulate = Color(1, 1, 1, 1) if _sex == 0 else Color(1, 1, 1, 0.6)
		if _female != null:
			var tex := _scheme.portrait(_series, 1, 1 if _sex == 1 else 2)
			_female.texture = tex
			if tex != null:
				_female.size = tex.get_size()
			_female.modulate = Color(1, 1, 1, 1) if _sex == 1 else Color(1, 1, 1, 0.6)
		for i in ELEMENTS.size():
			var b := _scheme.widget(ELEMENTS[i]) as BaseButton
			if b != null and b.toggle_mode:
				b.set_pressed_no_signal(i == _series)
	if _status != null:
		_status.text = ELEMENT_TEXT[_series]


# ------------------------------------------------------------ fallback

func _build_plain_ui() -> void:
	var center := CenterContainer.new()
	center.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(center)
	var box := VBoxContainer.new()
	box.custom_minimum_size = Vector2(420, 0)
	box.add_theme_constant_override("separation", 8)
	center.add_child(box)

	var title := Label.new()
	title.text = "Tạo nhân vật"
	title.add_theme_font_size_override("font_size", 28)
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	box.add_child(title)

	_name = LineEdit.new()
	_name.placeholder_text = "Tên nhân vật (2-16 ký tự)"
	_name.max_length = 16
	_name.text_submitted.connect(func(_t): _on_ok_pressed())
	box.add_child(_name)

	var row := HBoxContainer.new()
	box.add_child(row)
	for i in ELEMENT_NAMES.size():
		var b := Button.new()
		b.text = ELEMENT_NAMES[i]
		b.pressed.connect(_pick_series.bind(i))
		row.add_child(b)
	var sex := HBoxContainer.new()
	box.add_child(sex)
	for i in 2:
		var b := Button.new()
		b.text = "Nam" if i == 0 else "Nữ"
		b.pressed.connect(_pick_sex.bind(i))
		sex.add_child(b)

	var ok := Button.new()
	ok.text = "Tạo"
	ok.pressed.connect(_on_ok_pressed)
	box.add_child(ok)
	_ok = ok

	var back := Button.new()
	back.text = "Quay lại"
	back.pressed.connect(func(): get_tree().change_scene_to_file("res://scenes/UiSelPlayer.tscn"))
	box.add_child(back)

	_status = Label.new()
	_status.autowrap_mode = TextServer.AUTOWRAP_WORD
	box.add_child(_status)


# ------------------------------------------------------------ behaviour

func _on_ok_pressed() -> void:
	var who := _name.text.strip_edges()
	if who.length() < 2:
		_say("Tên quá ngắn.")
		return
	_ok.disabled = true
	Log.info("ui", "create character", {"name": who, "series": _series, "sex": _sex})
	Game.create_char(who, _series, _sex)


func _on_char_created(ok: bool, result: int, summary: Dictionary) -> void:
	_ok.disabled = false
	if ok:
		Log.info("ui", "character created", {"name": summary.get("name", "")})
		get_tree().change_scene_to_file("res://scenes/UiSelPlayer.tscn")
		return
	var reasons := {7: "Tên không hợp lệ.", 5: "Tên đã tồn tại.", 6: "Đã đủ số nhân vật."}
	_say(reasons.get(result, "Tạo nhân vật thất bại (%d)." % result))
	if "--auto" in OS.get_cmdline_user_args():
		print("AUTO_CREATE_FAILED %d" % result)
		get_tree().quit(2)


func _say(text: String) -> void:
	if _status != null:
		_status.text = text


func _on_kicked(reason: int, text: String) -> void:
	_ok.disabled = false
	_say("Bị ngắt: " + KLogin.result_text(reason, text))
	if KLogin.session_ends(reason):
		Game.logout("kicked")
		get_tree().change_scene_to_file("res://scenes/UiLogin.tscn")


func _on_connection_lost(reason: String) -> void:
	Log.warn("ui", "connection lost while creating", {"reason": reason})
	get_tree().change_scene_to_file("res://scenes/UiLogin.tscn")
