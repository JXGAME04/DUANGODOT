# KUiSelNativePlace - "Chọn tân thủ thôn": where the new character is born
# (Ui\UiCase\UiSelNativePlace.cpp; layout <theme>\UiNewLogin\选新手村.ini; the villages come from
# \Settings\NativePlaceList.ini: [List] Count, then one section per village with Id, Name, Img, Desc).
#
# The list on the right names the villages, the picture and the text on the left belong to the one
# that is picked.  [RecommendImg] is a fixed mark beside the first row - the village the game
# recommends.  Up / Down move the pick, Enter or a double click goes on to the character window with
# the village's Id, Escape goes back to the characters.  A village without an Id, a name or a
# picture is left out, like in LoadList of the old window.
extends "res://ui/elem/KWndShowAnimate.gd"

const KUiImage := preload("res://ui/KUiImage.gd")
const KWndButton := preload("res://ui/elem/KWndButton.gd")
const KWndText := preload("res://ui/elem/KWndText.gd")
const KWndList := preload("res://ui/elem/KWndList.gd")
const KWndImageBase := preload("res://ui/elem/KWndImage.gd")

const SCHEME := "chon-tan-thu-thon"

signal place_chosen(place_id: int)
signal cancelled

var login_bg := "Login2"
var places: Array = []                 # [{id, name, desc, image}]

var _list := KWndList.new()
var _place_img := KWndImageBase.new()
var _recommend := KWndImageBase.new()
var _ok := KWndButton.new()
var _cancel := KWndButton.new()
var _desc := KWndText.new()
var _last_sel := 0


func load_scheme(screen: Vector2i) -> bool:
	var ini: KUiScheme = KUiScheme.open(SCHEME)
	if ini == null or not init_window(ini, "Main", screen):
		return false
	name = "UiSelNativePlace"
	login_bg = ini.get_string("Main", "LoginBg", "Login2")
	for pair in [[_place_img, "PlaceImg"], [_list, "List"], [_recommend, "RecommendImg"], [_ok, "OK"], [_cancel, "Cancel"], [_desc, "PlaceDescText"]]:
		add_child(pair[0])
		pair[0].init_from(ini, pair[1])
	_load_list()
	_list.item_selected.connect(_on_pick)
	_list.item_activated.connect(func(_i): _on_ok())
	_ok.clicked.connect(_on_ok)
	_cancel.clicked.connect(_on_cancel)
	focus_mode = Control.FOCUS_ALL
	return true


# KUiSelNativePlace::LoadList
func _load_list() -> void:
	places = []
	var data = Assets.ui_data("tan-thu-thon")
	if data == null:
		return
	var sections := {}
	for sec in data.get("sections", []):
		sections[str(sec.get("name", "")).to_lower()] = sec
	var count := KUiScheme.leading_int(str(sections.get("list", {}).get("values", {}).get("count", "0")))
	for i in count:
		var sec: Dictionary = sections.get(str(i), {})
		var values: Dictionary = sec.get("values", {})
		if not values.has("id") or str(values.get("name", "")) == "" or str(values.get("img", "")) == "":
			continue
		places.append({"id": KUiScheme.leading_int(str(values["id"])), "name": str(values["name"]),
			"desc": str(values.get("desc", "")), "image": KUiImage.from_entry(sec.get("images", {}).get("img", null))})
	var titles: Array = []
	for p in places:
		titles.append(p["name"])
	_list.set_items(titles)


# KUiSelNativePlace::OpenWindow(nPlaceId)
func open(place_id: int = -1) -> void:
	_last_sel = 0
	for i in places.size():
		if int(places[i]["id"]) == place_id:
			_last_sel = i
	_list.set_cur_sel(_last_sel if not places.is_empty() else -1)
	update_data()
	show_window()
	grab_focus()


func update_data() -> void:
	if places.is_empty():
		return
	var place: Dictionary = places[_last_sel]
	_place_img.set_image(place["image"])
	_desc.text = str(place["desc"])


func _on_pick(index: int) -> void:
	if index >= 0 and index != _last_sel:
		_last_sel = index
		update_data()


func _on_ok() -> void:
	if _list.selected < 0 or _list.selected >= places.size():
		return
	hide_window()
	place_chosen.emit(int(places[_list.selected]["id"]))


func _on_cancel() -> void:
	hide_window()
	cancelled.emit()


func _gui_input(event: InputEvent) -> void:
	if not (event is InputEventKey) or not event.pressed:
		return
	match event.keycode:
		KEY_ENTER, KEY_KP_ENTER:
			_on_ok()
		KEY_ESCAPE:
			_on_cancel()
		KEY_UP:
			if _list.selected > 0:
				_list.set_cur_sel(_list.selected - 1)
				_on_pick(_list.selected)
		KEY_DOWN:
			if _list.selected < _list.count() - 1:
				_list.set_cur_sel(_list.selected + 1)
				_on_pick(_list.selected)
		_:
			return
	accept_event()
