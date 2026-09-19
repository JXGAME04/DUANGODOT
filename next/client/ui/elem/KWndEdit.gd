# KWndEdit - a box the player types in (Ui\Elem\WndEdit.cpp).
#
#   Font=        size (default 12; under 8 becomes 12)     Color= / BorderColor=
#   Password=1   every character shows as '*'
#   Type=        1 plain ASCII only (account, password), 2 the input method may be used (names)
#   MaxLen=      the longest text, in characters
#   HAlign=1     the text is centred in the box (a key of the 2.0 layouts)
#   FocusBKColor= / FocusBKColorAlpha=   a tint behind the text while the box has the keyboard
#
# The old box drew the text at its corner with the game font, then a '|' GLYPH as the caret at
#     x = (caret - first shown) * Font / 2 + 2 - Font / 2
# shown 10 paints out of 17.  This class draws the same way.  Typing itself - keyboard, input
# method, clipboard, the on-screen keyboard of a phone - is left to an invisible LineEdit inside,
# which is what Godot does well and the old code did by hand.
extends "res://ui/elem/KWndWindow.gd"

const KFont := preload("res://ui/KFont.gd")
const CARET_ON_MS := 556        # 10 of 17 paints at 18 frames a second
const CARET_PERIOD_MS := 944

signal text_changed(text: String)
signal submitted(text: String)             # Enter
signal tab_pressed(backwards: bool)        # the window decides who is next, like WND_N_EDIT_SPECIAL_KEY_DOWN
signal escape_pressed
signal focus_gained

var font_size := 12
var text_color := Color.BLACK
var border_color := Color.BLACK
var password := false
var ascii_only := false
var max_len := 0
var digits_only := false                  # the numeric box of KUiGetString: '0'..'9' and nothing else
var halign := 0
var focus_bg = null                       # Color or null
var input: LineEdit = null

var _skip_ahead := 0                      # m_nSkipAhead: characters scrolled out on the left
var _caret_epoch := 0


func init_from(ini: KUiScheme, section: String) -> bool:
	if not super.init_from(ini, section):
		return false
	password = ini.get_bool(section, "Password", false)
	ascii_only = ini.get_integer(section, "Type", 0) == 1
	max_len = maxi(ini.get_integer(section, "MaxLen", 0), 0)
	text_color = ini.get_color(section, "Color", Color.BLACK)
	border_color = ini.get_color(section, "BorderColor", Color.BLACK)
	font_size = ini.get_integer(section, "Font", 12)
	if font_size < 8:
		font_size = 12
	halign = ini.get_integer(section, "HAlign", 0)
	if size.x < font_size:
		size.x = font_size
	if size.y < font_size + 1:
		size.y = font_size + 1
	if ini.get_string(section, "FocusBKColor", "") != "":
		var c: Color = ini.get_color(section, "FocusBKColor", Color.BLACK)
		var alpha := ini.get_integer(section, "FocusBKColorAlpha", 0)
		c.a = 0.35 if alpha < 0 or alpha > 255 else float(255 - alpha) / 255.0
		focus_bg = c
	_make_input()
	mouse_filter = Control.MOUSE_FILTER_STOP
	mouse_default_cursor_shape = Control.CURSOR_IBEAM
	return true


func _make_input() -> void:
	input = LineEdit.new()
	input.name = "Input"
	input.position = Vector2.ZERO
	input.size = size
	input.flat = true
	input.secret = password
	input.max_length = max_len
	input.context_menu_enabled = false
	input.caret_blink = false
	input.mouse_filter = Control.MOUSE_FILTER_IGNORE    # clicks are ours: the caret follows OUR pitch
	input.focus_mode = Control.FOCUS_ALL
	var nothing := StyleBoxEmpty.new()
	for style in ["normal", "focus", "read_only"]:
		input.add_theme_stylebox_override(style, nothing)
	var clear := Color(0, 0, 0, 0)
	for c in ["font_color", "font_selected_color", "font_uneditable_color", "font_placeholder_color", "caret_color", "selection_color", "font_outline_color"]:
		input.add_theme_color_override(c, clear)
	input.text_changed.connect(_on_input_changed)
	input.text_submitted.connect(func(t): submitted.emit(t))
	input.gui_input.connect(_on_input_key)
	input.focus_entered.connect(_on_focus.bind(true))
	input.focus_exited.connect(_on_focus.bind(false))
	add_child(input)


func get_text() -> String:
	return input.text if input != null else ""


func set_text(value: String) -> void:
	if input == null:
		return
	input.text = value
	input.caret_column = value.length()
	_skip_ahead = 0
	queue_redraw()


