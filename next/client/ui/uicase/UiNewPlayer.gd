# KUiNewPlayer - "Tạo nhân vật": name, one of the five elements, man or woman
# (Ui\UiCase\UiNewPlayer.cpp; layout <theme>\UiNewLogin\新建角色.ini; LoadScheme of gamecl.exe 2.0
# at 0x4978E0).
#
# The window covers the screen.  Down the left edge five two-state tabs pick the element
# ([Gold] .. [Earth]); the man stands at [Male] and the woman at [Female], both clickable on their
# pixels (Trans=1).  The picked sex shows figure 1 of KUiSelPlayer::GetRoleImageName once and then
# figure 0, the other sex figure 2.  Rules of the old UpdateProperty, which the 2.0 texts repeat
# ("Kim: Nam ...", "Thủy: Nữ ..."):  Kim is for men only, Thủy for women only.
# [PropertyBg] shows the picture that names the element - built in code as
# <PropertyBgImgPrefix>\<金|木|水|火|土>vn.spr - and [PropertyShow] its description, PropText of
# \Ui\五行.ini (read into 256 bytes and shown in a 270x38 box: two lines of it).
# A name must not contain a blank or a control character (sentence 17) and has to be long enough
# (sentence 18); our gateway takes 2 to 16 characters.  Enter creates, Escape goes back to the
# village window.
extends "res://ui/elem/KWndShowAnimate.gd"

const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KWndEdit := preload("res://ui/elem/KWndEdit.gd")
const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndImageBase := preload("res://ui/elem/KWndImage.gd")

const SCHEME := "tao-nhan-vat"
const SERIES_SECTIONS := ["Gold", "Wood", "Water", "Fire", "Earth"]
const SERIES_METAL := 0
const SERIES_WATER := 2
const NAME_MIN := 2
const NAME_MAX := 16

signal create_requested(role_name: String, series: int, sex: int, place_id: int)
signal name_invalid(sentence: int)     # 17 a bad character, 18 a bad length
signal cancelled(place_id: int)

var login_bg := "Login2"
var series := SERIES_METAL
var sex := 0
var place_id := 0

var _ini: KUiScheme = null
var _name := KWndEdit.new()
var _name_bg := KWndImageBase.new()
var _male := KWndButton.new()
var _female := KWndButton.new()
var _ok := KWndButton.new()
var _cancel := KWndButton.new()
var _property_show := KWndText.new()
var _property_bg := KWndImageBase.new()
var _series_btn: Array = []
var _prop_text: Array = ["", "", "", "", ""]
var _just_clicked := false


func load_scheme(screen: Vector2i) -> bool:
	_ini = KUiScheme.open(SCHEME)
	if _ini == null or not init_window(_ini, "NewPlayer", screen):
		return false
	name = "UiNewPlayer"
	login_bg = _ini.get_string("NewPlayer", "LoginBg", "Login2")
	for pair in [[_property_bg, "PropertyBg"], [_male, "Male"], [_female, "Female"], [_name_bg, "NameBg"], [_name, "Name"],
			[_ok, "OK"], [_cancel, "Cancel"], [_property_show, "PropertyShow"]]:
		add_child(pair[0])
		pair[0].init_from(_ini, pair[1])
	for i in SERIES_SECTIONS.size():
		var b := KWndButton.new()
		add_child(b)
		b.init_from(_ini, SERIES_SECTIONS[i])
		b.toggled.connect(_on_series.bind(i))
		_series_btn.append(b)
	var five = Assets.ui_data("ngu-hanh")
	if five != null:
		for sec in five.get("sections", []):
			var at := SERIES_SECTIONS.find(str(sec.get("name", "")))
			if at >= 0:
				_prop_text[at] = str(sec.get("values", {}).get("proptext", "")).substr(0, 255)
	_male.clicked.connect(_on_sex.bind(0))
	_female.clicked.connect(_on_sex.bind(1))
	_ok.clicked.connect(_on_ok)
	_cancel.clicked.connect(_on_cancel)
	_name.submitted.connect(func(_t): _on_ok())
	_name.escape_pressed.connect(_on_cancel)
	mouse_filter = Control.MOUSE_FILTER_PASS
	focus_mode = Control.FOCUS_ALL
	return true


# KUiNewPlayer::OpenWindow(nNativePlaceId)
func open(native_place: int, keep_name: bool = false) -> void:
	place_id = native_place
	if not keep_name:
		_name.clear_text()
	update_property()
	show_window()
	_name.take_focus()


func set_role_name(text: String) -> void:
	_name.set_text(text)


# KUiNewPlayer::UpdateProperty
func update_property() -> void:
	for i in _series_btn.size():
		_series_btn[i].check(i == series)
	_male.enable(series != SERIES_WATER)
	_female.enable(series != SERIES_METAL)
	if series == SERIES_METAL:
		sex = 0
	elif series == SERIES_WATER:
		sex = 1
	sel_gender()
	_property_bg.set_image(_ini.image("PropertyBg", SERIES_SECTIONS[series]))
	_property_show.text = _prop_text[series]


# KUiNewPlayer::SelGender
func sel_gender() -> void:
	var male := sex == 0
	_male.check(male)
	_male.set_image(_ini.portrait(series, 0, 1 if male else 2))
	_female.check(not male)
	_female.set_image(_ini.portrait(series, 1, 2 if male else 1))
	_just_clicked = true
	_name.take_focus()


func _on_series(_checked: bool, index: int) -> void:
	series = index
	update_property()


func _on_sex(which: int) -> void:
	sex = which
	sel_gender()


# KUiNewPlayer::Breathe: the picked figure plays its step forward once, then stands
func _process(delta: float) -> void:
	super._process(delta)
	var male_looped: bool = _male.next_frame()
	var female_looped: bool = _female.next_frame()
	if _just_clicked and ((sex == 0 and male_looped) or (sex == 1 and female_looped)):
		(_male if sex == 0 else _female).set_image(_ini.portrait(series, sex, 0))
		_just_clicked = false


# KUiNewPlayer::GetInputInfo
func _on_ok() -> void:
	var role_name := _name.get_text()
	for ch in role_name:
		if ch.unicode_at(0) <= 0x20:
			hide_window()
			name_invalid.emit(17)
			return
	if role_name.length() < NAME_MIN or role_name.length() > NAME_MAX:
		hide_window()
		name_invalid.emit(18)
		return
	hide_window()
	create_requested.emit(role_name, series, sex, place_id)


func _on_cancel() -> void:
	hide_window()
	cancelled.emit(place_id)


func _gui_input(event: InputEvent) -> void:
	if not (event is InputEventKey) or not event.pressed:
		return
	if event.keycode == KEY_ENTER or event.keycode == KEY_KP_ENTER:
		_on_ok()
	elif event.keycode == KEY_ESCAPE:
		_on_cancel()
	else:
		return
	accept_event()
