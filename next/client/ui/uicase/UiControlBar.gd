# UiControlBar - the control bars of the VLTK 2.0 client (KUiControlBar, gamecl.exe 0x00469840, docs/CLIENT-2.0.md
# §7): the top bar 顶部控制条.ini (KUiHeaderControlBar: life / mana / stamina / exp bars, the level and the rank)
# and the tool bar 工具控制条.ini (KUiToolsControlBar: the buttons that open the windows), read line by line:
#   - LoadScheme 0x00469840: Init(ini, "Main"), AddChild(Txt_Level), AddChild(Txt_WorldSort); then [Main] Button%d for
#     i = 0.. while present: AddElement 0x004695B0 - the section's ClassType names the class (registry 0x00449C40:
#     Player_Life Player_Mana Player_Stamina Player_Exp Player_Level Player_WorldSort Player_Status Player_Items
#     Player_ItemEx Player_Skills Player_Team Player_Faction Player_ChatRoom Player_Task Player_Friend Player_Sit
#     Player_Run Player_Horse Player_Exchange Player_PK Player_Recorder Esc_Options), the element's Init(ini,
#     section) and AddChild.
#   - a bar element (LoadScheme 0x00451C40): the section's rect + Tip; [Section] Part (default 1) picks a
#     KWndPartImage (+0xcdc, cut by PartType 0..3 - KUiPartMath) or a plain KWndImage (+0x770) for <name>_Image, and
#     a KWndText for <name>_Text; its update (Player_Life 0x0044AAA0: GDI 0x3ea {life, max, max2}, SetPart(cur,
#     max(max, max2)), the text "cur/max" while the showplayernumber switch (0x80ED44, on by default) is set; a
#     click runs Switch([[showplayernumber]]) 0x0044AB60; Player_Mana 0x0044AB80 / Player_Stamina 0x0044AC60 the
#     same; Player_Exp 0x0044AD20: SetPart(percent into the level, 100); Player_Level 0x0044AFE0: the level of GDI
#     0x3eb; Player_WorldSort 0x0044B050: the rank of GDI 0x3e9 (+0x64) or "-").
#   - a button element (Init 0x00451580 = a KWndButton from the section): a click runs the Lua of its class
#     (0x0044B250 Open([[status]]), 0x0044B290 Open([[items]]), 0x0044B310 Open([[skills]]), 0x0044BAF0
#     Open([[system]]), 0x0044B470 Switch([[sit]]), ...) - the `command` signal carries that name.
extends "res://ui/elem/KWndWindow.gd"

const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndImage := preload("res://ui/elem/KWndImage.gd")
const KWndPartImage := preload("res://ui/elem/KWndPartImage.gd")
const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KUiPartMath := preload("res://ui/KUiPartMath.gd")

# the classes that draw a value (0x00451C40 elements) and what their click / tip does
const BAR_CLASSES := ["Player_Life", "Player_Mana", "Player_Stamina", "Player_Exp", "Player_Level", "Player_WorldSort"]
# the button classes and the Lua their click runs (Open([[x]]) / Switch([[x]]))
const BUTTON_COMMANDS := {
	"Player_Status": "status", "Player_Items": "items", "Player_ItemEx": "ItemEx", "Player_Skills": "skills",
	"Player_Team": "team", "Player_Faction": "faction", "Player_ChatRoom": "chatroom", "Player_Task": "task",
	"Player_Friend": "friend", "Esc_Options": "system", "Player_Sit": "sit", "Player_Run": "run",
	"Player_Horse": "horse", "Player_Exchange": "exchange", "Player_PK": "pk", "Player_Recorder": "recorder",
}

signal command(name: String)

var show_numbers := true   # the showplayernumber switch: 0x80ED44 starts at 1
var _elements: Array = []  # {"class", "root", "image", "text"} for the bars; {"class", "button"} for the buttons
var _txt_level: KWndText = null
var _txt_worldsort: KWndText = null


