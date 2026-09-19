# Address of the gateway, as the player types it on the login screen.  Pure functions so the
# headless tests cover every form:
#
#   127.0.0.1:19100        raw TCP (default, what the old client had)
#   tcp://host:port        the same
#   tls://host:port        TCP inside TLS
#   ws://host:port/ws      WebSocket (web build, mobile behind a proxy)
#   wss://host:port/ws     WebSocket inside TLS
extends RefCounted

const DEFAULT_PORT := 19100
const DEFAULT_WS_PATH := "/ws"
const SCHEMES := ["wss://", "ws://", "tls://", "tcp://"]


# Returns {scheme: "tcp"|"tls"|"ws"|"wss", host, port, path}.
static func parse(address: String) -> Dictionary:
	var rest := address.strip_edges()
	var scheme := "tcp"
	for s in SCHEMES:
		if rest.to_lower().begins_with(s):
			scheme = s.substr(0, s.length() - 3)
			rest = rest.substr(s.length())
			break
	var path := DEFAULT_WS_PATH
	var slash := rest.find("/")
	if slash >= 0:
		path = rest.substr(slash)
		rest = rest.substr(0, slash)
	var host := rest
	var port := DEFAULT_PORT
	var colon := rest.rfind(":")
	if colon > 0 and rest.rfind("]") < colon:   # an ipv6 address in brackets keeps its own colons
		host = rest.substr(0, colon)
		var digits := rest.substr(colon + 1)
		if digits.is_valid_int():
			port = int(digits)
	if host == "":
		host = "127.0.0.1"
	if port <= 0 or port > 65535:
		port = DEFAULT_PORT
	if path == "":
		path = DEFAULT_WS_PATH
	return {"scheme": scheme, "host": host, "port": port, "path": path}


# "ws://host:port/path" for the WebSocket peer.
static func url(a: Dictionary) -> String:
	return "%s://%s:%d%s" % [a["scheme"], a["host"], a["port"], a["path"]]


static func is_websocket(a: Dictionary) -> bool:
	return a["scheme"] == "ws" or a["scheme"] == "wss"


static func is_tls(a: Dictionary) -> bool:
	return a["scheme"] == "tls" or a["scheme"] == "wss"
