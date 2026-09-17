# KUiLoginBackGround - the picture behind every window of the login flow
# (Ui\UiCase\UiLoginBg.cpp; layout <theme>\UiNewLogin\登陆过程背景窗口.ini).
#
# A window of the flow names its backdrop with LoginBg= in its own layout (Login for the server and
# login windows, Login2 for the character windows), and the backdrop window loads the section of
# that name plus <name>_Butterfly_0..2, its ornaments:
#     Interval=0,0          the ornament plays on and on (the falling leaves of "Login")
#     Interval=1000,3000    it plays once, rests between 1 and 3 seconds, plays again
# The 2.0 layout adds three parts that do not change with the backdrop: VersionText, HealthGame
# and Limit16YearsOld (the "18+" plate in the corner).
extends "res://ui/elem/KWndImage.gd"

const KWndImage := preload("res://ui/elem/KWndImage.gd")
const KWndText := preload("res://ui/elem/KWndText.gd")

const SCHEME := "nen-dang-nhap"
const MAX_NUM_BUTTERFLY := 3

var config := ""                     # m_szConfig: the section in use
var _ini: KUiScheme = null
var _butterflies: Array = []         # of KWndImage
var _interval_min: Array = [0, 0, 0]
var _interval_max: Array = [0, 0, 0]
var _sleep_ms: Array = [0, 0, 0]     # m_uInterval: how long the ornament rests now, 0 = it is playing
var _slept_at: Array = [0, 0, 0]
var _version: KWndText = null
var _health: KWndText = null
var _plate: KWndImage = null


func _init() -> void:
	name = "UiLoginBg"
	_ini = KUiScheme.open(SCHEME)
	for i in MAX_NUM_BUTTERFLY:
		var b := KWndImage.new()
		add_child(b)
		_butterflies.append(b)
	_version = KWndText.new()
	add_child(_version)
	_health = KWndText.new()
	add_child(_health)
	_plate = KWndImage.new()
	add_child(_plate)
	if _ini != null:
		_version.init_from(_ini, "VersionText")
		_health.init_from(_ini, "HealthGame")
		_plate.init_from(_ini, "Limit16YearsOld")


func is_ready() -> bool:
	return _ini != null


func set_version(text: String) -> void:
	_version.text = text


# KUiLoginBackGround::SetConfig
func set_config(section: String) -> void:
	if _ini == null or section == "" or section == config:
		return
	if not _ini.has_section(section):
		Log.warn("ui", "login backdrop has no such section", {"section": section})
		return
	config = section
	init_from(_ini, section)
	position = Vector2.ZERO      # the backdrop covers the screen whatever its section says
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	for i in MAX_NUM_BUTTERFLY:
		var part := "%s_Butterfly_%d" % [section, i]
		var b: KWndImage = _butterflies[i]
		b.visible = true
		b.image = null
		b.init_from(_ini, part)
		var iv: Vector2i = _ini.get_integer2(part, "Interval", Vector2i.ZERO)
		_interval_min[i] = iv.x
		_interval_max[i] = maxi(iv.y, iv.x)
		_sleep_ms[i] = 0
	Log.debug("ui", "login backdrop", {"section": section})


# KUiLoginBackGround::Breathe
func _process(_delta: float) -> void:
	var now := Time.get_ticks_msec()
	for i in MAX_NUM_BUTTERFLY:
		var b: KWndImage = _butterflies[i]
		if b.image == null:
			continue
		if _interval_min[i] == 0:
			b.next_frame()
		elif _sleep_ms[i] == 0:
			if b.next_frame():
				_slept_at[i] = now
				var spread: int = _interval_max[i] - _interval_min[i]
				_sleep_ms[i] = _interval_min[i] + (randi() % spread if spread > 0 else 0)
		elif now - _slept_at[i] >= _sleep_ms[i]:
			b._flip_ms = now
			_sleep_ms[i] = 0
