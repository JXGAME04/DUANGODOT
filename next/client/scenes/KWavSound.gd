# KWavSound / KSoundCache of the old engine (Engine/Src/KWavSound.cpp, KSoundCache.cpp; gamecl.exe 2.0 sound manager
# 0x1f9cf28: Play 0x0064E600(name, pan, volume, loop), IsPlaying 0x0064E220, Stop 0x0064E1C0): the fight sounds placed
# in the world.  A sound is a .wav the archives hold (jxassets export-sounds), played through up to BUFFER_COUNT
# AudioStreamPlayer2D nodes per file (KWavSound::Play takes the first buffer that is not playing, none free = dropped).
#
# The callers compute the DirectSound volume (hundredths of dB, -10000..0) and pan (-10000..10000) the way
# KMissleRes::PlaySound 0x00717ED0 and KSkill::PlayCastSound 0x006F6D90 do, from the sound's map position and the
# scene focus (g_ScenePlace.GetFocusPosition 0x00671B00):
#   volume = (10000 - (|dx| + |dy|)) * option / 100 - 10000        (KMissleRes::GetSndVolume 0x00717CC0, option = 100)
#   pan    = dx * 5
# Godot: volume_db = volume / 100; the pan is the AudioStreamPlayer2D's own (linear in the screen x) - a
# deliberate deviation, Godot has no per-channel gains.  docs/CLIENT-2.0.md §15
extends Node2D

const BUFFER_COUNT := 3           # KWavSound.h
const DS_PAN_FULL := 10000        # DSBPAN_LEFT / DSBPAN_RIGHT
const DS_VOLUME_MIN := -10000     # DSBVOLUME_MIN

var option_volume := 100          # Option.GetSndVolume(): the sound volume setting, percent
var focus_provider: Callable      # returns the scene focus (Vector2, scene units); unset = (0, 0)
var stream_provider: Callable     # game path -> AudioStream (Assets.sound, the KSoundCache); unset = silent
var _players := {}                # file -> [AudioStreamPlayer2D] (the BUFFER_COUNT buffers of a sound)
var played := 0                   # sounds started (the --auto proof)
var dropped := 0                  # sounds refused: no buffer free, or already playing when the caller asked for one
var history: Array = []           # the last files started, newest last (the --auto proof)


static func to_screen(p: Vector2) -> Vector2:
	return Vector2(p.x, p.y * 0.5)


# KMissleRes::GetSndVolume 0x00717CC0 with nVol = -(|dx| + |dy|): hundredths of dB
static func snd_volume(dx: int, dy: int, option: int) -> int:
	@warning_ignore("integer_division")
	var v := (10000 - (absi(dx) + absi(dy))) * option / 100 - 10000
	return clampi(v, DS_VOLUME_MIN, 0)


# the pan of KMissleRes::PlaySound / PlayCastSound: dx * 5, DirectSound units
static func snd_pan(dx: int) -> int:
	return clampi(dx * 5, -DS_PAN_FULL, DS_PAN_FULL)


func focus() -> Vector2:
	return focus_provider.call() if focus_provider.is_valid() else Vector2.ZERO


# KWavSound::IsPlaying: any buffer of the file playing
func is_playing(file: String) -> bool:
	for p in _players.get(file, []):
		if p.playing:
			return true
	return false


# KWavSound::Stop: the first playing buffer of the file
func stop(file: String) -> void:
	for p in _players.get(file, []):
		if p.playing:
			p.stop()
			return


# KSoundCache::GetNode + KWavSound::Play(pan, volume, loop) at a map position.  `unless_playing` is the
# IsPlaying check KMissleRes::PlaySound makes first (0x00717F1C); the cast sound has none.
func play(file: String, scene_pos: Vector2, loop: bool = false, unless_playing: bool = false) -> bool:
	if file == "" or file == "0":
		return false
	if unless_playing and is_playing(file):
		dropped += 1
		return false
	if not stream_provider.is_valid():
		return false
	var stream: AudioStream = stream_provider.call(file)
	if stream == null:
		return false
	var list: Array = _players.get(file, [])
	var player: AudioStreamPlayer2D = null
	for p in list:
		if not p.playing:
			player = p
			break
	if player == null:
		if list.size() >= BUFFER_COUNT:
			dropped += 1   # KWavSound::GetFreeBuffer found none
			return false
		player = AudioStreamPlayer2D.new()
		player.stream = stream
		player.max_distance = 1.0e9      # the volume law is the old one, not Godot's distance attenuation
		player.attenuation = 0.001
		add_child(player)
		list.append(player)
		_players[file] = list
	var f := focus()
	var dx := int(scene_pos.x - f.x)
	var dy := int(scene_pos.y - f.y)
	player.volume_db = float(snd_volume(dx, dy, option_volume)) / 100.0
	player.position = to_screen(scene_pos)
	if loop:
		if stream is AudioStreamWAV:
			stream.loop_mode = AudioStreamWAV.LOOP_FORWARD
	if player.is_inside_tree():   # a buffer plays only in a running tree (the headless tests build the node outside one)
		player.play()
	played += 1
	history.append(file.get_file())
	if history.size() > 16:
		history.pop_front()
	return true