func clear_text() -> void:
	set_text("")


# the longest text (0 = no limit) and whether only digits may be typed - what the 2.0 window sets on the edit after the
# layout (+0x4a4 of the edit = MaxLen; KUiGetString 0x0051CA8D / 0x0051CE13)
func set_limits(max_length: int, digits: bool) -> void:
	max_len = maxi(max_length, 0)
	digits_only = digits
	if input != null:
		input.max_length = max_len


func take_focus() -> void:
	if input != null and is_visible_in_tree():
		input.grab_focus()


func has_keyboard() -> bool:
	return input != null and input.has_focus()


func enable(on: bool) -> void:
	super.enable(on)
	if input != null:
		input.editable = on


func _on_input_changed(new_text: String) -> void:
	if ascii_only or digits_only:
		var kept := ""
		for ch in new_text:
			var code := ch.unicode_at(0)
			if digits_only:
				if code >= 0x30 and code <= 0x39:
					kept += ch
			elif code >= 0x20 and code < 0x7f:
				kept += ch
		if kept != new_text:
			var caret := input.caret_column - (new_text.length() - kept.length())
			input.text = kept
			input.caret_column = clampi(caret, 0, kept.length())
			new_text = kept
	_caret_epoch = Time.get_ticks_msec()
	queue_redraw()
	text_changed.emit(new_text)


func _on_input_key(event: InputEvent) -> void:
	if not (event is InputEventKey) or not event.pressed:
		return
	if event.keycode == KEY_TAB:
		input.accept_event()
		tab_pressed.emit(event.shift_pressed)
	elif event.keycode == KEY_ESCAPE:
		input.accept_event()
		escape_pressed.emit()
	_caret_epoch = Time.get_ticks_msec()
	queue_redraw()


func _on_focus(gained: bool) -> void:
	_caret_epoch = Time.get_ticks_msec()
	queue_redraw()
	if gained:
		focus_gained.emit()


func _gui_input(event: InputEvent) -> void:
	if disabled or input == null:
		return
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
		input.grab_focus()
		var pitch := font_size / 2
		var column := _skip_ahead + int(round((event.position.x - _text_x()) / float(pitch)))
		input.caret_column = clampi(column, 0, input.text.length())
		input.deselect()
		_caret_epoch = Time.get_ticks_msec()
		queue_redraw()
		accept_event()


func _process(_delta: float) -> void:
	if has_keyboard():
		queue_redraw()     # the caret blinks


# How many characters fit the box.
func _room() -> int:
	return maxi(int(size.x) * 2 / font_size, 1)


func _shown_text() -> String:
	var t := input.text
	if password:
		t = "*".repeat(t.length())
	return t


func _text_x() -> int:
	if halign != 1:
		return 0
	var chars := mini(input.text.length() - _skip_ahead, _room())
	return maxi((int(size.x) - chars * font_size / 2) / 2, 0)


func _draw() -> void:
	if input == null:
		return
	var focused := has_keyboard()
	if focused and focus_bg != null:
		draw_rect(Rect2(Vector2.ZERO, size), focus_bg)
	# keep the caret inside the box (m_nSkipAhead of the old code)
	var caret := input.caret_column
	var room := _room()
	if caret < _skip_ahead:
		_skip_ahead = caret
	elif caret > _skip_ahead + room:
		_skip_ahead = caret - room
	_skip_ahead = clampi(_skip_ahead, 0, maxi(input.text.length() - 1, 0))
	var shown := _shown_text().substr(_skip_ahead, room)
	var x0 := _text_x()
	var pitch := font_size / 2
	if input.has_selection():
		var from := clampi(input.get_selection_from_column() - _skip_ahead, 0, shown.length())
		var to := clampi(input.get_selection_to_column() - _skip_ahead, 0, shown.length())
		if to > from:
			draw_rect(Rect2(x0 + from * pitch, 0, (to - from) * pitch, font_size + 1), Color(0.35, 0.45, 0.8, 0.55))
	var font = KFont.of(font_size)
	if font != null:
		font.draw(self, Vector2(x0, 0), shown, text_color, border_color)
	else:
		draw_string(get_theme_default_font(), Vector2(x0, font_size), shown, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, text_color)
	if focused and (Time.get_ticks_msec() - _caret_epoch) % CARET_PERIOD_MS < CARET_ON_MS:
		var caret_x := x0 + (caret - _skip_ahead) * pitch + 2 - pitch
		if font != null:
			font.draw(self, Vector2(caret_x, 0), "|", text_color, border_color)
		else:
			draw_rect(Rect2(caret_x + pitch, 1, 1, font_size), text_color)
