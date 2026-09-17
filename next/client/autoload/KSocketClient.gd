# Net (KSocketClient, cf. old S3Client/NetConnect) - one TCP connection to the gateway using
# the Protocol V2 framing (net/KProtocol.gd).
# Emits `message(msg_id, payload)` for every complete frame.
extends Node

const Frame := preload("res://net/KProtocol.gd")

signal connected
signal disconnected(reason: String)
signal message(msg_id: int, payload: PackedByteArray)

var _peer: StreamPeerTCP = StreamPeerTCP.new()
var _buf := PackedByteArray()
var _was_connected := false
var _connecting := false
var frames_in := 0
var frames_out := 0
var bytes_in := 0
var bytes_out := 0


func is_connected_to_server() -> bool:
	return _peer.get_status() == StreamPeerTCP.STATUS_CONNECTED


func connect_to(host: String, port: int) -> Error:
	disconnect_from("reconnect")
	_peer = StreamPeerTCP.new()
	_buf = PackedByteArray()
	var err := _peer.connect_to_host(host, port)
	if err != OK:
		Log.error("net", "connect failed", {"host": host, "port": port, "error": error_string(err)})
		return err
	_connecting = true
	Log.info("net", "connecting", {"host": host, "port": port})
	return OK


func disconnect_from(reason: String = "client") -> void:
	if _peer.get_status() != StreamPeerTCP.STATUS_NONE:
		_peer.disconnect_from_host()
	if _was_connected:
		_was_connected = false
		Log.info("net", "disconnected", {"reason": reason})
		disconnected.emit(reason)
	_connecting = false


func send(msg_id: int, payload: PackedByteArray, flags: int = 0) -> void:
	if not is_connected_to_server():
		Log.warn("net", "send while disconnected", {"msg": msg_id})
		return
	var frame := Frame.encode(msg_id, payload, flags)
	var err := _peer.put_data(frame)
	if err != OK:
		Log.error("net", "send failed", {"msg": msg_id, "error": error_string(err)})
		disconnect_from("send failed")
		return
	frames_out += 1
	bytes_out += frame.size()
	Log.trace("net.send", "to gateway", {"msg": msg_id, "bytes": payload.size()})


# pb is a godobuf message object (has to_bytes()).
func send_msg(msg_id: int, pb) -> void:
	send(msg_id, pb.to_bytes())


func _process(_delta: float) -> void:
	_peer.poll()
	var status := _peer.get_status()
	if status == StreamPeerTCP.STATUS_CONNECTED:
		if not _was_connected:
			_was_connected = true
			_connecting = false
			_peer.set_no_delay(true)   # only valid once the socket is open
			Log.info("net", "connected", {"host": _peer.get_connected_host(), "port": _peer.get_connected_port()})
			connected.emit()
		_read_available()
	elif status == StreamPeerTCP.STATUS_ERROR or (status == StreamPeerTCP.STATUS_NONE and (_was_connected or _connecting)):
		var reason := "connection error" if status == StreamPeerTCP.STATUS_ERROR else "connection closed"
		if _connecting and not _was_connected:
			Log.error("net", "connect failed", {"reason": reason})
			_connecting = false
			disconnected.emit(reason)
		else:
			disconnect_from(reason)


func _read_available() -> void:
	var avail := _peer.get_available_bytes()
	if avail <= 0:
		return
	var res := _peer.get_data(avail)
	if res[0] != OK:
		disconnect_from("read failed")
		return
	var chunk: PackedByteArray = res[1]
	bytes_in += chunk.size()
	_buf.append_array(chunk)
	var parsed := Frame.parse(_buf)
	_buf = parsed["rest"]
	for f in parsed["frames"]:
		frames_in += 1
		Log.trace("net.recv", "from gateway", {"msg": f[0], "bytes": f[2].size()})
		message.emit(f[0], f[2])
	if parsed["error"] != "":
		Log.error("net", "bad frame from gateway", {"error": parsed["error"]})
		disconnect_from("bad frame")