# 0x00469840 with the [Main] section of `screen_name` (thanh-dieu-khien-tren / thanh-cong-cu).  The 2.0 layouts are
# drawn for the theme's 1024 x 768 screen; on another size the bar keeps the place it has in that frame - the frame
# is centred (like the bottom bar's PositionType 2) and, for a bar of the bottom row (anchor_bottom), pushed to the
# bottom edge
func load_scheme(screen_name: String, screen: Vector2i, anchor_bottom: bool = false) -> bool:
	var ini: KUiScheme = KUiScheme.open(screen_name)
	if ini == null:
		return false
	var frame: Vector2i = ini.screen_size()
	var shift := Vector2((screen.x - frame.x) / 2.0, (screen.y - frame.y) if anchor_bottom else 0.0)
	if ini.has_section("Main"):
		init_from(ini, "Main")
	position += shift
	name = screen_name
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	var back := KWndImage.new()
	add_child(back)
	if back.init_from(ini, "Main"):
		back.position = Vector2.ZERO
	else:
		back.queue_free()
	_txt_level = _text(ini, "Txt_Level")
	_txt_worldsort = _text(ini, "Txt_WorldSort")
	var i := 0
	while true:
		var section := ini.get_string("Main", "Button%d" % i, "")
		if section == "":
			break
		_add_element(ini, section)
		i += 1
	Log.info("ui", "control bar", {"screen": screen_name, "elements": _elements.size()})
	return true


func _text(ini: KUiScheme, section: String) -> KWndText:
	if not ini.has_section(section):
		return null
	var t := KWndText.new()
	add_child(t)
	t.init_from(ini, section)
	return t


# AddElement 0x004695B0: the ClassType of the section picks the class
func _add_element(ini: KUiScheme, section: String) -> void:
	var cls := ini.get_string(section, "ClassType", "")
	if cls in BAR_CLASSES:
		var root := Control.new()
		root.name = section
		root.position = Vector2(ini.get_integer(section, "Left", 0), ini.get_integer(section, "Top", 0))
		root.size = Vector2(ini.get_integer(section, "Width", 0), ini.get_integer(section, "Height", 0))
		root.tooltip_text = ini.get_string(section, "Tip", "")
		root.mouse_filter = Control.MOUSE_FILTER_STOP
		add_child(root)
		var image = null
		if ini.get_integer(section, "Part", 1) != 0:
			image = KWndPartImage.new()
		else:
			image = KWndImage.new()
		root.add_child(image)
		if not image.init_from(ini, section + "_Image"):
			image.queue_free()
			image = null
		var text: KWndText = null
		if ini.has_section(section + "_Text"):
			text = KWndText.new()
			root.add_child(text)
			text.init_from(ini, section + "_Text")
		root.gui_input.connect(_on_bar_input.bind(cls))
		_elements.append({"class": cls, "root": root, "image": image, "text": text})
		return
	if BUTTON_COMMANDS.has(cls) or ini.has_section(section):
		var b := KWndButton.new()
		add_child(b)
		if not b.init_from(ini, section):
			b.queue_free()
			return
		b.clicked.connect(_on_button.bind(cls))
		_elements.append({"class": cls, "button": b})


# 0x0044AB60: a click on a bar toggles the numbers
func _on_bar_input(event: InputEvent, cls: String) -> void:
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		if cls in ["Player_Life", "Player_Mana", "Player_Stamina"]:
			show_numbers = not show_numbers
			refresh()


func _on_button(cls: String) -> void:
	var cmd: String = BUTTON_COMMANDS.get(cls, cls)
	Log.info("ui", "control bar button", {"class": cls, "command": cmd})
	command.emit(cmd)


# the updates of the six bar classes from the character's own numbers (G2C_PLAYER_ATTRIB)
func refresh() -> void:
	var a: Dictionary = Game.player_attrib
	for e in _elements:
		if not e.has("root"):
			continue
		var cur := 0
		var max_value := 0
		var label := ""
		match e["class"]:
			"Player_Life":
				cur = int(a.get("life", 0))
				max_value = int(a.get("life_max", 0))
				label = "%d/%d" % [cur, max_value] if show_numbers else ""
			"Player_Mana":
				cur = int(a.get("mana", 0))
				max_value = int(a.get("mana_max", 0))
				label = "%d/%d" % [cur, max_value] if show_numbers else ""
			"Player_Stamina":
				cur = int(a.get("stamina", 0))
				max_value = int(a.get("stamina_max", 0))
				label = "%d/%d" % [cur, max_value] if show_numbers else ""
			"Player_Exp":
				cur = KUiPartMath.exp_percent(int(a.get("exp", 0)), int(a.get("level_exp", 0)), int(a.get("next_level_exp", 0)))
				max_value = 100
				label = "%d%%" % cur
			"Player_Level":
				label = str(int(a.get("level", 0))) if a.has("level") else ""
			"Player_WorldSort":
				label = "-"
		if e["image"] != null and e["image"] is KWndPartImage:
			e["image"].set_part(cur, max_value)
		if e["text"] != null:
			e["text"].text = label
