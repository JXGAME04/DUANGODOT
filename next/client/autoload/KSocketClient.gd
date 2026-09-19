# Net (KSocketClient, cf. old S3Client/NetConnect) - one connection to the gateway using the
# Protocol V2 framing (net/KProtocol.gd), over whichever transport the address asks for:
#
#   127.0.0.1:19100        raw TCP          (PC, LAN, the old game had only this)
#   tcp://host:port        the same
#   tls://host:port        TCP inside TLS   (public server)
#   ws://host:port/ws      WebSocket        (web build, mobile behind a proxy)
#   wss://host:port/ws     WebSocket in TLS
#
# Everything above this file - framing, protobuf, the session flow - is the same on all four.
# Emits `message(msg_id, payload)` for every complete frame.
extends Node

const Frame := preload("res://net/KProtocol.gd")
const Address := preload("res://net/KNetAddress.gd")

signal connected
signal disconnected(reason: String)
signal message(msg_id: int, payload: PackedByteArray)

var _mode := "tcp"                  # tcp | tls | ws
var _tcp: StreamPeerTCP = null      # tcp and tls
var _tls: StreamPeerTLS = null      # tls only
var _ws: WebSocketPeer = null       # ws / wss
var _stream: StreamPeer = null      # the peer bytes are read from and written to (tcp/tls)
var _buf := PackedByteArray()
var _was_connected := false
var _connecting := false
var _tls_handshaking := false
var _host := ""
var frames_in := 0
var frames_out := 0
var bytes_in := 0
var bytes_out := 0
var transport := "tcp"              # what the current connection actually uses (HUD / logs)


# Splits "wss://host:port/path"; see net/KNetAddress.gd.
static func parse_address(address: String) -> Dictionary:
	return Address.parse(address)


func is_connected_to_server() -> bool:
	if _mode == "ws":
		return _ws != null and _ws.get_ready_state() == WebSocketPeer.STATE_OPEN
	return _tcp != null and _tcp.get_status() == StreamPeerTCP.STATUS_CONNECTED and not _tls_handshaking


# True when the certificate of a tls:// / wss:// server is not checked (dev machines).
func _insecure_tls() -> bool:
	return "--tls-insecure" in OS.get_cmdline_user_args() or OS.get_environment("JX_TLS_INSECURE") == "1"


func connect_to(address: String, port: int = 0) -> Error:
	disconnect_from("reconnect")
	var a := Address.parse(address)
	if port > 0:
		a["port"] = port   # old call style: connect_to(host, port)
	_host = a["host"]
	_buf = PackedByteArray()
	_tls = null
	_stream = null
	_ws = null
	_tcp = null
	_tls_handshaking = false
	match a["scheme"]:
		"ws", "wss":
			_mode = "ws"
			transport = a["scheme"]
			_ws = WebSocketPeer.new()
			_ws.inbound_buffer_size = 1 << 20
			_ws.outbound_buffer_size = 1 << 20
			var url: String = Address.url(a)
			var opts: TLSOptions = TLSOptions.client_unsafe() if (a["scheme"] == "wss" and _insecure_tls()) else null
			var err := _ws.connect_to_url(url, opts)
			if err != OK:
				Log.error("net", "connect failed", {"url": url, "error": error_string(err)})
				return err
			Log.info("net", "connecting", {"url": url, "transport": transport})
		_:
			_mode = "tls" if a["scheme"] == "tls" else "tcp"
			transport = _mode
			_tcp = StreamPeerTCP.new()
			var err2 := _tcp.connect_to_host(a["host"], a["port"])
			if err2 != OK:
				Log.error("net", "connect failed", {"host": a["host"], "port": a["port"], "error": error_string(err2)})
				return err2
			_stream = _tcp
			Log.info("net", "connecting", {"host": a["host"], "port": a["port"], "transport": transport})
	_connecting = true
	return OK


func disconnect_from(reason: String = "client") -> void:
	if _mode == "ws":
		if _ws != null and _ws.get_ready_state() != WebSocketPeer.STATE_CLOSED:
			_ws.close()
	elif _tcp != null and _tcp.get_status() != StreamPeerTCP.STATUS_NONE:
		_tcp.disconnect_from_host()
	if _was_connected:
		_was_connected = false
		Log.info("net", "disconnected", {"reason": reason, "transport": transport})
		disconnected.emit(reason)
	_connecting = false
	_tls_handshaking = false


