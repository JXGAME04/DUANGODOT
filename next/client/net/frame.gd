# Protocol V2 framing (next/docs/PROTOCOL.md), pure functions with no autoload dependency so the
# headless tests can check them against the vectors shared with C++ and Go:
#   u32 len | u16 msg | u16 flags | payload      little-endian, len counts msg+flags+payload
extends RefCounted

const LENGTH_SIZE := 4
const HEADER_SIZE := 4
const MAX_PAYLOAD := 64 * 1024


static func encode(msg_id: int, payload: PackedByteArray, flags: int = 0) -> PackedByteArray:
	var out := PackedByteArray()
	out.resize(LENGTH_SIZE + HEADER_SIZE)
	out.encode_u32(0, HEADER_SIZE + payload.size())
	out.encode_u16(4, msg_id)
	out.encode_u16(6, flags)
	out.append_array(payload)
	return out


# Extracts complete frames from buf.  Returns {"frames": [[msg_id, flags, payload], ...],
# "rest": remaining bytes, "error": "" | "corrupt" | "too_large"}.
static func parse(buf: PackedByteArray, max_payload: int = MAX_PAYLOAD) -> Dictionary:
	var frames := []
	var pos := 0
	var err := ""
	while buf.size() - pos >= LENGTH_SIZE:
		var length := buf.decode_u32(pos)
		if length < HEADER_SIZE:
			err = "corrupt"
			break
		if length - HEADER_SIZE > max_payload:
			err = "too_large"
			break
		if buf.size() - pos < LENGTH_SIZE + length:
			break
		var msg_id := buf.decode_u16(pos + 4)
		var flags := buf.decode_u16(pos + 6)
		var payload := buf.slice(pos + 8, pos + LENGTH_SIZE + length)
		frames.append([msg_id, flags, payload])
		pos += LENGTH_SIZE + length
	return {"frames": frames, "rest": buf.slice(pos), "error": err}