func send(msg_id: int, payload: PackedByteArray, flags: int = 0) -> void:
	if not is_connected_to_server():
		Log.warn("net", "send while disconnected", {"msg": msg_id})
		return
	var frame := Frame.encode(msg_id, payload, flags)
	var err := OK
	if _mode == "ws":
		err = _ws.put_packet(frame)     # one frame per websocket message
	else:
		err = _stream.put_data(frame)
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
	if _mode == "ws":
		_poll_ws()
	else:
		_poll_stream()


func _poll_ws() -> void:
	if _ws == null:
		return
	_ws.poll()
	match _ws.get_ready_state():
		WebSocketPeer.STATE_OPEN:
			if not _was_connected:
				_open()
			while _ws.get_available_packet_count() > 0:
				var pkt := _ws.get_packet()
				bytes_in += pkt.size()
				_buf.append_array(pkt)
			_drain_buffer()
		WebSocketPeer.STATE_CLOSED:
			if _was_connected or _connecting:
				var code := _ws.get_close_code()
				var reason := "connection closed" if code >= 0 else "connect failed"
				if _connecting and not _was_connected:
					Log.error("net", "connect failed", {"reason": reason, "code": code})
					_connecting = false
					disconnected.emit(reason)
				else:
					disconnect_from(reason)


func _poll_stream() -> void:
	if _tcp == null:
		return
	_tcp.poll()
	var status := _tcp.get_status()
	if status == StreamPeerTCP.STATUS_CONNECTED:
		if _mode == "tls" and _tls == null:
			_start_tls()
			return
		if _mode == "tls":
			_tls.poll()
			match _tls.get_status():
				StreamPeerTLS.STATUS_HANDSHAKING:
					return
				StreamPeerTLS.STATUS_CONNECTED:
					if _tls_handshaking:
						_tls_handshaking = false
				_:
					Log.error("net", "tls handshake failed", {"status": _tls.get_status()})
					disconnect_from("tls handshake failed")
					return
		if not _was_connected:
			_tcp.set_no_delay(true)   # only valid once the socket is open
			_open()
		_read_stream()
	elif status == StreamPeerTCP.STATUS_ERROR or (status == StreamPeerTCP.STATUS_NONE and (_was_connected or _connecting)):
		var reason := "connection error" if status == StreamPeerTCP.STATUS_ERROR else "connection closed"
		if _connecting and not _was_connected:
			Log.error("net", "connect failed", {"reason": reason})
			_connecting = false
			disconnected.emit(reason)
		else:
			disconnect_from(reason)


func _start_tls() -> void:
	_tls = StreamPeerTLS.new()
	var opts: TLSOptions = TLSOptions.client_unsafe() if _insecure_tls() else TLSOptions.client()
	var err := _tls.connect_to_stream(_tcp, _host, opts)
	if err != OK:
		Log.error("net", "tls connect failed", {"error": error_string(err)})
		disconnect_from("tls connect failed")
		return
	_stream = _tls
	_tls_handshaking = true


func _open() -> void:
	_was_connected = true
	_connecting = false
	var where := _host if _mode == "ws" else "%s:%d" % [_tcp.get_connected_host(), _tcp.get_connected_port()]
	Log.info("net", "connected", {"peer": where, "transport": transport})
	connected.emit()


func _read_stream() -> void:
	var avail := _stream.get_available_bytes()
	if avail <= 0:
		return
	var res := _stream.get_data(avail)
	if res[0] != OK:
		disconnect_from("read failed")
		return
	var chunk: PackedByteArray = res[1]
	bytes_in += chunk.size()
	_buf.append_array(chunk)
	_drain_buffer()


func _drain_buffer() -> void:
	var parsed := Frame.parse(_buf)
	_buf = parsed["rest"]
	for f in parsed["frames"]:
		frames_in += 1
		Log.trace("net.recv", "from gateway", {"msg": f[0], "bytes": f[2].size()})
		message.emit(f[0], f[2])
	if parsed["error"] != "":
		Log.error("net", "bad frame from gateway", {"error": parsed["error"]})
		disconnect_from("bad frame")
