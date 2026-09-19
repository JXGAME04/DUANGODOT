#
# BSD 3-Clause License
#
# Copyright (c) 2018 - 2026, Oleg Malyavkin
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# DEBUG_TAB redefine this "  " if you need, example: const DEBUG_TAB = "\t"

const PROTO_VERSION = 3

const DEBUG_TAB : String = "  "

enum PB_ERR {
	NO_ERRORS = 0,
	VARINT_NOT_FOUND = -1,
	REPEATED_COUNT_NOT_FOUND = -2,
	REPEATED_COUNT_MISMATCH = -3,
	LENGTHDEL_SIZE_NOT_FOUND = -4,
	LENGTHDEL_SIZE_MISMATCH = -5,
	PACKAGE_SIZE_MISMATCH = -6,
	UNDEFINED_STATE = -7,
	PARSE_INCOMPLETE = -8,
	REQUIRED_FIELDS = -9
}

enum PB_DATA_TYPE {
	INT32 = 0,
	SINT32 = 1,
	UINT32 = 2,
	INT64 = 3,
	SINT64 = 4,
	UINT64 = 5,
	BOOL = 6,
	ENUM = 7,
	FIXED32 = 8,
	SFIXED32 = 9,
	FLOAT = 10,
	FIXED64 = 11,
	SFIXED64 = 12,
	DOUBLE = 13,
	STRING = 14,
	BYTES = 15,
	MESSAGE = 16,
	MAP = 17
}

const DEFAULT_VALUES_2 = {
	PB_DATA_TYPE.INT32: null,
	PB_DATA_TYPE.SINT32: null,
	PB_DATA_TYPE.UINT32: null,
	PB_DATA_TYPE.INT64: null,
	PB_DATA_TYPE.SINT64: null,
	PB_DATA_TYPE.UINT64: null,
	PB_DATA_TYPE.BOOL: null,
	PB_DATA_TYPE.ENUM: null,
	PB_DATA_TYPE.FIXED32: null,
	PB_DATA_TYPE.SFIXED32: null,
	PB_DATA_TYPE.FLOAT: null,
	PB_DATA_TYPE.FIXED64: null,
	PB_DATA_TYPE.SFIXED64: null,
	PB_DATA_TYPE.DOUBLE: null,
	PB_DATA_TYPE.STRING: null,
	PB_DATA_TYPE.BYTES: null,
	PB_DATA_TYPE.MESSAGE: null,
	PB_DATA_TYPE.MAP: null
}

const DEFAULT_VALUES_3 = {
	PB_DATA_TYPE.INT32: 0,
	PB_DATA_TYPE.SINT32: 0,
	PB_DATA_TYPE.UINT32: 0,
	PB_DATA_TYPE.INT64: 0,
	PB_DATA_TYPE.SINT64: 0,
	PB_DATA_TYPE.UINT64: 0,
	PB_DATA_TYPE.BOOL: false,
	PB_DATA_TYPE.ENUM: 0,
	PB_DATA_TYPE.FIXED32: 0,
	PB_DATA_TYPE.SFIXED32: 0,
	PB_DATA_TYPE.FLOAT: 0.0,
	PB_DATA_TYPE.FIXED64: 0,
	PB_DATA_TYPE.SFIXED64: 0,
	PB_DATA_TYPE.DOUBLE: 0.0,
	PB_DATA_TYPE.STRING: "",
	PB_DATA_TYPE.BYTES: [],
	PB_DATA_TYPE.MESSAGE: null,
	PB_DATA_TYPE.MAP: []
}

enum PB_TYPE {
	VARINT = 0,
	FIX64 = 1,
	LENGTHDEL = 2,
	STARTGROUP = 3,
	ENDGROUP = 4,
	FIX32 = 5,
	UNDEFINED = 8
}

enum PB_RULE {
	OPTIONAL = 0,
	REQUIRED = 1,
	REPEATED = 2,
	RESERVED = 3
}

enum PB_SERVICE_STATE {
	FILLED = 0,
	UNFILLED = 1
}

class PBField:
	extends RefCounted
	func _init(a_name : String, a_type : int, a_rule : int, a_tag : int, packed : bool, a_value = null):
		name = a_name
		type = a_type
		rule = a_rule
		tag = a_tag
		option_packed = packed
		value = a_value
		
	var name : String
	var type : int
	var rule : int
	var tag : int
	var option_packed : bool
	var value
	var is_map_field : bool = false
	var option_default : bool = false

class PBTypeTag:
	extends RefCounted
	var ok : bool = false
	var type : int
	var tag : int
	var offset : int

class PBServiceField:
	extends RefCounted
	var field : PBField
	var func_ref = null
	var state : int = PB_SERVICE_STATE.UNFILLED

class PBPacker:
	static func convert_signed(n : int) -> int:
		if n < -2147483648:
			return (n << 1) ^ (n >> 63)
		else:
			return (n << 1) ^ (n >> 31)

	static func deconvert_signed(n : int) -> int:
		if n & 0x01:
			return ~(n >> 1)
		else:
			return (n >> 1)

	static func pack_varint(value) -> PackedByteArray:
		var varint : PackedByteArray = PackedByteArray()
		if typeof(value) == TYPE_BOOL:
			if value:
				value = 1
			else:
				value = 0
		for _i in range(9):
			var b = value & 0x7F
			value >>= 7
			if value:
				varint.append(b | 0x80)
			else:
				varint.append(b)
				break
		if varint.size() == 9 && (varint[8] & 0x80 != 0):
			varint.append(0x01)
		return varint

	static func pack_bytes(value, count : int, data_type : int) -> PackedByteArray:
		var bytes : PackedByteArray = PackedByteArray()
		if data_type == PB_DATA_TYPE.FLOAT:
			var spb : StreamPeerBuffer = StreamPeerBuffer.new()
			spb.put_float(value)
			bytes = spb.get_data_array()
		elif data_type == PB_DATA_TYPE.DOUBLE:
			var spb : StreamPeerBuffer = StreamPeerBuffer.new()
			spb.put_double(value)
			bytes = spb.get_data_array()
		else:
			for _i in range(count):
				bytes.append(value & 0xFF)
				value >>= 8
		return bytes

	static func unpack_bytes(bytes : PackedByteArray, index : int, count : int, data_type : int):
		if data_type == PB_DATA_TYPE.FLOAT:
			return bytes.decode_float(index)
		elif data_type == PB_DATA_TYPE.DOUBLE:
			return bytes.decode_double(index)
		elif data_type == PB_DATA_TYPE.FIXED32:
			return bytes.decode_u32(index)
		elif data_type == PB_DATA_TYPE.SFIXED32:
			return bytes.decode_s32(index)
		elif data_type == PB_DATA_TYPE.FIXED64:
			return bytes.decode_u64(index)
		elif data_type == PB_DATA_TYPE.SFIXED64:
			return bytes.decode_s64(index)
		else:
			var value : int = 0
			for i in range(count):
				value |= bytes[index + i] << (8 * i)
			return value

	static func unpack_varint(varint_bytes) -> int:
		var value : int = 0
		var i: int = varint_bytes.size() - 1
		while i > -1:
			value = (value << 7) | (varint_bytes[i] & 0x7F)
			i -= 1
		return value

	static func pack_type_tag(type : int, tag : int) -> PackedByteArray:
		return pack_varint((tag << 3) | type)

	static func isolate_varint(bytes : PackedByteArray, index : int) -> PackedByteArray:
		var i: int = index
		while i <= index + 10 && i < bytes.size(): # Protobuf varint max size is 10 bytes
			if !(bytes[i] & 0x80):
				return bytes.slice(index, i + 1)
			i += 1
		return [] # Unreachable

	static func unpack_type_tag(bytes : PackedByteArray, index : int) -> PBTypeTag:
		var varint_bytes : PackedByteArray = isolate_varint(bytes, index)
		var result : PBTypeTag = PBTypeTag.new()
		if varint_bytes.size() != 0:
			result.ok = true
			result.offset = varint_bytes.size()
			var unpacked : int = unpack_varint(varint_bytes)
			result.type = unpacked & 0x07
			result.tag = unpacked >> 3
		return result

	static func pack_length_delimeted(type : int, tag : int, bytes : PackedByteArray) -> PackedByteArray:
		var result : PackedByteArray = pack_type_tag(type, tag)
		result.append_array(pack_varint(bytes.size()))
		result.append_array(bytes)
		return result

	static func pb_type_from_data_type(data_type : int) -> int:
		if data_type == PB_DATA_TYPE.INT32 || data_type == PB_DATA_TYPE.SINT32 || data_type == PB_DATA_TYPE.UINT32 || data_type == PB_DATA_TYPE.INT64 || data_type == PB_DATA_TYPE.SINT64 || data_type == PB_DATA_TYPE.UINT64 || data_type == PB_DATA_TYPE.BOOL || data_type == PB_DATA_TYPE.ENUM:
			return PB_TYPE.VARINT
		elif data_type == PB_DATA_TYPE.FIXED32 || data_type == PB_DATA_TYPE.SFIXED32 || data_type == PB_DATA_TYPE.FLOAT:
			return PB_TYPE.FIX32
		elif data_type == PB_DATA_TYPE.FIXED64 || data_type == PB_DATA_TYPE.SFIXED64 || data_type == PB_DATA_TYPE.DOUBLE:
			return PB_TYPE.FIX64
		elif data_type == PB_DATA_TYPE.STRING || data_type == PB_DATA_TYPE.BYTES || data_type == PB_DATA_TYPE.MESSAGE || data_type == PB_DATA_TYPE.MAP:
			return PB_TYPE.LENGTHDEL
		else:
			return PB_TYPE.UNDEFINED

	static func pack_field(field : PBField) -> PackedByteArray:
		var type : int = pb_type_from_data_type(field.type)
		var type_copy : int = type
		if field.rule == PB_RULE.REPEATED && field.option_packed:
			type = PB_TYPE.LENGTHDEL
		var head : PackedByteArray = pack_type_tag(type, field.tag)
		var data : PackedByteArray = PackedByteArray()
		if type == PB_TYPE.VARINT:
			var value
			if field.rule == PB_RULE.REPEATED:
				for v in field.value:
					data.append_array(head)
					if field.type == PB_DATA_TYPE.SINT32 || field.type == PB_DATA_TYPE.SINT64:
						value = convert_signed(v)
					else:
						value = v
					data.append_array(pack_varint(value))
				return data
			else:
				if field.type == PB_DATA_TYPE.SINT32 || field.type == PB_DATA_TYPE.SINT64:
					value = convert_signed(field.value)
				else:
					value = field.value
				data = pack_varint(value)
		elif type == PB_TYPE.FIX32:
			if field.rule == PB_RULE.REPEATED:
				for v in field.value:
					data.append_array(head)
					data.append_array(pack_bytes(v, 4, field.type))
				return data
			else:
				data.append_array(pack_bytes(field.value, 4, field.type))
		elif type == PB_TYPE.FIX64:
			if field.rule == PB_RULE.REPEATED:
				for v in field.value:
					data.append_array(head)
					data.append_array(pack_bytes(v, 8, field.type))
				return data
			else:
				data.append_array(pack_bytes(field.value, 8, field.type))
		elif type == PB_TYPE.LENGTHDEL:
			if field.rule == PB_RULE.REPEATED:
				if type_copy == PB_TYPE.VARINT:
					if field.type == PB_DATA_TYPE.SINT32 || field.type == PB_DATA_TYPE.SINT64:
						var signed_value : int
						for v in field.value:
							signed_value = convert_signed(v)
							data.append_array(pack_varint(signed_value))
					else:
						for v in field.value:
							data.append_array(pack_varint(v))
					return pack_length_delimeted(type, field.tag, data)
				elif type_copy == PB_TYPE.FIX32:
					for v in field.value:
						data.append_array(pack_bytes(v, 4, field.type))
					return pack_length_delimeted(type, field.tag, data)
				elif type_copy == PB_TYPE.FIX64:
					for v in field.value:
						data.append_array(pack_bytes(v, 8, field.type))
					return pack_length_delimeted(type, field.tag, data)
				elif field.type == PB_DATA_TYPE.STRING:
					for v in field.value:
						var obj = v.to_utf8_buffer()
						data.append_array(pack_length_delimeted(type, field.tag, obj))
					return data
				elif field.type == PB_DATA_TYPE.BYTES:
					for v in field.value:
						data.append_array(pack_length_delimeted(type, field.tag, v))
					return data
				elif typeof(field.value[0]) == TYPE_OBJECT:
					for v in field.value:
						var obj : PackedByteArray = v.to_bytes()
						data.append_array(pack_length_delimeted(type, field.tag, obj))
					return data
			else:
				if field.type == PB_DATA_TYPE.STRING:
					var str_bytes : PackedByteArray = field.value.to_utf8_buffer()
					if PROTO_VERSION == 2 || (PROTO_VERSION == 3 && str_bytes.size() > 0):
						data.append_array(str_bytes)
						return pack_length_delimeted(type, field.tag, data)
				if field.type == PB_DATA_TYPE.BYTES:
					if PROTO_VERSION == 2 || (PROTO_VERSION == 3 && field.value.size() > 0):
						data.append_array(field.value)
						return pack_length_delimeted(type, field.tag, data)
				elif typeof(field.value) == TYPE_OBJECT:
					var obj : PackedByteArray = field.value.to_bytes()
					if obj.size() > 0:
						data.append_array(obj)
					return pack_length_delimeted(type, field.tag, data)
				else:
					pass
		if data.size() > 0:
			head.append_array(data)
			return head
		else:
			return data

	static func skip_unknown_field(bytes : PackedByteArray, offset : int, type : int) -> int:
		if type == PB_TYPE.VARINT:
			return offset + isolate_varint(bytes, offset).size()
		if type == PB_TYPE.FIX64:
			return offset + 8
		if type == PB_TYPE.LENGTHDEL:
			var length_bytes : PackedByteArray = isolate_varint(bytes, offset)
			var length : int = unpack_varint(length_bytes)
			return offset + length_bytes.size() + length
		if type == PB_TYPE.FIX32:
			return offset + 4
		return PB_ERR.UNDEFINED_STATE

	static func unpack_field(bytes : PackedByteArray, offset : int, field : PBField, type : int, message_func_ref) -> int:
		if field.rule == PB_RULE.REPEATED && type != PB_TYPE.LENGTHDEL && field.option_packed:
			var count = isolate_varint(bytes, offset)
			if count.size() > 0:
				offset += count.size()
				count = unpack_varint(count)
				if type == PB_TYPE.VARINT:
					var val
					var counter = offset + count
					while offset < counter:
						val = isolate_varint(bytes, offset)
						if val.size() > 0:
							offset += val.size()
							val = unpack_varint(val)
							if field.type == PB_DATA_TYPE.SINT32 || field.type == PB_DATA_TYPE.SINT64:
								val = deconvert_signed(val)
							elif field.type == PB_DATA_TYPE.BOOL:
								if val:
									val = true
								else:
									val = false
							field.value.append(val)
						else:
							return PB_ERR.REPEATED_COUNT_MISMATCH
					return offset
				elif type == PB_TYPE.FIX32 || type == PB_TYPE.FIX64:
					var type_size
					if type == PB_TYPE.FIX32:
						type_size = 4
					else:
						type_size = 8
					var val
					var counter = offset + count
					while offset < counter:
						if (offset + type_size) > bytes.size():
							return PB_ERR.REPEATED_COUNT_MISMATCH
						val = unpack_bytes(bytes, offset, type_size, field.type)
						offset += type_size
						field.value.append(val)
					return offset
			else:
				return PB_ERR.REPEATED_COUNT_NOT_FOUND
		else:
			if type == PB_TYPE.VARINT:
				var val = isolate_varint(bytes, offset)
				if val.size() > 0:
					offset += val.size()
					val = unpack_varint(val)
					if field.type == PB_DATA_TYPE.SINT32 || field.type == PB_DATA_TYPE.SINT64:
						val = deconvert_signed(val)
					elif field.type == PB_DATA_TYPE.BOOL:
						if val:
							val = true
						else:
							val = false
					if field.rule == PB_RULE.REPEATED:
						field.value.append(val)
					else:
						field.value = val
				else:
					return PB_ERR.VARINT_NOT_FOUND
				return offset
			elif type == PB_TYPE.FIX32 || type == PB_TYPE.FIX64:
				var type_size
				if type == PB_TYPE.FIX32:
					type_size = 4
				else:
					type_size = 8
				var val
				if (offset + type_size) > bytes.size():
					return PB_ERR.REPEATED_COUNT_MISMATCH
				val = unpack_bytes(bytes, offset, type_size, field.type)
				offset += type_size
				if field.rule == PB_RULE.REPEATED:
					field.value.append(val)
				else:
					field.value = val
				return offset
			elif type == PB_TYPE.LENGTHDEL:
				var inner_size = isolate_varint(bytes, offset)
				if inner_size.size() > 0:
					offset += inner_size.size()
					inner_size = unpack_varint(inner_size)
					if inner_size >= 0:
						if inner_size + offset > bytes.size():
							return PB_ERR.LENGTHDEL_SIZE_MISMATCH
						if message_func_ref != null:
							var message = message_func_ref.call()
							if inner_size > 0:
								var sub_offset = message.from_bytes(bytes, offset, inner_size + offset)
								if sub_offset > 0:
									if sub_offset - offset >= inner_size:
										offset = sub_offset
										return offset
									else:
										return PB_ERR.LENGTHDEL_SIZE_MISMATCH
								return sub_offset
							else:
								return offset
						elif field.type == PB_DATA_TYPE.STRING:
							var str_bytes : PackedByteArray = bytes.slice(offset, inner_size + offset)
							if field.rule == PB_RULE.REPEATED:
								field.value.append(str_bytes.get_string_from_utf8())
							else:
								field.value = str_bytes.get_string_from_utf8()
							return offset + inner_size
						elif field.type == PB_DATA_TYPE.BYTES:
							var val_bytes : PackedByteArray = bytes.slice(offset, inner_size + offset)
							if field.rule == PB_RULE.REPEATED:
								field.value.append(val_bytes)
							else:
								field.value = val_bytes
							return offset + inner_size
					else:
						return PB_ERR.LENGTHDEL_SIZE_NOT_FOUND
				else:
					return PB_ERR.LENGTHDEL_SIZE_NOT_FOUND
		return PB_ERR.UNDEFINED_STATE

	static func unpack_message(data, bytes : PackedByteArray, offset : int, limit : int) -> int:
		while true:
			var tt : PBTypeTag = unpack_type_tag(bytes, offset)
			if tt.ok:
				offset += tt.offset
				if data.has(tt.tag):
					var service : PBServiceField = data[tt.tag]
					var type : int = pb_type_from_data_type(service.field.type)
					if type == tt.type || (tt.type == PB_TYPE.LENGTHDEL && service.field.rule == PB_RULE.REPEATED && service.field.option_packed):
						var res : int = unpack_field(bytes, offset, service.field, type, service.func_ref)
						if res > 0:
							service.state = PB_SERVICE_STATE.FILLED
							offset = res
							if offset == limit:
								return offset
							elif offset > limit:
								return PB_ERR.PACKAGE_SIZE_MISMATCH
						elif res < 0:
							return res
						else:
							break
				else:
					var res : int = skip_unknown_field(bytes, offset, tt.type)
					if res > 0:
						offset = res
						if offset == limit:
							return offset
						elif offset > limit:
							return PB_ERR.PACKAGE_SIZE_MISMATCH
					elif res < 0:
						return res
					else:
						break							
			else:
				return offset
		return PB_ERR.UNDEFINED_STATE

	static func pack_message(data) -> PackedByteArray:
		var DEFAULT_VALUES
		if PROTO_VERSION == 2:
			DEFAULT_VALUES = DEFAULT_VALUES_2
		elif PROTO_VERSION == 3:
			DEFAULT_VALUES = DEFAULT_VALUES_3
		var result : PackedByteArray = PackedByteArray()
		var keys : Array = data.keys()
		keys.sort()
		for i in keys:
			if data[i].field.value != null:
				if data[i].state == PB_SERVICE_STATE.UNFILLED \
				&& !data[i].field.is_map_field \
				&& typeof(data[i].field.value) == typeof(DEFAULT_VALUES[data[i].field.type]) \
				&& data[i].field.value == DEFAULT_VALUES[data[i].field.type]:
					continue
				elif data[i].field.rule == PB_RULE.REPEATED && data[i].field.value.size() == 0:
					continue
				result.append_array(pack_field(data[i].field))
			elif data[i].field.rule == PB_RULE.REQUIRED:
				print("Error: required field is not filled: Tag:", data[i].field.tag)
				return PackedByteArray()
		return result

	static func check_required(data) -> bool:
		var keys : Array = data.keys()
		for i in keys:
			if data[i].field.rule == PB_RULE.REQUIRED && data[i].state == PB_SERVICE_STATE.UNFILLED:
				return false
		return true

	static func construct_map(key_values):
		var result = {}
		for kv in key_values:
			result[kv.get_key()] = kv.get_value()
		return result
	
	static func tabulate(text : String, nesting : int) -> String:
		var tab : String = ""
		for _i in range(nesting):
			tab += DEBUG_TAB
		return tab + text
	
	static func value_to_string(value, field : PBField, nesting : int) -> String:
		var result : String = ""
		var text : String
		if field.type == PB_DATA_TYPE.MESSAGE:
			result += "{"
			nesting += 1
			text = message_to_string(value.data, nesting)
			if text != "":
				result += "\n" + text
				nesting -= 1
				result += tabulate("}", nesting)
			else:
				nesting -= 1
				result += "}"
		elif field.type == PB_DATA_TYPE.BYTES:
			result += "<"
			for i in range(value.size()):
				result += str(value[i])
				if i != (value.size() - 1):
					result += ", "
			result += ">"
		elif field.type == PB_DATA_TYPE.STRING:
			result += "\"" + value + "\""
		elif field.type == PB_DATA_TYPE.ENUM:
			result += "ENUM::" + str(value)
		else:
			result += str(value)
		return result
	
	static func field_to_string(field : PBField, nesting : int) -> String:
		var result : String = tabulate(field.name + ": ", nesting)
		if field.type == PB_DATA_TYPE.MAP:
			if field.value.size() > 0:
				result += "(\n"
				nesting += 1
				for i in range(field.value.size()):
					var local_key_value = field.value[i].data[1].field
					result += tabulate(value_to_string(local_key_value.value, local_key_value, nesting), nesting) + ": "
					local_key_value = field.value[i].data[2].field
					result += value_to_string(local_key_value.value, local_key_value, nesting)
					if i != (field.value.size() - 1):
						result += ","
					result += "\n"
				nesting -= 1
				result += tabulate(")", nesting)
			else:
				result += "()"
		elif field.rule == PB_RULE.REPEATED:
			if field.value.size() > 0:
				result += "[\n"
				nesting += 1
				for i in range(field.value.size()):
					result += tabulate(str(i) + ": ", nesting)
					result += value_to_string(field.value[i], field, nesting)
					if i != (field.value.size() - 1):
						result += ","
					result += "\n"
				nesting -= 1
				result += tabulate("]", nesting)
			else:
				result += "[]"
		else:
			result += value_to_string(field.value, field, nesting)
		result += ";\n"
		return result
		
	static func message_to_string(data, nesting : int = 0) -> String:
		var DEFAULT_VALUES
		if PROTO_VERSION == 2:
			DEFAULT_VALUES = DEFAULT_VALUES_2
		elif PROTO_VERSION == 3:
			DEFAULT_VALUES = DEFAULT_VALUES_3
		var result : String = ""
		var keys : Array = data.keys()
		keys.sort()
		for i in keys:
			if data[i].field.value != null:
				if data[i].state == PB_SERVICE_STATE.UNFILLED \
				&& !data[i].field.is_map_field \
				&& typeof(data[i].field.value) == typeof(DEFAULT_VALUES[data[i].field.type]) \
				&& data[i].field.value == DEFAULT_VALUES[data[i].field.type]:
					continue
				elif data[i].field.rule == PB_RULE.REPEATED && data[i].field.value.size() == 0:
					continue
				result += field_to_string(data[i].field, nesting)
			elif data[i].field.rule == PB_RULE.REQUIRED:
				result += data[i].field.name + ": " + "error"
		return result



############### USER DATA BEGIN ################


class Hello:
	extends RefCounted
	func _init():
		var service
		
		__protocol_version = PBField.new("protocol_version", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __protocol_version
		data[__protocol_version.tag] = service
		
		__client_version = PBField.new("client_version", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __client_version
		data[__client_version.tag] = service
		
		__platform = PBField.new("platform", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __platform
		data[__platform.tag] = service
		
		__device_id = PBField.new("device_id", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __device_id
		data[__device_id.tag] = service
		
	var data = {}
	
	var __protocol_version: PBField
	func has_protocol_version() -> bool:
		if __protocol_version.value != null:
			return true
		return false
	func get_protocol_version() -> int:
		return __protocol_version.value
	func clear_protocol_version() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__protocol_version.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_protocol_version(value : int) -> void:
		__protocol_version.value = value
	
	var __client_version: PBField
	func has_client_version() -> bool:
		if __client_version.value != null:
			return true
		return false
	func get_client_version() -> String:
		return __client_version.value
	func clear_client_version() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__client_version.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_client_version(value : String) -> void:
		__client_version.value = value
	
	var __platform: PBField
	func has_platform() -> bool:
		if __platform.value != null:
			return true
		return false
	func get_platform() -> String:
		return __platform.value
	func clear_platform() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__platform.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_platform(value : String) -> void:
		__platform.value = value
	
	var __device_id: PBField
	func has_device_id() -> bool:
		if __device_id.value != null:
			return true
		return false
	func get_device_id() -> String:
		return __device_id.value
	func clear_device_id() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__device_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_device_id(value : String) -> void:
		__device_id.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class HelloAck:
	extends RefCounted
	func _init():
		var service
		
		__protocol_version = PBField.new("protocol_version", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __protocol_version
		data[__protocol_version.tag] = service
		
		__server_time_ms = PBField.new("server_time_ms", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __server_time_ms
		data[__server_time_ms.tag] = service
		
		__server_version = PBField.new("server_version", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __server_version
		data[__server_version.tag] = service
		
		__sid = PBField.new("sid", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __sid
		data[__sid.tag] = service
		
		__auth_mode = PBField.new("auth_mode", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __auth_mode
		data[__auth_mode.tag] = service
		
		__heartbeat_s = PBField.new("heartbeat_s", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __heartbeat_s
		data[__heartbeat_s.tag] = service
		
	var data = {}
	
	var __protocol_version: PBField
	func has_protocol_version() -> bool:
		if __protocol_version.value != null:
			return true
		return false
	func get_protocol_version() -> int:
		return __protocol_version.value
	func clear_protocol_version() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__protocol_version.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_protocol_version(value : int) -> void:
		__protocol_version.value = value
	
	var __server_time_ms: PBField
	func has_server_time_ms() -> bool:
		if __server_time_ms.value != null:
			return true
		return false
	func get_server_time_ms() -> int:
		return __server_time_ms.value
	func clear_server_time_ms() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__server_time_ms.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_server_time_ms(value : int) -> void:
		__server_time_ms.value = value
	
	var __server_version: PBField
	func has_server_version() -> bool:
		if __server_version.value != null:
			return true
		return false
	func get_server_version() -> String:
		return __server_version.value
	func clear_server_version() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__server_version.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_server_version(value : String) -> void:
		__server_version.value = value
	
	var __sid: PBField
	func has_sid() -> bool:
		if __sid.value != null:
			return true
		return false
	func get_sid() -> int:
		return __sid.value
	func clear_sid() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__sid.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_sid(value : int) -> void:
		__sid.value = value
	
	var __auth_mode: PBField
	func has_auth_mode() -> bool:
		if __auth_mode.value != null:
			return true
		return false
	func get_auth_mode() -> String:
		return __auth_mode.value
	func clear_auth_mode() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__auth_mode.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_auth_mode(value : String) -> void:
		__auth_mode.value = value
	
	var __heartbeat_s: PBField
	func has_heartbeat_s() -> bool:
		if __heartbeat_s.value != null:
			return true
		return false
	func get_heartbeat_s() -> int:
		return __heartbeat_s.value
	func clear_heartbeat_s() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__heartbeat_s.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_heartbeat_s(value : int) -> void:
		__heartbeat_s.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class LoginReq:
	extends RefCounted
	func _init():
		var service
		
		__account = PBField.new("account", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __account
		data[__account.tag] = service
		
		__password = PBField.new("password", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __password
		data[__password.tag] = service
		
	var data = {}
	
	var __account: PBField
	func has_account() -> bool:
		if __account.value != null:
			return true
		return false
	func get_account() -> String:
		return __account.value
	func clear_account() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__account.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_account(value : String) -> void:
		__account.value = value
	
	var __password: PBField
	func has_password() -> bool:
		if __password.value != null:
			return true
		return false
	func get_password() -> String:
		return __password.value
	func clear_password() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__password.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_password(value : String) -> void:
		__password.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class LoginRes:
	extends RefCounted
	func _init():
		var service
		
		__result = PBField.new("result", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __result
		data[__result.tag] = service
		
		__account_id = PBField.new("account_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __account_id
		data[__account_id.tag] = service
		
		__text = PBField.new("text", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __text
		data[__text.tag] = service
		
	var data = {}
	
	var __result: PBField
	func has_result() -> bool:
		if __result.value != null:
			return true
		return false
	func get_result():
		return __result.value
	func clear_result() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__result.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_result(value) -> void:
		__result.value = value
	
	var __account_id: PBField
	func has_account_id() -> bool:
		if __account_id.value != null:
			return true
		return false
	func get_account_id() -> int:
		return __account_id.value
	func clear_account_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__account_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_account_id(value : int) -> void:
		__account_id.value = value
	
	var __text: PBField
	func has_text() -> bool:
		if __text.value != null:
			return true
		return false
	func get_text() -> String:
		return __text.value
	func clear_text() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__text.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_text(value : String) -> void:
		__text.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class CharSummary:
	extends RefCounted
	func _init():
		var service
		
		__player_id = PBField.new("player_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __player_id
		data[__player_id.tag] = service
		
		__name = PBField.new("name", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __name
		data[__name.tag] = service
		
		__level = PBField.new("level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
		__series = PBField.new("series", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __series
		data[__series.tag] = service
		
		__sex = PBField.new("sex", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __sex
		data[__sex.tag] = service
		
		__faction = PBField.new("faction", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __faction
		data[__faction.tag] = service
		
		__zone_id = PBField.new("zone_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __zone_id
		data[__zone_id.tag] = service
		
	var data = {}
	
	var __player_id: PBField
	func has_player_id() -> bool:
		if __player_id.value != null:
			return true
		return false
	func get_player_id() -> int:
		return __player_id.value
	func clear_player_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__player_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_player_id(value : int) -> void:
		__player_id.value = value
	
	var __name: PBField
	func has_name() -> bool:
		if __name.value != null:
			return true
		return false
	func get_name() -> String:
		return __name.value
	func clear_name() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__name.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_name(value : String) -> void:
		__name.value = value
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	var __series: PBField
	func has_series() -> bool:
		if __series.value != null:
			return true
		return false
	func get_series() -> int:
		return __series.value
	func clear_series() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__series.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_series(value : int) -> void:
		__series.value = value
	
	var __sex: PBField
	func has_sex() -> bool:
		if __sex.value != null:
			return true
		return false
	func get_sex() -> int:
		return __sex.value
	func clear_sex() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__sex.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_sex(value : int) -> void:
		__sex.value = value
	
	var __faction: PBField
	func has_faction() -> bool:
		if __faction.value != null:
			return true
		return false
	func get_faction() -> int:
		return __faction.value
	func clear_faction() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__faction.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_faction(value : int) -> void:
		__faction.value = value
	
	var __zone_id: PBField
	func has_zone_id() -> bool:
		if __zone_id.value != null:
			return true
		return false
	func get_zone_id() -> int:
		return __zone_id.value
	func clear_zone_id() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__zone_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_zone_id(value : int) -> void:
		__zone_id.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class CharListReq:
	extends RefCounted
	func _init():
		var service
		
	var data = {}
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class CharListRes:
	extends RefCounted
	func _init():
		var service
		
		__result = PBField.new("result", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __result
		data[__result.tag] = service
		
		var __chars_default: Array[CharSummary] = []
		__chars = PBField.new("chars", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 2, true, __chars_default)
		service = PBServiceField.new()
		service.field = __chars
		service.func_ref = Callable(self, "add_chars")
		data[__chars.tag] = service
		
		__max_chars = PBField.new("max_chars", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __max_chars
		data[__max_chars.tag] = service
		
	var data = {}
	
	var __result: PBField
	func has_result() -> bool:
		if __result.value != null:
			return true
		return false
	func get_result():
		return __result.value
	func clear_result() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__result.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_result(value) -> void:
		__result.value = value
	
	var __chars: PBField
	func get_chars() -> Array[CharSummary]:
		return __chars.value
	func clear_chars() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__chars.value.clear()
	func add_chars() -> CharSummary:
		var element = CharSummary.new()
		__chars.value.append(element)
		return element
	
	var __max_chars: PBField
	func has_max_chars() -> bool:
		if __max_chars.value != null:
			return true
		return false
	func get_max_chars() -> int:
		return __max_chars.value
	func clear_max_chars() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__max_chars.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_max_chars(value : int) -> void:
		__max_chars.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class CharCreateReq:
	extends RefCounted
	func _init():
		var service
		
		__name = PBField.new("name", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __name
		data[__name.tag] = service
		
		__series = PBField.new("series", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __series
		data[__series.tag] = service
		
		__sex = PBField.new("sex", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __sex
		data[__sex.tag] = service
		
		__native_place = PBField.new("native_place", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __native_place
		data[__native_place.tag] = service
		
	var data = {}
	
	var __name: PBField
	func has_name() -> bool:
		if __name.value != null:
			return true
		return false
	func get_name() -> String:
		return __name.value
	func clear_name() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__name.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_name(value : String) -> void:
		__name.value = value
	
	var __series: PBField
	func has_series() -> bool:
		if __series.value != null:
			return true
		return false
	func get_series() -> int:
		return __series.value
	func clear_series() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__series.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_series(value : int) -> void:
		__series.value = value
	
	var __sex: PBField
	func has_sex() -> bool:
		if __sex.value != null:
			return true
		return false
	func get_sex() -> int:
		return __sex.value
	func clear_sex() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__sex.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_sex(value : int) -> void:
		__sex.value = value
	
	var __native_place: PBField
	func has_native_place() -> bool:
		if __native_place.value != null:
			return true
		return false
	func get_native_place() -> int:
		return __native_place.value
	func clear_native_place() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__native_place.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_native_place(value : int) -> void:
		__native_place.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class CharCreateRes:
	extends RefCounted
	func _init():
		var service
		
		__result = PBField.new("result", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __result
		data[__result.tag] = service
		
		__summary = PBField.new("summary", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __summary
		service.func_ref = Callable(self, "new_summary")
		data[__summary.tag] = service
		
	var data = {}
	
	var __result: PBField
	func has_result() -> bool:
		if __result.value != null:
			return true
		return false
	func get_result():
		return __result.value
	func clear_result() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__result.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_result(value) -> void:
		__result.value = value
	
	var __summary: PBField
	func has_summary() -> bool:
		if __summary.value != null:
			return true
		return false
	func get_summary() -> CharSummary:
		return __summary.value
	func clear_summary() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__summary.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_summary() -> CharSummary:
		__summary.value = CharSummary.new()
		return __summary.value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EnterWorldReq:
	extends RefCounted
	func _init():
		var service
		
		__player_id = PBField.new("player_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __player_id
		data[__player_id.tag] = service
		
	var data = {}
	
	var __player_id: PBField
	func has_player_id() -> bool:
		if __player_id.value != null:
			return true
		return false
	func get_player_id() -> int:
		return __player_id.value
	func clear_player_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__player_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_player_id(value : int) -> void:
		__player_id.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EnterWorldRes:
	extends RefCounted
	func _init():
		var service
		
		__result = PBField.new("result", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __result
		data[__result.tag] = service
		
		__zone_id = PBField.new("zone_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __zone_id
		data[__zone_id.tag] = service
		
		__zone_name = PBField.new("zone_name", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __zone_name
		data[__zone_name.tag] = service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__pos = PBField.new("pos", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __pos
		service.func_ref = Callable(self, "new_pos")
		data[__pos.tag] = service
		
		__tick_hz = PBField.new("tick_hz", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __tick_hz
		data[__tick_hz.tag] = service
		
		__map_id = PBField.new("map_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __map_id
		data[__map_id.tag] = service
		
		__scene_w = PBField.new("scene_w", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __scene_w
		data[__scene_w.tag] = service
		
		__scene_h = PBField.new("scene_h", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 9, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __scene_h
		data[__scene_h.tag] = service
		
	var data = {}
	
	var __result: PBField
	func has_result() -> bool:
		if __result.value != null:
			return true
		return false
	func get_result():
		return __result.value
	func clear_result() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__result.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_result(value) -> void:
		__result.value = value
	
	var __zone_id: PBField
	func has_zone_id() -> bool:
		if __zone_id.value != null:
			return true
		return false
	func get_zone_id() -> int:
		return __zone_id.value
	func clear_zone_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__zone_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_zone_id(value : int) -> void:
		__zone_id.value = value
	
	var __zone_name: PBField
	func has_zone_name() -> bool:
		if __zone_name.value != null:
			return true
		return false
	func get_zone_name() -> String:
		return __zone_name.value
	func clear_zone_name() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__zone_name.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_zone_name(value : String) -> void:
		__zone_name.value = value
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __pos: PBField
	func has_pos() -> bool:
		if __pos.value != null:
			return true
		return false
	func get_pos() -> Vec2:
		return __pos.value
	func clear_pos() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__pos.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_pos() -> Vec2:
		__pos.value = Vec2.new()
		return __pos.value
	
	var __tick_hz: PBField
	func has_tick_hz() -> bool:
		if __tick_hz.value != null:
			return true
		return false
	func get_tick_hz() -> int:
		return __tick_hz.value
	func clear_tick_hz() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__tick_hz.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_tick_hz(value : int) -> void:
		__tick_hz.value = value
	
	var __map_id: PBField
	func has_map_id() -> bool:
		if __map_id.value != null:
			return true
		return false
	func get_map_id() -> int:
		return __map_id.value
	func clear_map_id() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__map_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_map_id(value : int) -> void:
		__map_id.value = value
	
	var __scene_w: PBField
	func has_scene_w() -> bool:
		if __scene_w.value != null:
			return true
		return false
	func get_scene_w() -> int:
		return __scene_w.value
	func clear_scene_w() -> void:
		data[8].state = PB_SERVICE_STATE.UNFILLED
		__scene_w.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_scene_w(value : int) -> void:
		__scene_w.value = value
	
	var __scene_h: PBField
	func has_scene_h() -> bool:
		if __scene_h.value != null:
			return true
		return false
	func get_scene_h() -> int:
		return __scene_h.value
	func clear_scene_h() -> void:
		data[9].state = PB_SERVICE_STATE.UNFILLED
		__scene_h.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_scene_h(value : int) -> void:
		__scene_h.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class LeaveWorldReq:
	extends RefCounted
	func _init():
		var service
		
	var data = {}
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class MoveReq:
	extends RefCounted
	func _init():
		var service
		
		__target = PBField.new("target", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __target
		service.func_ref = Callable(self, "new_target")
		data[__target.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __target: PBField
	func has_target() -> bool:
		if __target.value != null:
			return true
		return false
	func get_target() -> Vec2:
		return __target.value
	func clear_target() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__target.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_target() -> Vec2:
		__target.value = Vec2.new()
		return __target.value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class AttackReq:
	extends RefCounted
	func _init():
		var service
		
		__target = PBField.new("target", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __target
		data[__target.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __target: PBField
	func has_target() -> bool:
		if __target.value != null:
			return true
		return false
	func get_target() -> int:
		return __target.value
	func clear_target() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__target.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_target(value : int) -> void:
		__target.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
enum Action {
	STAND = 0,
	ATTACK = 1,
	HURT = 2,
	DEATH = 3,
	REVIVE = 4,
	JUMP = 5,
	KNOCK_BACK = 6,
	SIT = 7
}

class EntityAction:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__action = PBField.new("action", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __action
		data[__action.tag] = service
		
		__target = PBField.new("target", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __target
		data[__target.tag] = service
		
		__dir = PBField.new("dir", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __dir
		data[__dir.tag] = service
		
		__frames = PBField.new("frames", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __frames
		data[__frames.tag] = service
		
		__pos = PBField.new("pos", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __pos
		service.func_ref = Callable(self, "new_pos")
		data[__pos.tag] = service
		
		__tick = PBField.new("tick", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __tick
		data[__tick.tag] = service
		
		__skill_id = PBField.new("skill_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __skill_id
		data[__skill_id.tag] = service
		
		__skill_level = PBField.new("skill_level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 9, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __skill_level
		data[__skill_level.tag] = service
		
		__aim = PBField.new("aim", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 10, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __aim
		service.func_ref = Callable(self, "new_aim")
		data[__aim.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __action: PBField
	func has_action() -> bool:
		if __action.value != null:
			return true
		return false
	func get_action():
		return __action.value
	func clear_action() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__action.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_action(value) -> void:
		__action.value = value
	
	var __target: PBField
	func has_target() -> bool:
		if __target.value != null:
			return true
		return false
	func get_target() -> int:
		return __target.value
	func clear_target() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__target.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_target(value : int) -> void:
		__target.value = value
	
	var __dir: PBField
	func has_dir() -> bool:
		if __dir.value != null:
			return true
		return false
	func get_dir() -> int:
		return __dir.value
	func clear_dir() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__dir.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_dir(value : int) -> void:
		__dir.value = value
	
	var __frames: PBField
	func has_frames() -> bool:
		if __frames.value != null:
			return true
		return false
	func get_frames() -> int:
		return __frames.value
	func clear_frames() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__frames.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_frames(value : int) -> void:
		__frames.value = value
	
	var __pos: PBField
	func has_pos() -> bool:
		if __pos.value != null:
			return true
		return false
	func get_pos() -> Vec2:
		return __pos.value
	func clear_pos() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__pos.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_pos() -> Vec2:
		__pos.value = Vec2.new()
		return __pos.value
	
	var __tick: PBField
	func has_tick() -> bool:
		if __tick.value != null:
			return true
		return false
	func get_tick() -> int:
		return __tick.value
	func clear_tick() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__tick.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_tick(value : int) -> void:
		__tick.value = value
	
	var __skill_id: PBField
	func has_skill_id() -> bool:
		if __skill_id.value != null:
			return true
		return false
	func get_skill_id() -> int:
		return __skill_id.value
	func clear_skill_id() -> void:
		data[8].state = PB_SERVICE_STATE.UNFILLED
		__skill_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_skill_id(value : int) -> void:
		__skill_id.value = value
	
	var __skill_level: PBField
	func has_skill_level() -> bool:
		if __skill_level.value != null:
			return true
		return false
	func get_skill_level() -> int:
		return __skill_level.value
	func clear_skill_level() -> void:
		data[9].state = PB_SERVICE_STATE.UNFILLED
		__skill_level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_skill_level(value : int) -> void:
		__skill_level.value = value
	
	var __aim: PBField
	func has_aim() -> bool:
		if __aim.value != null:
			return true
		return false
	func get_aim() -> Vec2:
		return __aim.value
	func clear_aim() -> void:
		data[10].state = PB_SERVICE_STATE.UNFILLED
		__aim.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_aim() -> Vec2:
		__aim.value = Vec2.new()
		return __aim.value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EntityLife:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__life = PBField.new("life", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __life
		data[__life.tag] = service
		
		__life_max = PBField.new("life_max", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __life_max
		data[__life_max.tag] = service
		
		__delta = PBField.new("delta", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __delta
		data[__delta.tag] = service
		
		__source = PBField.new("source", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __source
		data[__source.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __life: PBField
	func has_life() -> bool:
		if __life.value != null:
			return true
		return false
	func get_life() -> int:
		return __life.value
	func clear_life() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__life.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_life(value : int) -> void:
		__life.value = value
	
	var __life_max: PBField
	func has_life_max() -> bool:
		if __life_max.value != null:
			return true
		return false
	func get_life_max() -> int:
		return __life_max.value
	func clear_life_max() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__life_max.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_life_max(value : int) -> void:
		__life_max.value = value
	
	var __delta: PBField
	func has_delta() -> bool:
		if __delta.value != null:
			return true
		return false
	func get_delta() -> int:
		return __delta.value
	func clear_delta() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__delta.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_delta(value : int) -> void:
		__delta.value = value
	
	var __source: PBField
	func has_source() -> bool:
		if __source.value != null:
			return true
		return false
	func get_source() -> int:
		return __source.value
	func clear_source() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__source.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_source(value : int) -> void:
		__source.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EntityInfo:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__entity_type = PBField.new("entity_type", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __entity_type
		data[__entity_type.tag] = service
		
		__name = PBField.new("name", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __name
		data[__name.tag] = service
		
		__pos = PBField.new("pos", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __pos
		service.func_ref = Callable(self, "new_pos")
		data[__pos.tag] = service
		
		__target = PBField.new("target", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __target
		service.func_ref = Callable(self, "new_target")
		data[__target.tag] = service
		
		__move_speed = PBField.new("move_speed", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __move_speed
		data[__move_speed.tag] = service
		
		__level = PBField.new("level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
		__series = PBField.new("series", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __series
		data[__series.tag] = service
		
		__sex = PBField.new("sex", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 9, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __sex
		data[__sex.tag] = service
		
		__template_id = PBField.new("template_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 10, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __template_id
		data[__template_id.tag] = service
		
		var __path_default: Array[Vec2] = []
		__path = PBField.new("path", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 11, true, __path_default)
		service = PBServiceField.new()
		service.field = __path
		service.func_ref = Callable(self, "add_path")
		data[__path.tag] = service
		
		__dir = PBField.new("dir", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 12, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __dir
		data[__dir.tag] = service
		
		__life = PBField.new("life", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 13, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __life
		data[__life.tag] = service
		
		__life_max = PBField.new("life_max", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 14, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __life_max
		data[__life_max.tag] = service
		
		__doing = PBField.new("doing", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 15, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __doing
		data[__doing.tag] = service
		
		__doing_frames = PBField.new("doing_frames", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 16, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __doing_frames
		data[__doing_frames.tag] = service
		
		__count = PBField.new("count", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 17, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __count
		data[__count.tag] = service
		
		__hide = PBField.new("hide", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 18, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __hide
		data[__hide.tag] = service
		
		__riding = PBField.new("riding", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 19, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __riding
		data[__riding.tag] = service
		
		__gold_type = PBField.new("gold_type", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 20, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __gold_type
		data[__gold_type.tag] = service
		
		__camp = PBField.new("camp", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 21, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __camp
		data[__camp.tag] = service
		
		__current_camp = PBField.new("current_camp", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 22, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __current_camp
		data[__current_camp.tag] = service
		
		__helm_res = PBField.new("helm_res", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 23, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __helm_res
		data[__helm_res.tag] = service
		
		__armor_res = PBField.new("armor_res", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 24, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __armor_res
		data[__armor_res.tag] = service
		
		__weapon_res = PBField.new("weapon_res", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 25, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __weapon_res
		data[__weapon_res.tag] = service
		
		__horse_res = PBField.new("horse_res", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 26, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __horse_res
		data[__horse_res.tag] = service
		
		__mantle_res = PBField.new("mantle_res", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 27, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __mantle_res
		data[__mantle_res.tag] = service
		
		__pk_state = PBField.new("pk_state", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 28, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __pk_state
		data[__pk_state.tag] = service
		
		__menu_state = PBField.new("menu_state", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 29, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __menu_state
		data[__menu_state.tag] = service
		
		__menu_sentence = PBField.new("menu_sentence", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 30, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __menu_sentence
		data[__menu_sentence.tag] = service
		
		__npc_kind = PBField.new("npc_kind", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 31, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __npc_kind
		data[__npc_kind.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __entity_type: PBField
	func has_entity_type() -> bool:
		if __entity_type.value != null:
			return true
		return false
	func get_entity_type():
		return __entity_type.value
	func clear_entity_type() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__entity_type.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_entity_type(value) -> void:
		__entity_type.value = value
	
	var __name: PBField
	func has_name() -> bool:
		if __name.value != null:
			return true
		return false
	func get_name() -> String:
		return __name.value
	func clear_name() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__name.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_name(value : String) -> void:
		__name.value = value
	
	var __pos: PBField
	func has_pos() -> bool:
		if __pos.value != null:
			return true
		return false
	func get_pos() -> Vec2:
		return __pos.value
	func clear_pos() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__pos.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_pos() -> Vec2:
		__pos.value = Vec2.new()
		return __pos.value
	
	var __target: PBField
	func has_target() -> bool:
		if __target.value != null:
			return true
		return false
	func get_target() -> Vec2:
		return __target.value
	func clear_target() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__target.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_target() -> Vec2:
		__target.value = Vec2.new()
		return __target.value
	
	var __move_speed: PBField
	func has_move_speed() -> bool:
		if __move_speed.value != null:
			return true
		return false
	func get_move_speed() -> int:
		return __move_speed.value
	func clear_move_speed() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__move_speed.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_move_speed(value : int) -> void:
		__move_speed.value = value
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	var __series: PBField
	func has_series() -> bool:
		if __series.value != null:
			return true
		return false
	func get_series() -> int:
		return __series.value
	func clear_series() -> void:
		data[8].state = PB_SERVICE_STATE.UNFILLED
		__series.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_series(value : int) -> void:
		__series.value = value
	
	var __sex: PBField
	func has_sex() -> bool:
		if __sex.value != null:
			return true
		return false
	func get_sex() -> int:
		return __sex.value
	func clear_sex() -> void:
		data[9].state = PB_SERVICE_STATE.UNFILLED
		__sex.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_sex(value : int) -> void:
		__sex.value = value
	
	var __template_id: PBField
	func has_template_id() -> bool:
		if __template_id.value != null:
			return true
		return false
	func get_template_id() -> int:
		return __template_id.value
	func clear_template_id() -> void:
		data[10].state = PB_SERVICE_STATE.UNFILLED
		__template_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_template_id(value : int) -> void:
		__template_id.value = value
	
	var __path: PBField
	func get_path() -> Array[Vec2]:
		return __path.value
	func clear_path() -> void:
		data[11].state = PB_SERVICE_STATE.UNFILLED
		__path.value.clear()
	func add_path() -> Vec2:
		var element = Vec2.new()
		__path.value.append(element)
		return element
	
	var __dir: PBField
	func has_dir() -> bool:
		if __dir.value != null:
			return true
		return false
	func get_dir() -> int:
		return __dir.value
	func clear_dir() -> void:
		data[12].state = PB_SERVICE_STATE.UNFILLED
		__dir.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_dir(value : int) -> void:
		__dir.value = value
	
	var __life: PBField
	func has_life() -> bool:
		if __life.value != null:
			return true
		return false
	func get_life() -> int:
		return __life.value
	func clear_life() -> void:
		data[13].state = PB_SERVICE_STATE.UNFILLED
		__life.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_life(value : int) -> void:
		__life.value = value
	
	var __life_max: PBField
	func has_life_max() -> bool:
		if __life_max.value != null:
			return true
		return false
	func get_life_max() -> int:
		return __life_max.value
	func clear_life_max() -> void:
		data[14].state = PB_SERVICE_STATE.UNFILLED
		__life_max.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_life_max(value : int) -> void:
		__life_max.value = value
	
	var __doing: PBField
	func has_doing() -> bool:
		if __doing.value != null:
			return true
		return false
	func get_doing():
		return __doing.value
	func clear_doing() -> void:
		data[15].state = PB_SERVICE_STATE.UNFILLED
		__doing.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_doing(value) -> void:
		__doing.value = value
	
	var __doing_frames: PBField
	func has_doing_frames() -> bool:
		if __doing_frames.value != null:
			return true
		return false
	func get_doing_frames() -> int:
		return __doing_frames.value
	func clear_doing_frames() -> void:
		data[16].state = PB_SERVICE_STATE.UNFILLED
		__doing_frames.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_doing_frames(value : int) -> void:
		__doing_frames.value = value
	
	var __count: PBField
	func has_count() -> bool:
		if __count.value != null:
			return true
		return false
	func get_count() -> int:
		return __count.value
	func clear_count() -> void:
		data[17].state = PB_SERVICE_STATE.UNFILLED
		__count.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_count(value : int) -> void:
		__count.value = value
	
	var __hide: PBField
	func has_hide() -> bool:
		if __hide.value != null:
			return true
		return false
	func get_hide() -> int:
		return __hide.value
	func clear_hide() -> void:
		data[18].state = PB_SERVICE_STATE.UNFILLED
		__hide.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_hide(value : int) -> void:
		__hide.value = value
	
	var __riding: PBField
	func has_riding() -> bool:
		if __riding.value != null:
			return true
		return false
	func get_riding() -> bool:
		return __riding.value
	func clear_riding() -> void:
		data[19].state = PB_SERVICE_STATE.UNFILLED
		__riding.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_riding(value : bool) -> void:
		__riding.value = value
	
	var __gold_type: PBField
	func has_gold_type() -> bool:
		if __gold_type.value != null:
			return true
		return false
	func get_gold_type() -> int:
		return __gold_type.value
	func clear_gold_type() -> void:
		data[20].state = PB_SERVICE_STATE.UNFILLED
		__gold_type.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_gold_type(value : int) -> void:
		__gold_type.value = value
	
	var __camp: PBField
	func has_camp() -> bool:
		if __camp.value != null:
			return true
		return false
	func get_camp() -> int:
		return __camp.value
	func clear_camp() -> void:
		data[21].state = PB_SERVICE_STATE.UNFILLED
		__camp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_camp(value : int) -> void:
		__camp.value = value
	
	var __current_camp: PBField
	func has_current_camp() -> bool:
		if __current_camp.value != null:
			return true
		return false
	func get_current_camp() -> int:
		return __current_camp.value
	func clear_current_camp() -> void:
		data[22].state = PB_SERVICE_STATE.UNFILLED
		__current_camp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_current_camp(value : int) -> void:
		__current_camp.value = value
	
	var __helm_res: PBField
	func has_helm_res() -> bool:
		if __helm_res.value != null:
			return true
		return false
	func get_helm_res() -> int:
		return __helm_res.value
	func clear_helm_res() -> void:
		data[23].state = PB_SERVICE_STATE.UNFILLED
		__helm_res.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_helm_res(value : int) -> void:
		__helm_res.value = value
	
	var __armor_res: PBField
	func has_armor_res() -> bool:
		if __armor_res.value != null:
			return true
		return false
	func get_armor_res() -> int:
		return __armor_res.value
	func clear_armor_res() -> void:
		data[24].state = PB_SERVICE_STATE.UNFILLED
		__armor_res.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_armor_res(value : int) -> void:
		__armor_res.value = value
	
	var __weapon_res: PBField
	func has_weapon_res() -> bool:
		if __weapon_res.value != null:
			return true
		return false
	func get_weapon_res() -> int:
		return __weapon_res.value
	func clear_weapon_res() -> void:
		data[25].state = PB_SERVICE_STATE.UNFILLED
		__weapon_res.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_weapon_res(value : int) -> void:
		__weapon_res.value = value
	
	var __horse_res: PBField
	func has_horse_res() -> bool:
		if __horse_res.value != null:
			return true
		return false
	func get_horse_res() -> int:
		return __horse_res.value
	func clear_horse_res() -> void:
		data[26].state = PB_SERVICE_STATE.UNFILLED
		__horse_res.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_horse_res(value : int) -> void:
		__horse_res.value = value
	
	var __mantle_res: PBField
	func has_mantle_res() -> bool:
		if __mantle_res.value != null:
			return true
		return false
	func get_mantle_res() -> int:
		return __mantle_res.value
	func clear_mantle_res() -> void:
		data[27].state = PB_SERVICE_STATE.UNFILLED
		__mantle_res.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_mantle_res(value : int) -> void:
		__mantle_res.value = value
	
	var __pk_state: PBField
	func has_pk_state() -> bool:
		if __pk_state.value != null:
			return true
		return false
	func get_pk_state() -> int:
		return __pk_state.value
	func clear_pk_state() -> void:
		data[28].state = PB_SERVICE_STATE.UNFILLED
		__pk_state.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_pk_state(value : int) -> void:
		__pk_state.value = value
	
	var __menu_state: PBField
	func has_menu_state() -> bool:
		if __menu_state.value != null:
			return true
		return false
	func get_menu_state() -> int:
		return __menu_state.value
	func clear_menu_state() -> void:
		data[29].state = PB_SERVICE_STATE.UNFILLED
		__menu_state.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_menu_state(value : int) -> void:
		__menu_state.value = value
	
	var __menu_sentence: PBField
	func has_menu_sentence() -> bool:
		if __menu_sentence.value != null:
			return true
		return false
	func get_menu_sentence() -> String:
		return __menu_sentence.value
	func clear_menu_sentence() -> void:
		data[30].state = PB_SERVICE_STATE.UNFILLED
		__menu_sentence.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_menu_sentence(value : String) -> void:
		__menu_sentence.value = value
	
	var __npc_kind: PBField
	func has_npc_kind() -> bool:
		if __npc_kind.value != null:
			return true
		return false
	func get_npc_kind() -> int:
		return __npc_kind.value
	func clear_npc_kind() -> void:
		data[31].state = PB_SERVICE_STATE.UNFILLED
		__npc_kind.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_npc_kind(value : int) -> void:
		__npc_kind.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class NpcDialogReq:
	extends RefCounted
	func _init():
		var service
		
		__npc = PBField.new("npc", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __npc
		data[__npc.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __npc: PBField
	func has_npc() -> bool:
		if __npc.value != null:
			return true
		return false
	func get_npc() -> int:
		return __npc.value
	func clear_npc() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__npc.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_npc(value : int) -> void:
		__npc.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class DialogAnswer:
	extends RefCounted
	func _init():
		var service
		
		__index = PBField.new("index", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __index
		data[__index.tag] = service
		
		__kind = PBField.new("kind", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __kind
		data[__kind.tag] = service
		
	var data = {}
	
	var __index: PBField
	func has_index() -> bool:
		if __index.value != null:
			return true
		return false
	func get_index() -> int:
		return __index.value
	func clear_index() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__index.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_index(value : int) -> void:
		__index.value = value
	
	var __kind: PBField
	func has_kind() -> bool:
		if __kind.value != null:
			return true
		return false
	func get_kind() -> int:
		return __kind.value
	func clear_kind() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__kind.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_kind(value : int) -> void:
		__kind.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ScriptAction:
	extends RefCounted
	func _init():
		var service
		
		__operate = PBField.new("operate", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __operate
		data[__operate.tag] = service
		
		__ui_id = PBField.new("ui_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __ui_id
		data[__ui_id.tag] = service
		
		__text = PBField.new("text", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __text
		data[__text.tag] = service
		
		__text_id = PBField.new("text_id", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __text_id
		data[__text_id.tag] = service
		
		__interactive = PBField.new("interactive", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __interactive
		data[__interactive.tag] = service
		
		__param = PBField.new("param", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __param
		data[__param.tag] = service
		
		var __options_default: Array[String] = []
		__options = PBField.new("options", PB_DATA_TYPE.STRING, PB_RULE.REPEATED, 7, true, __options_default)
		service = PBServiceField.new()
		service.field = __options
		data[__options.tag] = service
		
		__notify_changes = PBField.new("notify_changes", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __notify_changes
		data[__notify_changes.tag] = service
		
	var data = {}
	
	var __operate: PBField
	func has_operate() -> bool:
		if __operate.value != null:
			return true
		return false
	func get_operate() -> int:
		return __operate.value
	func clear_operate() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__operate.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_operate(value : int) -> void:
		__operate.value = value
	
	var __ui_id: PBField
	func has_ui_id() -> bool:
		if __ui_id.value != null:
			return true
		return false
	func get_ui_id() -> int:
		return __ui_id.value
	func clear_ui_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__ui_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_ui_id(value : int) -> void:
		__ui_id.value = value
	
	var __text: PBField
	func has_text() -> bool:
		if __text.value != null:
			return true
		return false
	func get_text() -> String:
		return __text.value
	func clear_text() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__text.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_text(value : String) -> void:
		__text.value = value
	
	var __text_id: PBField
	func has_text_id() -> bool:
		if __text_id.value != null:
			return true
		return false
	func get_text_id() -> int:
		return __text_id.value
	func clear_text_id() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__text_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_text_id(value : int) -> void:
		__text_id.value = value
	
	var __interactive: PBField
	func has_interactive() -> bool:
		if __interactive.value != null:
			return true
		return false
	func get_interactive() -> bool:
		return __interactive.value
	func clear_interactive() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__interactive.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_interactive(value : bool) -> void:
		__interactive.value = value
	
	var __param: PBField
	func has_param() -> bool:
		if __param.value != null:
			return true
		return false
	func get_param() -> int:
		return __param.value
	func clear_param() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__param.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_param(value : int) -> void:
		__param.value = value
	
	var __options: PBField
	func get_options() -> Array[String]:
		return __options.value
	func clear_options() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__options.value.clear()
	func add_options(value : String) -> void:
		__options.value.append(value)
	
	var __notify_changes: PBField
	func has_notify_changes() -> bool:
		if __notify_changes.value != null:
			return true
		return false
	func get_notify_changes() -> bool:
		return __notify_changes.value
	func clear_notify_changes() -> void:
		data[8].state = PB_SERVICE_STATE.UNFILLED
		__notify_changes.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_notify_changes(value : bool) -> void:
		__notify_changes.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class TaskValue:
	extends RefCounted
	func _init():
		var service
		
		__id = PBField.new("id", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __id
		data[__id.tag] = service
		
		__value = PBField.new("value", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __value
		data[__value.tag] = service
		
	var data = {}
	
	var __id: PBField
	func has_id() -> bool:
		if __id.value != null:
			return true
		return false
	func get_id() -> int:
		return __id.value
	func clear_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_id(value : int) -> void:
		__id.value = value
	
	var __value: PBField
	func has_value() -> bool:
		if __value.value != null:
			return true
		return false
	func get_value() -> int:
		return __value.value
	func clear_value() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__value.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_value(value : int) -> void:
		__value.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class TaskValues:
	extends RefCounted
	func _init():
		var service
		
		var __values_default: Array[TaskValue] = []
		__values = PBField.new("values", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 1, true, __values_default)
		service = PBServiceField.new()
		service.field = __values
		service.func_ref = Callable(self, "add_values")
		data[__values.tag] = service
		
	var data = {}
	
	var __values: PBField
	func get_values() -> Array[TaskValue]:
		return __values.value
	func clear_values() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__values.value.clear()
	func add_values() -> TaskValue:
		var element = TaskValue.new()
		__values.value.append(element)
		return element
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class TaskValueReq:
	extends RefCounted
	func _init():
		var service
		
		__id = PBField.new("id", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __id
		data[__id.tag] = service
		
		__value = PBField.new("value", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __value
		data[__value.tag] = service
		
	var data = {}
	
	var __id: PBField
	func has_id() -> bool:
		if __id.value != null:
			return true
		return false
	func get_id() -> int:
		return __id.value
	func clear_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_id(value : int) -> void:
		__id.value = value
	
	var __value: PBField
	func has_value() -> bool:
		if __value.value != null:
			return true
		return false
	func get_value() -> int:
		return __value.value
	func clear_value() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__value.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_value(value : int) -> void:
		__value.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class TaskTip:
	extends RefCounted
	func _init():
		var service
		
		__text = PBField.new("text", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __text
		data[__text.tag] = service
		
	var data = {}
	
	var __text: PBField
	func has_text() -> bool:
		if __text.value != null:
			return true
		return false
	func get_text() -> String:
		return __text.value
	func clear_text() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__text.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_text(value : String) -> void:
		__text.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class GiveItemEntry:
	extends RefCounted
	func _init():
		var service
		
		__room = PBField.new("room", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __room
		data[__room.tag] = service
		
		__x = PBField.new("x", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __x
		data[__x.tag] = service
		
		__y = PBField.new("y", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __y
		data[__y.tag] = service
		
		__cell_x = PBField.new("cell_x", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __cell_x
		data[__cell_x.tag] = service
		
		__cell_y = PBField.new("cell_y", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __cell_y
		data[__cell_y.tag] = service
		
	var data = {}
	
	var __room: PBField
	func has_room() -> bool:
		if __room.value != null:
			return true
		return false
	func get_room() -> int:
		return __room.value
	func clear_room() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__room.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_room(value : int) -> void:
		__room.value = value
	
	var __x: PBField
	func has_x() -> bool:
		if __x.value != null:
			return true
		return false
	func get_x() -> int:
		return __x.value
	func clear_x() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__x.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_x(value : int) -> void:
		__x.value = value
	
	var __y: PBField
	func has_y() -> bool:
		if __y.value != null:
			return true
		return false
	func get_y() -> int:
		return __y.value
	func clear_y() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__y.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_y(value : int) -> void:
		__y.value = value
	
	var __cell_x: PBField
	func has_cell_x() -> bool:
		if __cell_x.value != null:
			return true
		return false
	func get_cell_x() -> int:
		return __cell_x.value
	func clear_cell_x() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__cell_x.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_cell_x(value : int) -> void:
		__cell_x.value = value
	
	var __cell_y: PBField
	func has_cell_y() -> bool:
		if __cell_y.value != null:
			return true
		return false
	func get_cell_y() -> int:
		return __cell_y.value
	func clear_cell_y() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__cell_y.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_cell_y(value : int) -> void:
		__cell_y.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class GiveItemsReq:
	extends RefCounted
	func _init():
		var service
		
		__kind = PBField.new("kind", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __kind
		data[__kind.tag] = service
		
		var __items_default: Array[GiveItemEntry] = []
		__items = PBField.new("items", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 2, true, __items_default)
		service = PBServiceField.new()
		service.field = __items
		service.func_ref = Callable(self, "add_items")
		data[__items.tag] = service
		
	var data = {}
	
	var __kind: PBField
	func has_kind() -> bool:
		if __kind.value != null:
			return true
		return false
	func get_kind() -> int:
		return __kind.value
	func clear_kind() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__kind.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_kind(value : int) -> void:
		__kind.value = value
	
	var __items: PBField
	func get_items() -> Array[GiveItemEntry]:
		return __items.value
	func clear_items() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__items.value.clear()
	func add_items() -> GiveItemEntry:
		var element = GiveItemEntry.new()
		__items.value.append(element)
		return element
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class GiveItemMsg:
	extends RefCounted
	func _init():
		var service
		
		__kind = PBField.new("kind", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __kind
		data[__kind.tag] = service
		
		__text = PBField.new("text", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __text
		data[__text.tag] = service
		
	var data = {}
	
	var __kind: PBField
	func has_kind() -> bool:
		if __kind.value != null:
			return true
		return false
	func get_kind() -> int:
		return __kind.value
	func clear_kind() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__kind.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_kind(value : int) -> void:
		__kind.value = value
	
	var __text: PBField
	func has_text() -> bool:
		if __text.value != null:
			return true
		return false
	func get_text() -> String:
		return __text.value
	func clear_text() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__text.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_text(value : String) -> void:
		__text.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ScriptAsk:
	extends RefCounted
	func _init():
		var service
		
		__kind = PBField.new("kind", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __kind
		data[__kind.tag] = service
		
		__title = PBField.new("title", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __title
		data[__title.tag] = service
		
		__min = PBField.new("min", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __min
		data[__min.tag] = service
		
		__max = PBField.new("max", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __max
		data[__max.tag] = service
		
		__default_text = PBField.new("default_text", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __default_text
		data[__default_text.tag] = service
		
	var data = {}
	
	var __kind: PBField
	func has_kind() -> bool:
		if __kind.value != null:
			return true
		return false
	func get_kind() -> int:
		return __kind.value
	func clear_kind() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__kind.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_kind(value : int) -> void:
		__kind.value = value
	
	var __title: PBField
	func has_title() -> bool:
		if __title.value != null:
			return true
		return false
	func get_title() -> String:
		return __title.value
	func clear_title() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__title.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_title(value : String) -> void:
		__title.value = value
	
	var __min: PBField
	func has_min() -> bool:
		if __min.value != null:
			return true
		return false
	func get_min() -> int:
		return __min.value
	func clear_min() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__min.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_min(value : int) -> void:
		__min.value = value
	
	var __max: PBField
	func has_max() -> bool:
		if __max.value != null:
			return true
		return false
	func get_max() -> int:
		return __max.value
	func clear_max() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__max.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_max(value : int) -> void:
		__max.value = value
	
	var __default_text: PBField
	func has_default_text() -> bool:
		if __default_text.value != null:
			return true
		return false
	func get_default_text() -> String:
		return __default_text.value
	func clear_default_text() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__default_text.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_default_text(value : String) -> void:
		__default_text.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ScriptInput:
	extends RefCounted
	func _init():
		var service
		
		__kind = PBField.new("kind", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __kind
		data[__kind.tag] = service
		
		__number = PBField.new("number", PB_DATA_TYPE.INT64, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT64])
		service = PBServiceField.new()
		service.field = __number
		data[__number.tag] = service
		
		__text = PBField.new("text", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __text
		data[__text.tag] = service
		
	var data = {}
	
	var __kind: PBField
	func has_kind() -> bool:
		if __kind.value != null:
			return true
		return false
	func get_kind() -> int:
		return __kind.value
	func clear_kind() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__kind.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_kind(value : int) -> void:
		__kind.value = value
	
	var __number: PBField
	func has_number() -> bool:
		if __number.value != null:
			return true
		return false
	func get_number() -> int:
		return __number.value
	func clear_number() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__number.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT64]
	func set_number(value : int) -> void:
		__number.value = value
	
	var __text: PBField
	func has_text() -> bool:
		if __text.value != null:
			return true
		return false
	func get_text() -> String:
		return __text.value
	func clear_text() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__text.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_text(value : String) -> void:
		__text.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class PKStateReq:
	extends RefCounted
	func _init():
		var service
		
		__state = PBField.new("state", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __state
		data[__state.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __state: PBField
	func has_state() -> bool:
		if __state.value != null:
			return true
		return false
	func get_state() -> int:
		return __state.value
	func clear_state() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__state.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_state(value : int) -> void:
		__state.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class PKState:
	extends RefCounted
	func _init():
		var service
		
		__state = PBField.new("state", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __state
		data[__state.tag] = service
		
		__value = PBField.new("value", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __value
		data[__value.tag] = service
		
		__refused = PBField.new("refused", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __refused
		data[__refused.tag] = service
		
	var data = {}
	
	var __state: PBField
	func has_state() -> bool:
		if __state.value != null:
			return true
		return false
	func get_state() -> int:
		return __state.value
	func clear_state() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__state.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_state(value : int) -> void:
		__state.value = value
	
	var __value: PBField
	func has_value() -> bool:
		if __value.value != null:
			return true
		return false
	func get_value() -> int:
		return __value.value
	func clear_value() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__value.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_value(value : int) -> void:
		__value.value = value
	
	var __refused: PBField
	func has_refused() -> bool:
		if __refused.value != null:
			return true
		return false
	func get_refused() -> bool:
		return __refused.value
	func clear_refused() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__refused.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_refused(value : bool) -> void:
		__refused.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EntityPK:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__pk_state = PBField.new("pk_state", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __pk_state
		data[__pk_state.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __pk_state: PBField
	func has_pk_state() -> bool:
		if __pk_state.value != null:
			return true
		return false
	func get_pk_state() -> int:
		return __pk_state.value
	func clear_pk_state() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__pk_state.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_pk_state(value : int) -> void:
		__pk_state.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EntityRes:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__helm_res = PBField.new("helm_res", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __helm_res
		data[__helm_res.tag] = service
		
		__armor_res = PBField.new("armor_res", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __armor_res
		data[__armor_res.tag] = service
		
		__weapon_res = PBField.new("weapon_res", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __weapon_res
		data[__weapon_res.tag] = service
		
		__horse_res = PBField.new("horse_res", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __horse_res
		data[__horse_res.tag] = service
		
		__mantle_res = PBField.new("mantle_res", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __mantle_res
		data[__mantle_res.tag] = service
		
		__version = PBField.new("version", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __version
		data[__version.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __helm_res: PBField
	func has_helm_res() -> bool:
		if __helm_res.value != null:
			return true
		return false
	func get_helm_res() -> int:
		return __helm_res.value
	func clear_helm_res() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__helm_res.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_helm_res(value : int) -> void:
		__helm_res.value = value
	
	var __armor_res: PBField
	func has_armor_res() -> bool:
		if __armor_res.value != null:
			return true
		return false
	func get_armor_res() -> int:
		return __armor_res.value
	func clear_armor_res() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__armor_res.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_armor_res(value : int) -> void:
		__armor_res.value = value
	
	var __weapon_res: PBField
	func has_weapon_res() -> bool:
		if __weapon_res.value != null:
			return true
		return false
	func get_weapon_res() -> int:
		return __weapon_res.value
	func clear_weapon_res() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__weapon_res.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_weapon_res(value : int) -> void:
		__weapon_res.value = value
	
	var __horse_res: PBField
	func has_horse_res() -> bool:
		if __horse_res.value != null:
			return true
		return false
	func get_horse_res() -> int:
		return __horse_res.value
	func clear_horse_res() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__horse_res.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_horse_res(value : int) -> void:
		__horse_res.value = value
	
	var __mantle_res: PBField
	func has_mantle_res() -> bool:
		if __mantle_res.value != null:
			return true
		return false
	func get_mantle_res() -> int:
		return __mantle_res.value
	func clear_mantle_res() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__mantle_res.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_mantle_res(value : int) -> void:
		__mantle_res.value = value
	
	var __version: PBField
	func has_version() -> bool:
		if __version.value != null:
			return true
		return false
	func get_version() -> int:
		return __version.value
	func clear_version() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__version.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_version(value : int) -> void:
		__version.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class NpcGold:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__gold_type = PBField.new("gold_type", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __gold_type
		data[__gold_type.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __gold_type: PBField
	func has_gold_type() -> bool:
		if __gold_type.value != null:
			return true
		return false
	func get_gold_type() -> int:
		return __gold_type.value
	func clear_gold_type() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__gold_type.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_gold_type(value : int) -> void:
		__gold_type.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EntityRide:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__riding = PBField.new("riding", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __riding
		data[__riding.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __riding: PBField
	func has_riding() -> bool:
		if __riding.value != null:
			return true
		return false
	func get_riding() -> bool:
		return __riding.value
	func clear_riding() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__riding.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_riding(value : bool) -> void:
		__riding.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class PickUpReq:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EntitySpawn:
	extends RefCounted
	func _init():
		var service
		
		var __entities_default: Array[EntityInfo] = []
		__entities = PBField.new("entities", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 1, true, __entities_default)
		service = PBServiceField.new()
		service.field = __entities
		service.func_ref = Callable(self, "add_entities")
		data[__entities.tag] = service
		
	var data = {}
	
	var __entities: PBField
	func get_entities() -> Array[EntityInfo]:
		return __entities.value
	func clear_entities() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entities.value.clear()
	func add_entities() -> EntityInfo:
		var element = EntityInfo.new()
		__entities.value.append(element)
		return element
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EntityDespawn:
	extends RefCounted
	func _init():
		var service
		
		var __entity_ids_default: Array[int] = []
		__entity_ids = PBField.new("entity_ids", PB_DATA_TYPE.UINT64, PB_RULE.REPEATED, 1, true, __entity_ids_default)
		service = PBServiceField.new()
		service.field = __entity_ids
		data[__entity_ids.tag] = service
		
	var data = {}
	
	var __entity_ids: PBField
	func get_entity_ids() -> Array[int]:
		return __entity_ids.value
	func clear_entity_ids() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_ids.value.clear()
	func add_entity_ids(value : int) -> void:
		__entity_ids.value.append(value)
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EntityMove:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__pos = PBField.new("pos", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __pos
		service.func_ref = Callable(self, "new_pos")
		data[__pos.tag] = service
		
		__target = PBField.new("target", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __target
		service.func_ref = Callable(self, "new_target")
		data[__target.tag] = service
		
		__move_speed = PBField.new("move_speed", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __move_speed
		data[__move_speed.tag] = service
		
		__tick = PBField.new("tick", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __tick
		data[__tick.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
		var __path_default: Array[Vec2] = []
		__path = PBField.new("path", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 7, true, __path_default)
		service = PBServiceField.new()
		service.field = __path
		service.func_ref = Callable(self, "add_path")
		data[__path.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __pos: PBField
	func has_pos() -> bool:
		if __pos.value != null:
			return true
		return false
	func get_pos() -> Vec2:
		return __pos.value
	func clear_pos() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__pos.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_pos() -> Vec2:
		__pos.value = Vec2.new()
		return __pos.value
	
	var __target: PBField
	func has_target() -> bool:
		if __target.value != null:
			return true
		return false
	func get_target() -> Vec2:
		return __target.value
	func clear_target() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__target.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_target() -> Vec2:
		__target.value = Vec2.new()
		return __target.value
	
	var __move_speed: PBField
	func has_move_speed() -> bool:
		if __move_speed.value != null:
			return true
		return false
	func get_move_speed() -> int:
		return __move_speed.value
	func clear_move_speed() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__move_speed.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_move_speed(value : int) -> void:
		__move_speed.value = value
	
	var __tick: PBField
	func has_tick() -> bool:
		if __tick.value != null:
			return true
		return false
	func get_tick() -> int:
		return __tick.value
	func clear_tick() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__tick.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_tick(value : int) -> void:
		__tick.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	var __path: PBField
	func get_path() -> Array[Vec2]:
		return __path.value
	func clear_path() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__path.value.clear()
	func add_path() -> Vec2:
		var element = Vec2.new()
		__path.value.append(element)
		return element
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EntityMoves:
	extends RefCounted
	func _init():
		var service
		
		var __moves_default: Array[EntityMove] = []
		__moves = PBField.new("moves", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 1, true, __moves_default)
		service = PBServiceField.new()
		service.field = __moves
		service.func_ref = Callable(self, "add_moves")
		data[__moves.tag] = service
		
	var data = {}
	
	var __moves: PBField
	func get_moves() -> Array[EntityMove]:
		return __moves.value
	func clear_moves() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__moves.value.clear()
	func add_moves() -> EntityMove:
		var element = EntityMove.new()
		__moves.value.append(element)
		return element
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
enum ChatChannel {
	CH_NEARBY = 0,
	CH_TEAM = 1,
	CH_WORLD = 2,
	CH_FACTION = 3,
	CH_SYSTEM = 4,
	CH_CITY = 5,
	CH_TONG = 6,
	CH_WHISPER = 7
}

class ChatReq:
	extends RefCounted
	func _init():
		var service
		
		__text = PBField.new("text", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __text
		data[__text.tag] = service
		
		__channel = PBField.new("channel", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __channel
		data[__channel.tag] = service
		
		__target = PBField.new("target", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __target
		data[__target.tag] = service
		
	var data = {}
	
	var __text: PBField
	func has_text() -> bool:
		if __text.value != null:
			return true
		return false
	func get_text() -> String:
		return __text.value
	func clear_text() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__text.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_text(value : String) -> void:
		__text.value = value
	
	var __channel: PBField
	func has_channel() -> bool:
		if __channel.value != null:
			return true
		return false
	func get_channel():
		return __channel.value
	func clear_channel() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__channel.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_channel(value) -> void:
		__channel.value = value
	
	var __target: PBField
	func has_target() -> bool:
		if __target.value != null:
			return true
		return false
	func get_target() -> String:
		return __target.value
	func clear_target() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__target.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_target(value : String) -> void:
		__target.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ItemView:
	extends RefCounted
	func _init():
		var service
		
		__id = PBField.new("id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __id
		data[__id.tag] = service
		
		__genre = PBField.new("genre", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __genre
		data[__genre.tag] = service
		
		__detail = PBField.new("detail", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __detail
		data[__detail.tag] = service
		
		__particular = PBField.new("particular", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __particular
		data[__particular.tag] = service
		
		__level = PBField.new("level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
		__series = PBField.new("series", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __series
		data[__series.tag] = service
		
		__count = PBField.new("count", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __count
		data[__count.tag] = service
		
		__durability = PBField.new("durability", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __durability
		data[__durability.tag] = service
		
		__max_durability = PBField.new("max_durability", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 9, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __max_durability
		data[__max_durability.tag] = service
		
		__ex_type = PBField.new("ex_type", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 10, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __ex_type
		data[__ex_type.tag] = service
		
		__room = PBField.new("room", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 11, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __room
		data[__room.tag] = service
		
		__x = PBField.new("x", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 12, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __x
		data[__x.tag] = service
		
		__y = PBField.new("y", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 13, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __y
		data[__y.tag] = service
		
		__width = PBField.new("width", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 14, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __width
		data[__width.tag] = service
		
		__height = PBField.new("height", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 15, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __height
		data[__height.tag] = service
		
		__name = PBField.new("name", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 16, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __name
		data[__name.tag] = service
		
		__image = PBField.new("image", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 17, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __image
		data[__image.tag] = service
		
		__intro = PBField.new("intro", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 18, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __intro
		data[__intro.tag] = service
		
		__price = PBField.new("price", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 19, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __price
		data[__price.tag] = service
		
		var __base_default: Array[ItemMagic] = []
		__base = PBField.new("base", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 20, true, __base_default)
		service = PBServiceField.new()
		service.field = __base
		service.func_ref = Callable(self, "add_base")
		data[__base.tag] = service
		
		var __require_default: Array[ItemMagic] = []
		__require = PBField.new("require", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 21, true, __require_default)
		service = PBServiceField.new()
		service.field = __require
		service.func_ref = Callable(self, "add_require")
		data[__require.tag] = service
		
		var __magic_default: Array[ItemMagic] = []
		__magic = PBField.new("magic", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 22, true, __magic_default)
		service = PBServiceField.new()
		service.field = __magic
		service.func_ref = Callable(self, "add_magic")
		data[__magic.tag] = service
		
		__version = PBField.new("version", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 23, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __version
		data[__version.tag] = service
		
		__gen_param = PBField.new("gen_param", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 24, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __gen_param
		data[__gen_param.tag] = service
		
	var data = {}
	
	var __id: PBField
	func has_id() -> bool:
		if __id.value != null:
			return true
		return false
	func get_id() -> int:
		return __id.value
	func clear_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_id(value : int) -> void:
		__id.value = value
	
	var __genre: PBField
	func has_genre() -> bool:
		if __genre.value != null:
			return true
		return false
	func get_genre() -> int:
		return __genre.value
	func clear_genre() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__genre.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_genre(value : int) -> void:
		__genre.value = value
	
	var __detail: PBField
	func has_detail() -> bool:
		if __detail.value != null:
			return true
		return false
	func get_detail() -> int:
		return __detail.value
	func clear_detail() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__detail.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_detail(value : int) -> void:
		__detail.value = value
	
	var __particular: PBField
	func has_particular() -> bool:
		if __particular.value != null:
			return true
		return false
	func get_particular() -> int:
		return __particular.value
	func clear_particular() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__particular.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_particular(value : int) -> void:
		__particular.value = value
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	var __series: PBField
	func has_series() -> bool:
		if __series.value != null:
			return true
		return false
	func get_series() -> int:
		return __series.value
	func clear_series() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__series.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_series(value : int) -> void:
		__series.value = value
	
	var __count: PBField
	func has_count() -> bool:
		if __count.value != null:
			return true
		return false
	func get_count() -> int:
		return __count.value
	func clear_count() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__count.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_count(value : int) -> void:
		__count.value = value
	
	var __durability: PBField
	func has_durability() -> bool:
		if __durability.value != null:
			return true
		return false
	func get_durability() -> int:
		return __durability.value
	func clear_durability() -> void:
		data[8].state = PB_SERVICE_STATE.UNFILLED
		__durability.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_durability(value : int) -> void:
		__durability.value = value
	
	var __max_durability: PBField
	func has_max_durability() -> bool:
		if __max_durability.value != null:
			return true
		return false
	func get_max_durability() -> int:
		return __max_durability.value
	func clear_max_durability() -> void:
		data[9].state = PB_SERVICE_STATE.UNFILLED
		__max_durability.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_max_durability(value : int) -> void:
		__max_durability.value = value
	
	var __ex_type: PBField
	func has_ex_type() -> bool:
		if __ex_type.value != null:
			return true
		return false
	func get_ex_type() -> int:
		return __ex_type.value
	func clear_ex_type() -> void:
		data[10].state = PB_SERVICE_STATE.UNFILLED
		__ex_type.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_ex_type(value : int) -> void:
		__ex_type.value = value
	
	var __room: PBField
	func has_room() -> bool:
		if __room.value != null:
			return true
		return false
	func get_room() -> int:
		return __room.value
	func clear_room() -> void:
		data[11].state = PB_SERVICE_STATE.UNFILLED
		__room.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_room(value : int) -> void:
		__room.value = value
	
	var __x: PBField
	func has_x() -> bool:
		if __x.value != null:
			return true
		return false
	func get_x() -> int:
		return __x.value
	func clear_x() -> void:
		data[12].state = PB_SERVICE_STATE.UNFILLED
		__x.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_x(value : int) -> void:
		__x.value = value
	
	var __y: PBField
	func has_y() -> bool:
		if __y.value != null:
			return true
		return false
	func get_y() -> int:
		return __y.value
	func clear_y() -> void:
		data[13].state = PB_SERVICE_STATE.UNFILLED
		__y.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_y(value : int) -> void:
		__y.value = value
	
	var __width: PBField
	func has_width() -> bool:
		if __width.value != null:
			return true
		return false
	func get_width() -> int:
		return __width.value
	func clear_width() -> void:
		data[14].state = PB_SERVICE_STATE.UNFILLED
		__width.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_width(value : int) -> void:
		__width.value = value
	
	var __height: PBField
	func has_height() -> bool:
		if __height.value != null:
			return true
		return false
	func get_height() -> int:
		return __height.value
	func clear_height() -> void:
		data[15].state = PB_SERVICE_STATE.UNFILLED
		__height.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_height(value : int) -> void:
		__height.value = value
	
	var __name: PBField
	func has_name() -> bool:
		if __name.value != null:
			return true
		return false
	func get_name() -> String:
		return __name.value
	func clear_name() -> void:
		data[16].state = PB_SERVICE_STATE.UNFILLED
		__name.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_name(value : String) -> void:
		__name.value = value
	
	var __image: PBField
	func has_image() -> bool:
		if __image.value != null:
			return true
		return false
	func get_image() -> String:
		return __image.value
	func clear_image() -> void:
		data[17].state = PB_SERVICE_STATE.UNFILLED
		__image.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_image(value : String) -> void:
		__image.value = value
	
	var __intro: PBField
	func has_intro() -> bool:
		if __intro.value != null:
			return true
		return false
	func get_intro() -> String:
		return __intro.value
	func clear_intro() -> void:
		data[18].state = PB_SERVICE_STATE.UNFILLED
		__intro.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_intro(value : String) -> void:
		__intro.value = value
	
	var __price: PBField
	func has_price() -> bool:
		if __price.value != null:
			return true
		return false
	func get_price() -> int:
		return __price.value
	func clear_price() -> void:
		data[19].state = PB_SERVICE_STATE.UNFILLED
		__price.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_price(value : int) -> void:
		__price.value = value
	
	var __base: PBField
	func get_base() -> Array[ItemMagic]:
		return __base.value
	func clear_base() -> void:
		data[20].state = PB_SERVICE_STATE.UNFILLED
		__base.value.clear()
	func add_base() -> ItemMagic:
		var element = ItemMagic.new()
		__base.value.append(element)
		return element
	
	var __require: PBField
	func get_require() -> Array[ItemMagic]:
		return __require.value
	func clear_require() -> void:
		data[21].state = PB_SERVICE_STATE.UNFILLED
		__require.value.clear()
	func add_require() -> ItemMagic:
		var element = ItemMagic.new()
		__require.value.append(element)
		return element
	
	var __magic: PBField
	func get_magic() -> Array[ItemMagic]:
		return __magic.value
	func clear_magic() -> void:
		data[22].state = PB_SERVICE_STATE.UNFILLED
		__magic.value.clear()
	func add_magic() -> ItemMagic:
		var element = ItemMagic.new()
		__magic.value.append(element)
		return element
	
	var __version: PBField
	func has_version() -> bool:
		if __version.value != null:
			return true
		return false
	func get_version() -> int:
		return __version.value
	func clear_version() -> void:
		data[23].state = PB_SERVICE_STATE.UNFILLED
		__version.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_version(value : int) -> void:
		__version.value = value
	
	var __gen_param: PBField
	func has_gen_param() -> bool:
		if __gen_param.value != null:
			return true
		return false
	func get_gen_param() -> int:
		return __gen_param.value
	func clear_gen_param() -> void:
		data[24].state = PB_SERVICE_STATE.UNFILLED
		__gen_param.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_gen_param(value : int) -> void:
		__gen_param.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class InventorySync:
	extends RefCounted
	func _init():
		var service
		
		var __items_default: Array[ItemView] = []
		__items = PBField.new("items", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 1, true, __items_default)
		service = PBServiceField.new()
		service.field = __items
		service.func_ref = Callable(self, "add_items")
		data[__items.tag] = service
		
		__money = PBField.new("money", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __money
		data[__money.tag] = service
		
		__bank_money = PBField.new("bank_money", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __bank_money
		data[__bank_money.tag] = service
		
	var data = {}
	
	var __items: PBField
	func get_items() -> Array[ItemView]:
		return __items.value
	func clear_items() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__items.value.clear()
	func add_items() -> ItemView:
		var element = ItemView.new()
		__items.value.append(element)
		return element
	
	var __money: PBField
	func has_money() -> bool:
		if __money.value != null:
			return true
		return false
	func get_money() -> int:
		return __money.value
	func clear_money() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__money.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_money(value : int) -> void:
		__money.value = value
	
	var __bank_money: PBField
	func has_bank_money() -> bool:
		if __bank_money.value != null:
			return true
		return false
	func get_bank_money() -> int:
		return __bank_money.value
	func clear_bank_money() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__bank_money.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_bank_money(value : int) -> void:
		__bank_money.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ItemAdd:
	extends RefCounted
	func _init():
		var service
		
		__item = PBField.new("item", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __item
		service.func_ref = Callable(self, "new_item")
		data[__item.tag] = service
		
	var data = {}
	
	var __item: PBField
	func has_item() -> bool:
		if __item.value != null:
			return true
		return false
	func get_item() -> ItemView:
		return __item.value
	func clear_item() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__item.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_item() -> ItemView:
		__item.value = ItemView.new()
		return __item.value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ItemRemove:
	extends RefCounted
	func _init():
		var service
		
		__id = PBField.new("id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __id
		data[__id.tag] = service
		
	var data = {}
	
	var __id: PBField
	func has_id() -> bool:
		if __id.value != null:
			return true
		return false
	func get_id() -> int:
		return __id.value
	func clear_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_id(value : int) -> void:
		__id.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ItemMove:
	extends RefCounted
	func _init():
		var service
		
		__id = PBField.new("id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __id
		data[__id.tag] = service
		
		__room = PBField.new("room", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __room
		data[__room.tag] = service
		
		__x = PBField.new("x", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __x
		data[__x.tag] = service
		
		__y = PBField.new("y", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __y
		data[__y.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __id: PBField
	func has_id() -> bool:
		if __id.value != null:
			return true
		return false
	func get_id() -> int:
		return __id.value
	func clear_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_id(value : int) -> void:
		__id.value = value
	
	var __room: PBField
	func has_room() -> bool:
		if __room.value != null:
			return true
		return false
	func get_room() -> int:
		return __room.value
	func clear_room() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__room.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_room(value : int) -> void:
		__room.value = value
	
	var __x: PBField
	func has_x() -> bool:
		if __x.value != null:
			return true
		return false
	func get_x() -> int:
		return __x.value
	func clear_x() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__x.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_x(value : int) -> void:
		__x.value = value
	
	var __y: PBField
	func has_y() -> bool:
		if __y.value != null:
			return true
		return false
	func get_y() -> int:
		return __y.value
	func clear_y() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__y.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_y(value : int) -> void:
		__y.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ItemEquipReq:
	extends RefCounted
	func _init():
		var service
		
		__id = PBField.new("id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __id
		data[__id.tag] = service
		
		__part = PBField.new("part", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __part
		data[__part.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __id: PBField
	func has_id() -> bool:
		if __id.value != null:
			return true
		return false
	func get_id() -> int:
		return __id.value
	func clear_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_id(value : int) -> void:
		__id.value = value
	
	var __part: PBField
	func has_part() -> bool:
		if __part.value != null:
			return true
		return false
	func get_part() -> int:
		return __part.value
	func clear_part() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__part.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_part(value : int) -> void:
		__part.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ItemUnequipReq:
	extends RefCounted
	func _init():
		var service
		
		__part = PBField.new("part", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __part
		data[__part.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __part: PBField
	func has_part() -> bool:
		if __part.value != null:
			return true
		return false
	func get_part() -> int:
		return __part.value
	func clear_part() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__part.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_part(value : int) -> void:
		__part.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ItemUseReq:
	extends RefCounted
	func _init():
		var service
		
		__id = PBField.new("id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __id
		data[__id.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __id: PBField
	func has_id() -> bool:
		if __id.value != null:
			return true
		return false
	func get_id() -> int:
		return __id.value
	func clear_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_id(value : int) -> void:
		__id.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ItemDropReq:
	extends RefCounted
	func _init():
		var service
		
		__id = PBField.new("id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __id
		data[__id.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __id: PBField
	func has_id() -> bool:
		if __id.value != null:
			return true
		return false
	func get_id() -> int:
		return __id.value
	func clear_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_id(value : int) -> void:
		__id.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ItemResult:
	extends RefCounted
	func _init():
		var service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
		__result = PBField.new("result", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __result
		data[__result.tag] = service
		
	var data = {}
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	var __result: PBField
	func has_result() -> bool:
		if __result.value != null:
			return true
		return false
	func get_result():
		return __result.value
	func clear_result() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__result.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_result(value) -> void:
		__result.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class MoneySync:
	extends RefCounted
	func _init():
		var service
		
		__money = PBField.new("money", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __money
		data[__money.tag] = service
		
		__bank_money = PBField.new("bank_money", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __bank_money
		data[__bank_money.tag] = service
		
	var data = {}
	
	var __money: PBField
	func has_money() -> bool:
		if __money.value != null:
			return true
		return false
	func get_money() -> int:
		return __money.value
	func clear_money() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__money.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_money(value : int) -> void:
		__money.value = value
	
	var __bank_money: PBField
	func has_bank_money() -> bool:
		if __bank_money.value != null:
			return true
		return false
	func get_bank_money() -> int:
		return __bank_money.value
	func clear_bank_money() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__bank_money.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_bank_money(value : int) -> void:
		__bank_money.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
enum PlayerAttribute {
	ATTRIB_STRENGTH = 0,
	ATTRIB_DEXTERITY = 1,
	ATTRIB_VITALITY = 2,
	ATTRIB_ENERGY = 3
}

class AddPointReq:
	extends RefCounted
	func _init():
		var service
		
		__attribute = PBField.new("attribute", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __attribute
		data[__attribute.tag] = service
		
		__points = PBField.new("points", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __points
		data[__points.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __attribute: PBField
	func has_attribute() -> bool:
		if __attribute.value != null:
			return true
		return false
	func get_attribute():
		return __attribute.value
	func clear_attribute() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__attribute.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_attribute(value) -> void:
		__attribute.value = value
	
	var __points: PBField
	func has_points() -> bool:
		if __points.value != null:
			return true
		return false
	func get_points() -> int:
		return __points.value
	func clear_points() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__points.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_points(value : int) -> void:
		__points.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class AddSkillPointReq:
	extends RefCounted
	func _init():
		var service
		
		__skill_id = PBField.new("skill_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __skill_id
		data[__skill_id.tag] = service
		
		__points = PBField.new("points", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __points
		data[__points.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __skill_id: PBField
	func has_skill_id() -> bool:
		if __skill_id.value != null:
			return true
		return false
	func get_skill_id() -> int:
		return __skill_id.value
	func clear_skill_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__skill_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_skill_id(value : int) -> void:
		__skill_id.value = value
	
	var __points: PBField
	func has_points() -> bool:
		if __points.value != null:
			return true
		return false
	func get_points() -> int:
		return __points.value
	func clear_points() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__points.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_points(value : int) -> void:
		__points.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class CastSkillReq:
	extends RefCounted
	func _init():
		var service
		
		__skill_id = PBField.new("skill_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __skill_id
		data[__skill_id.tag] = service
		
		__target = PBField.new("target", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __target
		data[__target.tag] = service
		
		__x = PBField.new("x", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __x
		data[__x.tag] = service
		
		__y = PBField.new("y", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __y
		data[__y.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __skill_id: PBField
	func has_skill_id() -> bool:
		if __skill_id.value != null:
			return true
		return false
	func get_skill_id() -> int:
		return __skill_id.value
	func clear_skill_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__skill_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_skill_id(value : int) -> void:
		__skill_id.value = value
	
	var __target: PBField
	func has_target() -> bool:
		if __target.value != null:
			return true
		return false
	func get_target() -> int:
		return __target.value
	func clear_target() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__target.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_target(value : int) -> void:
		__target.value = value
	
	var __x: PBField
	func has_x() -> bool:
		if __x.value != null:
			return true
		return false
	func get_x() -> int:
		return __x.value
	func clear_x() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__x.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_x(value : int) -> void:
		__x.value = value
	
	var __y: PBField
	func has_y() -> bool:
		if __y.value != null:
			return true
		return false
	func get_y() -> int:
		return __y.value
	func clear_y() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__y.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_y(value : int) -> void:
		__y.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SetAuraReq:
	extends RefCounted
	func _init():
		var service
		
		__skill_id = PBField.new("skill_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __skill_id
		data[__skill_id.tag] = service
		
	var data = {}
	
	var __skill_id: PBField
	func has_skill_id() -> bool:
		if __skill_id.value != null:
			return true
		return false
	func get_skill_id() -> int:
		return __skill_id.value
	func clear_skill_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__skill_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_skill_id(value : int) -> void:
		__skill_id.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SkillDescReq:
	extends RefCounted
	func _init():
		var service
		
		__skill_id = PBField.new("skill_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __skill_id
		data[__skill_id.tag] = service
		
		__level = PBField.new("level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
	var data = {}
	
	var __skill_id: PBField
	func has_skill_id() -> bool:
		if __skill_id.value != null:
			return true
		return false
	func get_skill_id() -> int:
		return __skill_id.value
	func clear_skill_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__skill_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_skill_id(value : int) -> void:
		__skill_id.value = value
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ReviveReq:
	extends RefCounted
	func _init():
		var service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class RideReq:
	extends RefCounted
	func _init():
		var service
		
		__on = PBField.new("on", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __on
		data[__on.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __on: PBField
	func has_on() -> bool:
		if __on.value != null:
			return true
		return false
	func get_on() -> bool:
		return __on.value
	func clear_on() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__on.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_on(value : bool) -> void:
		__on.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SitReq:
	extends RefCounted
	func _init():
		var service
		
		__sit = PBField.new("sit", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __sit
		data[__sit.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __sit: PBField
	func has_sit() -> bool:
		if __sit.value != null:
			return true
		return false
	func get_sit() -> bool:
		return __sit.value
	func clear_sit() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__sit.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_sit(value : bool) -> void:
		__sit.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SkillLevelSync:
	extends RefCounted
	func _init():
		var service
		
		__skill_id = PBField.new("skill_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __skill_id
		data[__skill_id.tag] = service
		
		__level = PBField.new("level", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
		__skill_point = PBField.new("skill_point", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __skill_point
		data[__skill_point.tag] = service
		
		__exp_percent = PBField.new("exp_percent", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __exp_percent
		data[__exp_percent.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
		__level_up = PBField.new("level_up", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __level_up
		data[__level_up.tag] = service
		
	var data = {}
	
	var __skill_id: PBField
	func has_skill_id() -> bool:
		if __skill_id.value != null:
			return true
		return false
	func get_skill_id() -> int:
		return __skill_id.value
	func clear_skill_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__skill_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_skill_id(value : int) -> void:
		__skill_id.value = value
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	var __skill_point: PBField
	func has_skill_point() -> bool:
		if __skill_point.value != null:
			return true
		return false
	func get_skill_point() -> int:
		return __skill_point.value
	func clear_skill_point() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__skill_point.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_skill_point(value : int) -> void:
		__skill_point.value = value
	
	var __exp_percent: PBField
	func has_exp_percent() -> bool:
		if __exp_percent.value != null:
			return true
		return false
	func get_exp_percent() -> int:
		return __exp_percent.value
	func clear_exp_percent() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__exp_percent.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_exp_percent(value : int) -> void:
		__exp_percent.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	var __level_up: PBField
	func has_level_up() -> bool:
		if __level_up.value != null:
			return true
		return false
	func get_level_up() -> bool:
		return __level_up.value
	func clear_level_up() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__level_up.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_level_up(value : bool) -> void:
		__level_up.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SkillEntry:
	extends RefCounted
	func _init():
		var service
		
		__skill_id = PBField.new("skill_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __skill_id
		data[__skill_id.tag] = service
		
		__level = PBField.new("level", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
		__current_level = PBField.new("current_level", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __current_level
		data[__current_level.tag] = service
		
		__exp_percent = PBField.new("exp_percent", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __exp_percent
		data[__exp_percent.tag] = service
		
		__max_level = PBField.new("max_level", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __max_level
		data[__max_level.tag] = service
		
		__req_level = PBField.new("req_level", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __req_level
		data[__req_level.tag] = service
		
		__forbidden = PBField.new("forbidden", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __forbidden
		data[__forbidden.tag] = service
		
		__cool_down_left = PBField.new("cool_down_left", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __cool_down_left
		data[__cool_down_left.tag] = service
		
		__only_inc = PBField.new("only_inc", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 9, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __only_inc
		data[__only_inc.tag] = service
		
	var data = {}
	
	var __skill_id: PBField
	func has_skill_id() -> bool:
		if __skill_id.value != null:
			return true
		return false
	func get_skill_id() -> int:
		return __skill_id.value
	func clear_skill_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__skill_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_skill_id(value : int) -> void:
		__skill_id.value = value
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	var __current_level: PBField
	func has_current_level() -> bool:
		if __current_level.value != null:
			return true
		return false
	func get_current_level() -> int:
		return __current_level.value
	func clear_current_level() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__current_level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_current_level(value : int) -> void:
		__current_level.value = value
	
	var __exp_percent: PBField
	func has_exp_percent() -> bool:
		if __exp_percent.value != null:
			return true
		return false
	func get_exp_percent() -> int:
		return __exp_percent.value
	func clear_exp_percent() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__exp_percent.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_exp_percent(value : int) -> void:
		__exp_percent.value = value
	
	var __max_level: PBField
	func has_max_level() -> bool:
		if __max_level.value != null:
			return true
		return false
	func get_max_level() -> int:
		return __max_level.value
	func clear_max_level() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__max_level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_max_level(value : int) -> void:
		__max_level.value = value
	
	var __req_level: PBField
	func has_req_level() -> bool:
		if __req_level.value != null:
			return true
		return false
	func get_req_level() -> int:
		return __req_level.value
	func clear_req_level() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__req_level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_req_level(value : int) -> void:
		__req_level.value = value
	
	var __forbidden: PBField
	func has_forbidden() -> bool:
		if __forbidden.value != null:
			return true
		return false
	func get_forbidden() -> bool:
		return __forbidden.value
	func clear_forbidden() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__forbidden.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_forbidden(value : bool) -> void:
		__forbidden.value = value
	
	var __cool_down_left: PBField
	func has_cool_down_left() -> bool:
		if __cool_down_left.value != null:
			return true
		return false
	func get_cool_down_left() -> int:
		return __cool_down_left.value
	func clear_cool_down_left() -> void:
		data[8].state = PB_SERVICE_STATE.UNFILLED
		__cool_down_left.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_cool_down_left(value : int) -> void:
		__cool_down_left.value = value
	
	var __only_inc: PBField
	func has_only_inc() -> bool:
		if __only_inc.value != null:
			return true
		return false
	func get_only_inc() -> bool:
		return __only_inc.value
	func clear_only_inc() -> void:
		data[9].state = PB_SERVICE_STATE.UNFILLED
		__only_inc.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_only_inc(value : bool) -> void:
		__only_inc.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SkillListSync:
	extends RefCounted
	func _init():
		var service
		
		var __skills_default: Array[SkillEntry] = []
		__skills = PBField.new("skills", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 1, true, __skills_default)
		service = PBServiceField.new()
		service.field = __skills
		service.func_ref = Callable(self, "add_skills")
		data[__skills.tag] = service
		
		__forbid_all = PBField.new("forbid_all", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __forbid_all
		data[__forbid_all.tag] = service
		
	var data = {}
	
	var __skills: PBField
	func get_skills() -> Array[SkillEntry]:
		return __skills.value
	func clear_skills() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__skills.value.clear()
	func add_skills() -> SkillEntry:
		var element = SkillEntry.new()
		__skills.value.append(element)
		return element
	
	var __forbid_all: PBField
	func has_forbid_all() -> bool:
		if __forbid_all.value != null:
			return true
		return false
	func get_forbid_all() -> bool:
		return __forbid_all.value
	func clear_forbid_all() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__forbid_all.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_forbid_all(value : bool) -> void:
		__forbid_all.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SkillForbidSync:
	extends RefCounted
	func _init():
		var service
		
		__skill_id = PBField.new("skill_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __skill_id
		data[__skill_id.tag] = service
		
		__forbid = PBField.new("forbid", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __forbid
		data[__forbid.tag] = service
		
	var data = {}
	
	var __skill_id: PBField
	func has_skill_id() -> bool:
		if __skill_id.value != null:
			return true
		return false
	func get_skill_id() -> int:
		return __skill_id.value
	func clear_skill_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__skill_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_skill_id(value : int) -> void:
		__skill_id.value = value
	
	var __forbid: PBField
	func has_forbid() -> bool:
		if __forbid.value != null:
			return true
		return false
	func get_forbid() -> bool:
		return __forbid.value
	func clear_forbid() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__forbid.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_forbid(value : bool) -> void:
		__forbid.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class PlayerAttribSync:
	extends RefCounted
	func _init():
		var service
		
		__level = PBField.new("level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
		__exp = PBField.new("exp", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __exp
		data[__exp.tag] = service
		
		__next_level_exp = PBField.new("next_level_exp", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __next_level_exp
		data[__next_level_exp.tag] = service
		
		__attribute_point = PBField.new("attribute_point", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __attribute_point
		data[__attribute_point.tag] = service
		
		__skill_point = PBField.new("skill_point", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __skill_point
		data[__skill_point.tag] = service
		
		__strength = PBField.new("strength", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __strength
		data[__strength.tag] = service
		
		__dexterity = PBField.new("dexterity", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __dexterity
		data[__dexterity.tag] = service
		
		__vitality = PBField.new("vitality", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __vitality
		data[__vitality.tag] = service
		
		__energy = PBField.new("energy", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 9, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __energy
		data[__energy.tag] = service
		
		__lucky = PBField.new("lucky", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 10, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __lucky
		data[__lucky.tag] = service
		
		__cur_strength = PBField.new("cur_strength", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 11, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __cur_strength
		data[__cur_strength.tag] = service
		
		__cur_dexterity = PBField.new("cur_dexterity", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 12, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __cur_dexterity
		data[__cur_dexterity.tag] = service
		
		__cur_vitality = PBField.new("cur_vitality", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 13, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __cur_vitality
		data[__cur_vitality.tag] = service
		
		__cur_energy = PBField.new("cur_energy", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 14, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __cur_energy
		data[__cur_energy.tag] = service
		
		__cur_lucky = PBField.new("cur_lucky", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 15, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __cur_lucky
		data[__cur_lucky.tag] = service
		
		__life = PBField.new("life", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 16, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __life
		data[__life.tag] = service
		
		__life_max = PBField.new("life_max", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 17, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __life_max
		data[__life_max.tag] = service
		
		__mana = PBField.new("mana", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 18, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __mana
		data[__mana.tag] = service
		
		__mana_max = PBField.new("mana_max", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 19, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __mana_max
		data[__mana_max.tag] = service
		
		__stamina = PBField.new("stamina", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 20, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __stamina
		data[__stamina.tag] = service
		
		__stamina_max = PBField.new("stamina_max", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 21, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __stamina_max
		data[__stamina_max.tag] = service
		
		__attack_rating = PBField.new("attack_rating", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 22, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __attack_rating
		data[__attack_rating.tag] = service
		
		__defend = PBField.new("defend", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 23, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __defend
		data[__defend.tag] = service
		
		__min_damage = PBField.new("min_damage", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 24, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __min_damage
		data[__min_damage.tag] = service
		
		__max_damage = PBField.new("max_damage", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 25, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __max_damage
		data[__max_damage.tag] = service
		
		__fire_resist = PBField.new("fire_resist", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 26, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __fire_resist
		data[__fire_resist.tag] = service
		
		__cold_resist = PBField.new("cold_resist", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 27, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __cold_resist
		data[__cold_resist.tag] = service
		
		__poison_resist = PBField.new("poison_resist", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 28, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __poison_resist
		data[__poison_resist.tag] = service
		
		__light_resist = PBField.new("light_resist", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 29, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __light_resist
		data[__light_resist.tag] = service
		
		__physics_resist = PBField.new("physics_resist", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 30, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __physics_resist
		data[__physics_resist.tag] = service
		
		__walk_speed = PBField.new("walk_speed", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 31, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __walk_speed
		data[__walk_speed.tag] = service
		
		__run_speed = PBField.new("run_speed", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 32, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __run_speed
		data[__run_speed.tag] = service
		
		__attack_speed = PBField.new("attack_speed", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 33, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __attack_speed
		data[__attack_speed.tag] = service
		
		__cast_speed = PBField.new("cast_speed", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 34, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __cast_speed
		data[__cast_speed.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 35, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
		__faction = PBField.new("faction", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 36, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __faction
		data[__faction.tag] = service
		
		__faction_last = PBField.new("faction_last", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 37, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __faction_last
		data[__faction_last.tag] = service
		
		__level_exp = PBField.new("level_exp", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 38, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __level_exp
		data[__level_exp.tag] = service
		
	var data = {}
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	var __exp: PBField
	func has_exp() -> bool:
		if __exp.value != null:
			return true
		return false
	func get_exp() -> int:
		return __exp.value
	func clear_exp() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__exp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_exp(value : int) -> void:
		__exp.value = value
	
	var __next_level_exp: PBField
	func has_next_level_exp() -> bool:
		if __next_level_exp.value != null:
			return true
		return false
	func get_next_level_exp() -> int:
		return __next_level_exp.value
	func clear_next_level_exp() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__next_level_exp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_next_level_exp(value : int) -> void:
		__next_level_exp.value = value
	
	var __attribute_point: PBField
	func has_attribute_point() -> bool:
		if __attribute_point.value != null:
			return true
		return false
	func get_attribute_point() -> int:
		return __attribute_point.value
	func clear_attribute_point() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__attribute_point.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_attribute_point(value : int) -> void:
		__attribute_point.value = value
	
	var __skill_point: PBField
	func has_skill_point() -> bool:
		if __skill_point.value != null:
			return true
		return false
	func get_skill_point() -> int:
		return __skill_point.value
	func clear_skill_point() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__skill_point.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_skill_point(value : int) -> void:
		__skill_point.value = value
	
	var __strength: PBField
	func has_strength() -> bool:
		if __strength.value != null:
			return true
		return false
	func get_strength() -> int:
		return __strength.value
	func clear_strength() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__strength.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_strength(value : int) -> void:
		__strength.value = value
	
	var __dexterity: PBField
	func has_dexterity() -> bool:
		if __dexterity.value != null:
			return true
		return false
	func get_dexterity() -> int:
		return __dexterity.value
	func clear_dexterity() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__dexterity.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_dexterity(value : int) -> void:
		__dexterity.value = value
	
	var __vitality: PBField
	func has_vitality() -> bool:
		if __vitality.value != null:
			return true
		return false
	func get_vitality() -> int:
		return __vitality.value
	func clear_vitality() -> void:
		data[8].state = PB_SERVICE_STATE.UNFILLED
		__vitality.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_vitality(value : int) -> void:
		__vitality.value = value
	
	var __energy: PBField
	func has_energy() -> bool:
		if __energy.value != null:
			return true
		return false
	func get_energy() -> int:
		return __energy.value
	func clear_energy() -> void:
		data[9].state = PB_SERVICE_STATE.UNFILLED
		__energy.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_energy(value : int) -> void:
		__energy.value = value
	
	var __lucky: PBField
	func has_lucky() -> bool:
		if __lucky.value != null:
			return true
		return false
	func get_lucky() -> int:
		return __lucky.value
	func clear_lucky() -> void:
		data[10].state = PB_SERVICE_STATE.UNFILLED
		__lucky.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_lucky(value : int) -> void:
		__lucky.value = value
	
	var __cur_strength: PBField
	func has_cur_strength() -> bool:
		if __cur_strength.value != null:
			return true
		return false
	func get_cur_strength() -> int:
		return __cur_strength.value
	func clear_cur_strength() -> void:
		data[11].state = PB_SERVICE_STATE.UNFILLED
		__cur_strength.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_cur_strength(value : int) -> void:
		__cur_strength.value = value
	
	var __cur_dexterity: PBField
	func has_cur_dexterity() -> bool:
		if __cur_dexterity.value != null:
			return true
		return false
	func get_cur_dexterity() -> int:
		return __cur_dexterity.value
	func clear_cur_dexterity() -> void:
		data[12].state = PB_SERVICE_STATE.UNFILLED
		__cur_dexterity.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_cur_dexterity(value : int) -> void:
		__cur_dexterity.value = value
	
	var __cur_vitality: PBField
	func has_cur_vitality() -> bool:
		if __cur_vitality.value != null:
			return true
		return false
	func get_cur_vitality() -> int:
		return __cur_vitality.value
	func clear_cur_vitality() -> void:
		data[13].state = PB_SERVICE_STATE.UNFILLED
		__cur_vitality.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_cur_vitality(value : int) -> void:
		__cur_vitality.value = value
	
	var __cur_energy: PBField
	func has_cur_energy() -> bool:
		if __cur_energy.value != null:
			return true
		return false
	func get_cur_energy() -> int:
		return __cur_energy.value
	func clear_cur_energy() -> void:
		data[14].state = PB_SERVICE_STATE.UNFILLED
		__cur_energy.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_cur_energy(value : int) -> void:
		__cur_energy.value = value
	
	var __cur_lucky: PBField
	func has_cur_lucky() -> bool:
		if __cur_lucky.value != null:
			return true
		return false
	func get_cur_lucky() -> int:
		return __cur_lucky.value
	func clear_cur_lucky() -> void:
		data[15].state = PB_SERVICE_STATE.UNFILLED
		__cur_lucky.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_cur_lucky(value : int) -> void:
		__cur_lucky.value = value
	
	var __life: PBField
	func has_life() -> bool:
		if __life.value != null:
			return true
		return false
	func get_life() -> int:
		return __life.value
	func clear_life() -> void:
		data[16].state = PB_SERVICE_STATE.UNFILLED
		__life.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_life(value : int) -> void:
		__life.value = value
	
	var __life_max: PBField
	func has_life_max() -> bool:
		if __life_max.value != null:
			return true
		return false
	func get_life_max() -> int:
		return __life_max.value
	func clear_life_max() -> void:
		data[17].state = PB_SERVICE_STATE.UNFILLED
		__life_max.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_life_max(value : int) -> void:
		__life_max.value = value
	
	var __mana: PBField
	func has_mana() -> bool:
		if __mana.value != null:
			return true
		return false
	func get_mana() -> int:
		return __mana.value
	func clear_mana() -> void:
		data[18].state = PB_SERVICE_STATE.UNFILLED
		__mana.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_mana(value : int) -> void:
		__mana.value = value
	
	var __mana_max: PBField
	func has_mana_max() -> bool:
		if __mana_max.value != null:
			return true
		return false
	func get_mana_max() -> int:
		return __mana_max.value
	func clear_mana_max() -> void:
		data[19].state = PB_SERVICE_STATE.UNFILLED
		__mana_max.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_mana_max(value : int) -> void:
		__mana_max.value = value
	
	var __stamina: PBField
	func has_stamina() -> bool:
		if __stamina.value != null:
			return true
		return false
	func get_stamina() -> int:
		return __stamina.value
	func clear_stamina() -> void:
		data[20].state = PB_SERVICE_STATE.UNFILLED
		__stamina.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_stamina(value : int) -> void:
		__stamina.value = value
	
	var __stamina_max: PBField
	func has_stamina_max() -> bool:
		if __stamina_max.value != null:
			return true
		return false
	func get_stamina_max() -> int:
		return __stamina_max.value
	func clear_stamina_max() -> void:
		data[21].state = PB_SERVICE_STATE.UNFILLED
		__stamina_max.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_stamina_max(value : int) -> void:
		__stamina_max.value = value
	
	var __attack_rating: PBField
	func has_attack_rating() -> bool:
		if __attack_rating.value != null:
			return true
		return false
	func get_attack_rating() -> int:
		return __attack_rating.value
	func clear_attack_rating() -> void:
		data[22].state = PB_SERVICE_STATE.UNFILLED
		__attack_rating.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_attack_rating(value : int) -> void:
		__attack_rating.value = value
	
	var __defend: PBField
	func has_defend() -> bool:
		if __defend.value != null:
			return true
		return false
	func get_defend() -> int:
		return __defend.value
	func clear_defend() -> void:
		data[23].state = PB_SERVICE_STATE.UNFILLED
		__defend.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_defend(value : int) -> void:
		__defend.value = value
	
	var __min_damage: PBField
	func has_min_damage() -> bool:
		if __min_damage.value != null:
			return true
		return false
	func get_min_damage() -> int:
		return __min_damage.value
	func clear_min_damage() -> void:
		data[24].state = PB_SERVICE_STATE.UNFILLED
		__min_damage.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_min_damage(value : int) -> void:
		__min_damage.value = value
	
	var __max_damage: PBField
	func has_max_damage() -> bool:
		if __max_damage.value != null:
			return true
		return false
	func get_max_damage() -> int:
		return __max_damage.value
	func clear_max_damage() -> void:
		data[25].state = PB_SERVICE_STATE.UNFILLED
		__max_damage.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_max_damage(value : int) -> void:
		__max_damage.value = value
	
	var __fire_resist: PBField
	func has_fire_resist() -> bool:
		if __fire_resist.value != null:
			return true
		return false
	func get_fire_resist() -> int:
		return __fire_resist.value
	func clear_fire_resist() -> void:
		data[26].state = PB_SERVICE_STATE.UNFILLED
		__fire_resist.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_fire_resist(value : int) -> void:
		__fire_resist.value = value
	
	var __cold_resist: PBField
	func has_cold_resist() -> bool:
		if __cold_resist.value != null:
			return true
		return false
	func get_cold_resist() -> int:
		return __cold_resist.value
	func clear_cold_resist() -> void:
		data[27].state = PB_SERVICE_STATE.UNFILLED
		__cold_resist.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_cold_resist(value : int) -> void:
		__cold_resist.value = value
	
	var __poison_resist: PBField
	func has_poison_resist() -> bool:
		if __poison_resist.value != null:
			return true
		return false
	func get_poison_resist() -> int:
		return __poison_resist.value
	func clear_poison_resist() -> void:
		data[28].state = PB_SERVICE_STATE.UNFILLED
		__poison_resist.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_poison_resist(value : int) -> void:
		__poison_resist.value = value
	
	var __light_resist: PBField
	func has_light_resist() -> bool:
		if __light_resist.value != null:
			return true
		return false
	func get_light_resist() -> int:
		return __light_resist.value
	func clear_light_resist() -> void:
		data[29].state = PB_SERVICE_STATE.UNFILLED
		__light_resist.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_light_resist(value : int) -> void:
		__light_resist.value = value
	
	var __physics_resist: PBField
	func has_physics_resist() -> bool:
		if __physics_resist.value != null:
			return true
		return false
	func get_physics_resist() -> int:
		return __physics_resist.value
	func clear_physics_resist() -> void:
		data[30].state = PB_SERVICE_STATE.UNFILLED
		__physics_resist.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_physics_resist(value : int) -> void:
		__physics_resist.value = value
	
	var __walk_speed: PBField
	func has_walk_speed() -> bool:
		if __walk_speed.value != null:
			return true
		return false
	func get_walk_speed() -> int:
		return __walk_speed.value
	func clear_walk_speed() -> void:
		data[31].state = PB_SERVICE_STATE.UNFILLED
		__walk_speed.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_walk_speed(value : int) -> void:
		__walk_speed.value = value
	
	var __run_speed: PBField
	func has_run_speed() -> bool:
		if __run_speed.value != null:
			return true
		return false
	func get_run_speed() -> int:
		return __run_speed.value
	func clear_run_speed() -> void:
		data[32].state = PB_SERVICE_STATE.UNFILLED
		__run_speed.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_run_speed(value : int) -> void:
		__run_speed.value = value
	
	var __attack_speed: PBField
	func has_attack_speed() -> bool:
		if __attack_speed.value != null:
			return true
		return false
	func get_attack_speed() -> int:
		return __attack_speed.value
	func clear_attack_speed() -> void:
		data[33].state = PB_SERVICE_STATE.UNFILLED
		__attack_speed.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_attack_speed(value : int) -> void:
		__attack_speed.value = value
	
	var __cast_speed: PBField
	func has_cast_speed() -> bool:
		if __cast_speed.value != null:
			return true
		return false
	func get_cast_speed() -> int:
		return __cast_speed.value
	func clear_cast_speed() -> void:
		data[34].state = PB_SERVICE_STATE.UNFILLED
		__cast_speed.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_cast_speed(value : int) -> void:
		__cast_speed.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[35].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	var __faction: PBField
	func has_faction() -> bool:
		if __faction.value != null:
			return true
		return false
	func get_faction() -> int:
		return __faction.value
	func clear_faction() -> void:
		data[36].state = PB_SERVICE_STATE.UNFILLED
		__faction.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_faction(value : int) -> void:
		__faction.value = value
	
	var __faction_last: PBField
	func has_faction_last() -> bool:
		if __faction_last.value != null:
			return true
		return false
	func get_faction_last() -> int:
		return __faction_last.value
	func clear_faction_last() -> void:
		data[37].state = PB_SERVICE_STATE.UNFILLED
		__faction_last.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_faction_last(value : int) -> void:
		__faction_last.value = value
	
	var __level_exp: PBField
	func has_level_exp() -> bool:
		if __level_exp.value != null:
			return true
		return false
	func get_level_exp() -> int:
		return __level_exp.value
	func clear_level_exp() -> void:
		data[38].state = PB_SERVICE_STATE.UNFILLED
		__level_exp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_level_exp(value : int) -> void:
		__level_exp.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
enum TeamCmd {
	TEAM_CMD_NONE = 0,
	TEAM_INFO = 1,
	TEAM_CREATE = 2,
	TEAM_OPEN_CLOSE = 3,
	TEAM_APPLY_ADD = 4,
	TEAM_ACCEPT = 5,
	TEAM_LEAVE = 6,
	TEAM_KICK = 7,
	TEAM_CHANGE_CAPTAIN = 8,
	TEAM_DISMISS = 9,
	TEAM_INVITE = 10,
	TEAM_REPLY_INVITE = 11
}

class TeamReq:
	extends RefCounted
	func _init():
		var service
		
		__cmd = PBField.new("cmd", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __cmd
		data[__cmd.tag] = service
		
		__target = PBField.new("target", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __target
		data[__target.tag] = service
		
		__flag = PBField.new("flag", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __flag
		data[__flag.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __cmd: PBField
	func has_cmd() -> bool:
		if __cmd.value != null:
			return true
		return false
	func get_cmd():
		return __cmd.value
	func clear_cmd() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__cmd.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_cmd(value) -> void:
		__cmd.value = value
	
	var __target: PBField
	func has_target() -> bool:
		if __target.value != null:
			return true
		return false
	func get_target() -> int:
		return __target.value
	func clear_target() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__target.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_target(value : int) -> void:
		__target.value = value
	
	var __flag: PBField
	func has_flag() -> bool:
		if __flag.value != null:
			return true
		return false
	func get_flag() -> int:
		return __flag.value
	func clear_flag() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__flag.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_flag(value : int) -> void:
		__flag.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class TeamMember:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__name = PBField.new("name", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __name
		data[__name.tag] = service
		
		__level = PBField.new("level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __name: PBField
	func has_name() -> bool:
		if __name.value != null:
			return true
		return false
	func get_name() -> String:
		return __name.value
	func clear_name() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__name.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_name(value : String) -> void:
		__name.value = value
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class TeamSelf:
	extends RefCounted
	func _init():
		var service
		
		__in_team = PBField.new("in_team", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __in_team
		data[__in_team.tag] = service
		
		__team_id = PBField.new("team_id", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __team_id
		data[__team_id.tag] = service
		
		__state = PBField.new("state", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __state
		data[__state.tag] = service
		
		__captain = PBField.new("captain", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __captain
		data[__captain.tag] = service
		
		__leader = PBField.new("leader", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __leader
		service.func_ref = Callable(self, "new_leader")
		data[__leader.tag] = service
		
		var __members_default: Array[TeamMember] = []
		__members = PBField.new("members", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 6, true, __members_default)
		service = PBServiceField.new()
		service.field = __members
		service.func_ref = Callable(self, "add_members")
		data[__members.tag] = service
		
		__lead_level = PBField.new("lead_level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __lead_level
		data[__lead_level.tag] = service
		
		__lead_exp = PBField.new("lead_exp", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __lead_exp
		data[__lead_exp.tag] = service
		
		__members_max = PBField.new("members_max", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 9, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __members_max
		data[__members_max.tag] = service
		
	var data = {}
	
	var __in_team: PBField
	func has_in_team() -> bool:
		if __in_team.value != null:
			return true
		return false
	func get_in_team() -> bool:
		return __in_team.value
	func clear_in_team() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__in_team.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_in_team(value : bool) -> void:
		__in_team.value = value
	
	var __team_id: PBField
	func has_team_id() -> bool:
		if __team_id.value != null:
			return true
		return false
	func get_team_id() -> int:
		return __team_id.value
	func clear_team_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__team_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_team_id(value : int) -> void:
		__team_id.value = value
	
	var __state: PBField
	func has_state() -> bool:
		if __state.value != null:
			return true
		return false
	func get_state() -> int:
		return __state.value
	func clear_state() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__state.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_state(value : int) -> void:
		__state.value = value
	
	var __captain: PBField
	func has_captain() -> bool:
		if __captain.value != null:
			return true
		return false
	func get_captain() -> bool:
		return __captain.value
	func clear_captain() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__captain.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_captain(value : bool) -> void:
		__captain.value = value
	
	var __leader: PBField
	func has_leader() -> bool:
		if __leader.value != null:
			return true
		return false
	func get_leader() -> TeamMember:
		return __leader.value
	func clear_leader() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__leader.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_leader() -> TeamMember:
		__leader.value = TeamMember.new()
		return __leader.value
	
	var __members: PBField
	func get_members() -> Array[TeamMember]:
		return __members.value
	func clear_members() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__members.value.clear()
	func add_members() -> TeamMember:
		var element = TeamMember.new()
		__members.value.append(element)
		return element
	
	var __lead_level: PBField
	func has_lead_level() -> bool:
		if __lead_level.value != null:
			return true
		return false
	func get_lead_level() -> int:
		return __lead_level.value
	func clear_lead_level() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__lead_level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_lead_level(value : int) -> void:
		__lead_level.value = value
	
	var __lead_exp: PBField
	func has_lead_exp() -> bool:
		if __lead_exp.value != null:
			return true
		return false
	func get_lead_exp() -> int:
		return __lead_exp.value
	func clear_lead_exp() -> void:
		data[8].state = PB_SERVICE_STATE.UNFILLED
		__lead_exp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_lead_exp(value : int) -> void:
		__lead_exp.value = value
	
	var __members_max: PBField
	func has_members_max() -> bool:
		if __members_max.value != null:
			return true
		return false
	func get_members_max() -> int:
		return __members_max.value
	func clear_members_max() -> void:
		data[9].state = PB_SERVICE_STATE.UNFILLED
		__members_max.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_members_max(value : int) -> void:
		__members_max.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
enum TeamEventKind {
	TEAM_EV_NONE = 0,
	TEAM_EV_CREATE_OK = 1,
	TEAM_EV_CREATE_FAIL = 2,
	TEAM_EV_ADD_MEMBER = 3,
	TEAM_EV_LEAVE = 4,
	TEAM_EV_KICK = 5,
	TEAM_EV_CHANGE_CAPTAIN = 6,
	TEAM_EV_OPEN_CLOSE = 7,
	TEAM_EV_INVITE = 8,
	TEAM_EV_APPLY = 9,
	TEAM_EV_INFO = 10,
	TEAM_EV_INFO_FALSE = 11,
	TEAM_EV_DISMISS = 12,
	TEAM_EV_REFUSE = 13,
	TEAM_EV_MSG = 14,
	TEAM_EV_SELF_ADD = 15
}

enum TradeCmd {
	TRADE_CMD_NONE = 0,
	TRADE_APPLY_OPEN = 1,
	TRADE_APPLY_CLOSE = 2,
	TRADE_APPLY_START = 3,
	TRADE_REPLY = 4,
	TRADE_MONEY = 5,
	TRADE_DECISION = 6
}

class TradeReq:
	extends RefCounted
	func _init():
		var service
		
		__cmd = PBField.new("cmd", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __cmd
		data[__cmd.tag] = service
		
		__target = PBField.new("target", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __target
		data[__target.tag] = service
		
		__arg = PBField.new("arg", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __arg
		data[__arg.tag] = service
		
		__text = PBField.new("text", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __text
		data[__text.tag] = service
		
		__seq = PBField.new("seq", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __seq
		data[__seq.tag] = service
		
	var data = {}
	
	var __cmd: PBField
	func has_cmd() -> bool:
		if __cmd.value != null:
			return true
		return false
	func get_cmd():
		return __cmd.value
	func clear_cmd() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__cmd.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_cmd(value) -> void:
		__cmd.value = value
	
	var __target: PBField
	func has_target() -> bool:
		if __target.value != null:
			return true
		return false
	func get_target() -> int:
		return __target.value
	func clear_target() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__target.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_target(value : int) -> void:
		__target.value = value
	
	var __arg: PBField
	func has_arg() -> bool:
		if __arg.value != null:
			return true
		return false
	func get_arg() -> int:
		return __arg.value
	func clear_arg() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__arg.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_arg(value : int) -> void:
		__arg.value = value
	
	var __text: PBField
	func has_text() -> bool:
		if __text.value != null:
			return true
		return false
	func get_text() -> String:
		return __text.value
	func clear_text() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__text.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_text(value : String) -> void:
		__text.value = value
	
	var __seq: PBField
	func has_seq() -> bool:
		if __seq.value != null:
			return true
		return false
	func get_seq() -> int:
		return __seq.value
	func clear_seq() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__seq.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_seq(value : int) -> void:
		__seq.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class TradeState:
	extends RefCounted
	func _init():
		var service
		
		__state = PBField.new("state", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __state
		data[__state.tag] = service
		
		__partner = PBField.new("partner", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __partner
		data[__partner.tag] = service
		
		__partner_name = PBField.new("partner_name", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __partner_name
		data[__partner_name.tag] = service
		
	var data = {}
	
	var __state: PBField
	func has_state() -> bool:
		if __state.value != null:
			return true
		return false
	func get_state() -> int:
		return __state.value
	func clear_state() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__state.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_state(value : int) -> void:
		__state.value = value
	
	var __partner: PBField
	func has_partner() -> bool:
		if __partner.value != null:
			return true
		return false
	func get_partner() -> int:
		return __partner.value
	func clear_partner() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__partner.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_partner(value : int) -> void:
		__partner.value = value
	
	var __partner_name: PBField
	func has_partner_name() -> bool:
		if __partner_name.value != null:
			return true
		return false
	func get_partner_name() -> String:
		return __partner_name.value
	func clear_partner_name() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__partner_name.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_partner_name(value : String) -> void:
		__partner_name.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class TradeSync:
	extends RefCounted
	func _init():
		var service
		
		__self_lock = PBField.new("self_lock", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __self_lock
		data[__self_lock.tag] = service
		
		__dest_lock = PBField.new("dest_lock", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __dest_lock
		data[__dest_lock.tag] = service
		
		__self_ok = PBField.new("self_ok", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __self_ok
		data[__self_ok.tag] = service
		
		__dest_ok = PBField.new("dest_ok", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __dest_ok
		data[__dest_ok.tag] = service
		
		__dest_money = PBField.new("dest_money", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __dest_money
		data[__dest_money.tag] = service
		
		__self_money = PBField.new("self_money", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __self_money
		data[__self_money.tag] = service
		
	var data = {}
	
	var __self_lock: PBField
	func has_self_lock() -> bool:
		if __self_lock.value != null:
			return true
		return false
	func get_self_lock() -> bool:
		return __self_lock.value
	func clear_self_lock() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__self_lock.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_self_lock(value : bool) -> void:
		__self_lock.value = value
	
	var __dest_lock: PBField
	func has_dest_lock() -> bool:
		if __dest_lock.value != null:
			return true
		return false
	func get_dest_lock() -> bool:
		return __dest_lock.value
	func clear_dest_lock() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__dest_lock.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_dest_lock(value : bool) -> void:
		__dest_lock.value = value
	
	var __self_ok: PBField
	func has_self_ok() -> bool:
		if __self_ok.value != null:
			return true
		return false
	func get_self_ok() -> bool:
		return __self_ok.value
	func clear_self_ok() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__self_ok.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_self_ok(value : bool) -> void:
		__self_ok.value = value
	
	var __dest_ok: PBField
	func has_dest_ok() -> bool:
		if __dest_ok.value != null:
			return true
		return false
	func get_dest_ok() -> bool:
		return __dest_ok.value
	func clear_dest_ok() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__dest_ok.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_dest_ok(value : bool) -> void:
		__dest_ok.value = value
	
	var __dest_money: PBField
	func has_dest_money() -> bool:
		if __dest_money.value != null:
			return true
		return false
	func get_dest_money() -> int:
		return __dest_money.value
	func clear_dest_money() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__dest_money.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_dest_money(value : int) -> void:
		__dest_money.value = value
	
	var __self_money: PBField
	func has_self_money() -> bool:
		if __self_money.value != null:
			return true
		return false
	func get_self_money() -> int:
		return __self_money.value
	func clear_self_money() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__self_money.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_self_money(value : int) -> void:
		__self_money.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class TradeItem:
	extends RefCounted
	func _init():
		var service
		
		__item = PBField.new("item", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __item
		service.func_ref = Callable(self, "new_item")
		data[__item.tag] = service
		
		__removed = PBField.new("removed", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __removed
		data[__removed.tag] = service
		
	var data = {}
	
	var __item: PBField
	func has_item() -> bool:
		if __item.value != null:
			return true
		return false
	func get_item() -> ItemView:
		return __item.value
	func clear_item() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__item.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_item() -> ItemView:
		__item.value = ItemView.new()
		return __item.value
	
	var __removed: PBField
	func has_removed() -> bool:
		if __removed.value != null:
			return true
		return false
	func get_removed() -> bool:
		return __removed.value
	func clear_removed() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__removed.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_removed(value : bool) -> void:
		__removed.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class TradeApply:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__name = PBField.new("name", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __name
		data[__name.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __name: PBField
	func has_name() -> bool:
		if __name.value != null:
			return true
		return false
	func get_name() -> String:
		return __name.value
	func clear_name() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__name.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_name(value : String) -> void:
		__name.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class TradeEnd:
	extends RefCounted
	func _init():
		var service
		
		__ok = PBField.new("ok", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __ok
		data[__ok.tag] = service
		
	var data = {}
	
	var __ok: PBField
	func has_ok() -> bool:
		if __ok.value != null:
			return true
		return false
	func get_ok() -> bool:
		return __ok.value
	func clear_ok() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__ok.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_ok(value : bool) -> void:
		__ok.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SysMsg:
	extends RefCounted
	func _init():
		var service
		
		__id = PBField.new("id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __id
		data[__id.tag] = service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__name = PBField.new("name", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __name
		data[__name.tag] = service
		
	var data = {}
	
	var __id: PBField
	func has_id() -> bool:
		if __id.value != null:
			return true
		return false
	func get_id() -> int:
		return __id.value
	func clear_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_id(value : int) -> void:
		__id.value = value
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __name: PBField
	func has_name() -> bool:
		if __name.value != null:
			return true
		return false
	func get_name() -> String:
		return __name.value
	func clear_name() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__name.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_name(value : String) -> void:
		__name.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EntityMenuState:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__state = PBField.new("state", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __state
		data[__state.tag] = service
		
		__sentence = PBField.new("sentence", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __sentence
		data[__sentence.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __state: PBField
	func has_state() -> bool:
		if __state.value != null:
			return true
		return false
	func get_state() -> int:
		return __state.value
	func clear_state() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__state.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_state(value : int) -> void:
		__state.value = value
	
	var __sentence: PBField
	func has_sentence() -> bool:
		if __sentence.value != null:
			return true
		return false
	func get_sentence() -> String:
		return __sentence.value
	func clear_sentence() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__sentence.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_sentence(value : String) -> void:
		__sentence.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class TeamEvent:
	extends RefCounted
	func _init():
		var service
		
		__event = PBField.new("event", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __event
		data[__event.tag] = service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__name = PBField.new("name", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __name
		data[__name.tag] = service
		
		__level = PBField.new("level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
		__arg = PBField.new("arg", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __arg
		data[__arg.tag] = service
		
		__leader = PBField.new("leader", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __leader
		service.func_ref = Callable(self, "new_leader")
		data[__leader.tag] = service
		
		var __members_default: Array[TeamMember] = []
		__members = PBField.new("members", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 7, true, __members_default)
		service = PBServiceField.new()
		service.field = __members
		service.func_ref = Callable(self, "add_members")
		data[__members.tag] = service
		
	var data = {}
	
	var __event: PBField
	func has_event() -> bool:
		if __event.value != null:
			return true
		return false
	func get_event():
		return __event.value
	func clear_event() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__event.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_event(value) -> void:
		__event.value = value
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __name: PBField
	func has_name() -> bool:
		if __name.value != null:
			return true
		return false
	func get_name() -> String:
		return __name.value
	func clear_name() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__name.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_name(value : String) -> void:
		__name.value = value
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	var __arg: PBField
	func has_arg() -> bool:
		if __arg.value != null:
			return true
		return false
	func get_arg() -> int:
		return __arg.value
	func clear_arg() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__arg.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_arg(value : int) -> void:
		__arg.value = value
	
	var __leader: PBField
	func has_leader() -> bool:
		if __leader.value != null:
			return true
		return false
	func get_leader() -> TeamMember:
		return __leader.value
	func clear_leader() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__leader.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_leader() -> TeamMember:
		__leader.value = TeamMember.new()
		return __leader.value
	
	var __members: PBField
	func get_members() -> Array[TeamMember]:
		return __members.value
	func clear_members() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__members.value.clear()
	func add_members() -> TeamMember:
		var element = TeamMember.new()
		__members.value.append(element)
		return element
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EntityCamp:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__camp = PBField.new("camp", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __camp
		data[__camp.tag] = service
		
		__current_camp = PBField.new("current_camp", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __current_camp
		data[__current_camp.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __camp: PBField
	func has_camp() -> bool:
		if __camp.value != null:
			return true
		return false
	func get_camp() -> int:
		return __camp.value
	func clear_camp() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__camp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_camp(value : int) -> void:
		__camp.value = value
	
	var __current_camp: PBField
	func has_current_camp() -> bool:
		if __current_camp.value != null:
			return true
		return false
	func get_current_camp() -> int:
		return __current_camp.value
	func clear_current_camp() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__current_camp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_current_camp(value : int) -> void:
		__current_camp.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class PlayerFaction:
	extends RefCounted
	func _init():
		var service
		
		__camp = PBField.new("camp", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __camp
		data[__camp.tag] = service
		
		__faction = PBField.new("faction", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __faction
		data[__faction.tag] = service
		
		__faction_last = PBField.new("faction_last", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __faction_last
		data[__faction_last.tag] = service
		
		__faction_count = PBField.new("faction_count", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __faction_count
		data[__faction_count.tag] = service
		
	var data = {}
	
	var __camp: PBField
	func has_camp() -> bool:
		if __camp.value != null:
			return true
		return false
	func get_camp() -> int:
		return __camp.value
	func clear_camp() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__camp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_camp(value : int) -> void:
		__camp.value = value
	
	var __faction: PBField
	func has_faction() -> bool:
		if __faction.value != null:
			return true
		return false
	func get_faction() -> int:
		return __faction.value
	func clear_faction() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__faction.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_faction(value : int) -> void:
		__faction.value = value
	
	var __faction_last: PBField
	func has_faction_last() -> bool:
		if __faction_last.value != null:
			return true
		return false
	func get_faction_last() -> int:
		return __faction_last.value
	func clear_faction_last() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__faction_last.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_faction_last(value : int) -> void:
		__faction_last.value = value
	
	var __faction_count: PBField
	func has_faction_count() -> bool:
		if __faction_count.value != null:
			return true
		return false
	func get_faction_count() -> int:
		return __faction_count.value
	func clear_faction_count() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__faction_count.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_faction_count(value : int) -> void:
		__faction_count.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class StateAttrib:
	extends RefCounted
	func _init():
		var service
		
		__type = PBField.new("type", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __type
		data[__type.tag] = service
		
		__v0 = PBField.new("v0", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __v0
		data[__v0.tag] = service
		
		__v1 = PBField.new("v1", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __v1
		data[__v1.tag] = service
		
		__v2 = PBField.new("v2", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __v2
		data[__v2.tag] = service
		
	var data = {}
	
	var __type: PBField
	func has_type() -> bool:
		if __type.value != null:
			return true
		return false
	func get_type() -> int:
		return __type.value
	func clear_type() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__type.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_type(value : int) -> void:
		__type.value = value
	
	var __v0: PBField
	func has_v0() -> bool:
		if __v0.value != null:
			return true
		return false
	func get_v0() -> int:
		return __v0.value
	func clear_v0() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__v0.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_v0(value : int) -> void:
		__v0.value = value
	
	var __v1: PBField
	func has_v1() -> bool:
		if __v1.value != null:
			return true
		return false
	func get_v1() -> int:
		return __v1.value
	func clear_v1() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__v1.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_v1(value : int) -> void:
		__v1.value = value
	
	var __v2: PBField
	func has_v2() -> bool:
		if __v2.value != null:
			return true
		return false
	func get_v2() -> int:
		return __v2.value
	func clear_v2() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__v2.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_v2(value : int) -> void:
		__v2.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EntityState:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__skill_id = PBField.new("skill_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __skill_id
		data[__skill_id.tag] = service
		
		__level = PBField.new("level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
		__time = PBField.new("time", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __time
		data[__time.tag] = service
		
		__special_id = PBField.new("special_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __special_id
		data[__special_id.tag] = service
		
		__removed = PBField.new("removed", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __removed
		data[__removed.tag] = service
		
		var __states_default: Array[StateAttrib] = []
		__states = PBField.new("states", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 7, true, __states_default)
		service = PBServiceField.new()
		service.field = __states
		service.func_ref = Callable(self, "add_states")
		data[__states.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __skill_id: PBField
	func has_skill_id() -> bool:
		if __skill_id.value != null:
			return true
		return false
	func get_skill_id() -> int:
		return __skill_id.value
	func clear_skill_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__skill_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_skill_id(value : int) -> void:
		__skill_id.value = value
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	var __time: PBField
	func has_time() -> bool:
		if __time.value != null:
			return true
		return false
	func get_time() -> int:
		return __time.value
	func clear_time() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__time.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_time(value : int) -> void:
		__time.value = value
	
	var __special_id: PBField
	func has_special_id() -> bool:
		if __special_id.value != null:
			return true
		return false
	func get_special_id() -> int:
		return __special_id.value
	func clear_special_id() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__special_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_special_id(value : int) -> void:
		__special_id.value = value
	
	var __removed: PBField
	func has_removed() -> bool:
		if __removed.value != null:
			return true
		return false
	func get_removed() -> bool:
		return __removed.value
	func clear_removed() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__removed.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_removed(value : bool) -> void:
		__removed.value = value
	
	var __states: PBField
	func get_states() -> Array[StateAttrib]:
		return __states.value
	func clear_states() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__states.value.clear()
	func add_states() -> StateAttrib:
		var element = StateAttrib.new()
		__states.value.append(element)
		return element
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SkillDescAttrib:
	extends RefCounted
	func _init():
		var service
		
		__group = PBField.new("group", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __group
		data[__group.tag] = service
		
		__name = PBField.new("name", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __name
		data[__name.tag] = service
		
		__v0 = PBField.new("v0", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __v0
		data[__v0.tag] = service
		
		__v1 = PBField.new("v1", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __v1
		data[__v1.tag] = service
		
		__v2 = PBField.new("v2", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __v2
		data[__v2.tag] = service
		
	var data = {}
	
	var __group: PBField
	func has_group() -> bool:
		if __group.value != null:
			return true
		return false
	func get_group() -> int:
		return __group.value
	func clear_group() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__group.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_group(value : int) -> void:
		__group.value = value
	
	var __name: PBField
	func has_name() -> bool:
		if __name.value != null:
			return true
		return false
	func get_name() -> String:
		return __name.value
	func clear_name() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__name.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_name(value : String) -> void:
		__name.value = value
	
	var __v0: PBField
	func has_v0() -> bool:
		if __v0.value != null:
			return true
		return false
	func get_v0() -> int:
		return __v0.value
	func clear_v0() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__v0.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_v0(value : int) -> void:
		__v0.value = value
	
	var __v1: PBField
	func has_v1() -> bool:
		if __v1.value != null:
			return true
		return false
	func get_v1() -> int:
		return __v1.value
	func clear_v1() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__v1.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_v1(value : int) -> void:
		__v1.value = value
	
	var __v2: PBField
	func has_v2() -> bool:
		if __v2.value != null:
			return true
		return false
	func get_v2() -> int:
		return __v2.value
	func clear_v2() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__v2.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_v2(value : int) -> void:
		__v2.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SkillDescAppend:
	extends RefCounted
	func _init():
		var service
		
		__skill_id = PBField.new("skill_id", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __skill_id
		data[__skill_id.tag] = service
		
		__value = PBField.new("value", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __value
		data[__value.tag] = service
		
	var data = {}
	
	var __skill_id: PBField
	func has_skill_id() -> bool:
		if __skill_id.value != null:
			return true
		return false
	func get_skill_id() -> int:
		return __skill_id.value
	func clear_skill_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__skill_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_skill_id(value : int) -> void:
		__skill_id.value = value
	
	var __value: PBField
	func has_value() -> bool:
		if __value.value != null:
			return true
		return false
	func get_value() -> int:
		return __value.value
	func clear_value() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__value.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_value(value : int) -> void:
		__value.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SkillDescRelated:
	extends RefCounted
	func _init():
		var service
		
		__skill_id = PBField.new("skill_id", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __skill_id
		data[__skill_id.tag] = service
		
		__level = PBField.new("level", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
		__flags = PBField.new("flags", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __flags
		data[__flags.tag] = service
		
		var __attribs_default: Array[SkillDescAttrib] = []
		__attribs = PBField.new("attribs", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 4, true, __attribs_default)
		service = PBServiceField.new()
		service.field = __attribs
		service.func_ref = Callable(self, "add_attribs")
		data[__attribs.tag] = service
		
	var data = {}
	
	var __skill_id: PBField
	func has_skill_id() -> bool:
		if __skill_id.value != null:
			return true
		return false
	func get_skill_id() -> int:
		return __skill_id.value
	func clear_skill_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__skill_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_skill_id(value : int) -> void:
		__skill_id.value = value
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	var __flags: PBField
	func has_flags() -> bool:
		if __flags.value != null:
			return true
		return false
	func get_flags() -> int:
		return __flags.value
	func clear_flags() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__flags.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_flags(value : int) -> void:
		__flags.value = value
	
	var __attribs: PBField
	func get_attribs() -> Array[SkillDescAttrib]:
		return __attribs.value
	func clear_attribs() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__attribs.value.clear()
	func add_attribs() -> SkillDescAttrib:
		var element = SkillDescAttrib.new()
		__attribs.value.append(element)
		return element
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SkillDescLevel:
	extends RefCounted
	func _init():
		var service
		
		__level = PBField.new("level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
		__cost = PBField.new("cost", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __cost
		data[__cost.tag] = service
		
		__cost_type = PBField.new("cost_type", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __cost_type
		data[__cost_type.tag] = service
		
		__attack_radius = PBField.new("attack_radius", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __attack_radius
		data[__attack_radius.tag] = service
		
		var __attribs_default: Array[SkillDescAttrib] = []
		__attribs = PBField.new("attribs", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 5, true, __attribs_default)
		service = PBServiceField.new()
		service.field = __attribs
		service.func_ref = Callable(self, "add_attribs")
		data[__attribs.tag] = service
		
		var __appends_default: Array[SkillDescAppend] = []
		__appends = PBField.new("appends", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 6, true, __appends_default)
		service = PBServiceField.new()
		service.field = __appends
		service.func_ref = Callable(self, "add_appends")
		data[__appends.tag] = service
		
		var __related_default: Array[SkillDescRelated] = []
		__related = PBField.new("related", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 7, true, __related_default)
		service = PBServiceField.new()
		service.field = __related
		service.func_ref = Callable(self, "add_related")
		data[__related.tag] = service
		
	var data = {}
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	var __cost: PBField
	func has_cost() -> bool:
		if __cost.value != null:
			return true
		return false
	func get_cost() -> int:
		return __cost.value
	func clear_cost() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__cost.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_cost(value : int) -> void:
		__cost.value = value
	
	var __cost_type: PBField
	func has_cost_type() -> bool:
		if __cost_type.value != null:
			return true
		return false
	func get_cost_type() -> int:
		return __cost_type.value
	func clear_cost_type() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__cost_type.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_cost_type(value : int) -> void:
		__cost_type.value = value
	
	var __attack_radius: PBField
	func has_attack_radius() -> bool:
		if __attack_radius.value != null:
			return true
		return false
	func get_attack_radius() -> int:
		return __attack_radius.value
	func clear_attack_radius() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__attack_radius.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_attack_radius(value : int) -> void:
		__attack_radius.value = value
	
	var __attribs: PBField
	func get_attribs() -> Array[SkillDescAttrib]:
		return __attribs.value
	func clear_attribs() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__attribs.value.clear()
	func add_attribs() -> SkillDescAttrib:
		var element = SkillDescAttrib.new()
		__attribs.value.append(element)
		return element
	
	var __appends: PBField
	func get_appends() -> Array[SkillDescAppend]:
		return __appends.value
	func clear_appends() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__appends.value.clear()
	func add_appends() -> SkillDescAppend:
		var element = SkillDescAppend.new()
		__appends.value.append(element)
		return element
	
	var __related: PBField
	func get_related() -> Array[SkillDescRelated]:
		return __related.value
	func clear_related() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__related.value.clear()
	func add_related() -> SkillDescRelated:
		var element = SkillDescRelated.new()
		__related.value.append(element)
		return element
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class EntityStateIcons:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		var __icons_default: Array[int] = []
		__icons = PBField.new("icons", PB_DATA_TYPE.UINT32, PB_RULE.REPEATED, 2, true, __icons_default)
		service = PBServiceField.new()
		service.field = __icons
		data[__icons.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __icons: PBField
	func get_icons() -> Array[int]:
		return __icons.value
	func clear_icons() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__icons.value.clear()
	func add_icons(value : int) -> void:
		__icons.value.append(value)
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class MissleSync:
	extends RefCounted
	func _init():
		var service
		
		__index = PBField.new("index", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __index
		data[__index.tag] = service
		
		__missle_id = PBField.new("missle_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __missle_id
		data[__missle_id.tag] = service
		
		__skill_id = PBField.new("skill_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __skill_id
		data[__skill_id.tag] = service
		
		__level = PBField.new("level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
		__launcher = PBField.new("launcher", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __launcher
		data[__launcher.tag] = service
		
		__x = PBField.new("x", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __x
		data[__x.tag] = service
		
		__y = PBField.new("y", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __y
		data[__y.tag] = service
		
		__z = PBField.new("z", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __z
		data[__z.tag] = service
		
		__dir = PBField.new("dir", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 9, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __dir
		data[__dir.tag] = service
		
		__x_factor = PBField.new("x_factor", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 10, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __x_factor
		data[__x_factor.tag] = service
		
		__y_factor = PBField.new("y_factor", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 11, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __y_factor
		data[__y_factor.tag] = service
		
		__speed = PBField.new("speed", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 12, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __speed
		data[__speed.tag] = service
		
		__life_time = PBField.new("life_time", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 13, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __life_time
		data[__life_time.tag] = service
		
		__start_life_time = PBField.new("start_life_time", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 14, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __start_life_time
		data[__start_life_time.tag] = service
		
		__current_life = PBField.new("current_life", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 15, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __current_life
		data[__current_life.tag] = service
		
		__status = PBField.new("status", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 16, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __status
		data[__status.tag] = service
		
		__removed = PBField.new("removed", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 17, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __removed
		data[__removed.tag] = service
		
		__move_kind = PBField.new("move_kind", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 18, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __move_kind
		data[__move_kind.tag] = service
		
		__collided = PBField.new("collided", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 19, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __collided
		data[__collided.tag] = service
		
		__height = PBField.new("height", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 20, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __height
		data[__height.tag] = service
		
		__height_speed = PBField.new("height_speed", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 21, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __height_speed
		data[__height_speed.tag] = service
		
		__z_acceleration = PBField.new("z_acceleration", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 22, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __z_acceleration
		data[__z_acceleration.tag] = service
		
	var data = {}
	
	var __index: PBField
	func has_index() -> bool:
		if __index.value != null:
			return true
		return false
	func get_index() -> int:
		return __index.value
	func clear_index() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__index.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_index(value : int) -> void:
		__index.value = value
	
	var __missle_id: PBField
	func has_missle_id() -> bool:
		if __missle_id.value != null:
			return true
		return false
	func get_missle_id() -> int:
		return __missle_id.value
	func clear_missle_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__missle_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_missle_id(value : int) -> void:
		__missle_id.value = value
	
	var __skill_id: PBField
	func has_skill_id() -> bool:
		if __skill_id.value != null:
			return true
		return false
	func get_skill_id() -> int:
		return __skill_id.value
	func clear_skill_id() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__skill_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_skill_id(value : int) -> void:
		__skill_id.value = value
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	var __launcher: PBField
	func has_launcher() -> bool:
		if __launcher.value != null:
			return true
		return false
	func get_launcher() -> int:
		return __launcher.value
	func clear_launcher() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__launcher.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_launcher(value : int) -> void:
		__launcher.value = value
	
	var __x: PBField
	func has_x() -> bool:
		if __x.value != null:
			return true
		return false
	func get_x() -> int:
		return __x.value
	func clear_x() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__x.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_x(value : int) -> void:
		__x.value = value
	
	var __y: PBField
	func has_y() -> bool:
		if __y.value != null:
			return true
		return false
	func get_y() -> int:
		return __y.value
	func clear_y() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__y.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_y(value : int) -> void:
		__y.value = value
	
	var __z: PBField
	func has_z() -> bool:
		if __z.value != null:
			return true
		return false
	func get_z() -> int:
		return __z.value
	func clear_z() -> void:
		data[8].state = PB_SERVICE_STATE.UNFILLED
		__z.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_z(value : int) -> void:
		__z.value = value
	
	var __dir: PBField
	func has_dir() -> bool:
		if __dir.value != null:
			return true
		return false
	func get_dir() -> int:
		return __dir.value
	func clear_dir() -> void:
		data[9].state = PB_SERVICE_STATE.UNFILLED
		__dir.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_dir(value : int) -> void:
		__dir.value = value
	
	var __x_factor: PBField
	func has_x_factor() -> bool:
		if __x_factor.value != null:
			return true
		return false
	func get_x_factor() -> int:
		return __x_factor.value
	func clear_x_factor() -> void:
		data[10].state = PB_SERVICE_STATE.UNFILLED
		__x_factor.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_x_factor(value : int) -> void:
		__x_factor.value = value
	
	var __y_factor: PBField
	func has_y_factor() -> bool:
		if __y_factor.value != null:
			return true
		return false
	func get_y_factor() -> int:
		return __y_factor.value
	func clear_y_factor() -> void:
		data[11].state = PB_SERVICE_STATE.UNFILLED
		__y_factor.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_y_factor(value : int) -> void:
		__y_factor.value = value
	
	var __speed: PBField
	func has_speed() -> bool:
		if __speed.value != null:
			return true
		return false
	func get_speed() -> int:
		return __speed.value
	func clear_speed() -> void:
		data[12].state = PB_SERVICE_STATE.UNFILLED
		__speed.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_speed(value : int) -> void:
		__speed.value = value
	
	var __life_time: PBField
	func has_life_time() -> bool:
		if __life_time.value != null:
			return true
		return false
	func get_life_time() -> int:
		return __life_time.value
	func clear_life_time() -> void:
		data[13].state = PB_SERVICE_STATE.UNFILLED
		__life_time.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_life_time(value : int) -> void:
		__life_time.value = value
	
	var __start_life_time: PBField
	func has_start_life_time() -> bool:
		if __start_life_time.value != null:
			return true
		return false
	func get_start_life_time() -> int:
		return __start_life_time.value
	func clear_start_life_time() -> void:
		data[14].state = PB_SERVICE_STATE.UNFILLED
		__start_life_time.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_start_life_time(value : int) -> void:
		__start_life_time.value = value
	
	var __current_life: PBField
	func has_current_life() -> bool:
		if __current_life.value != null:
			return true
		return false
	func get_current_life() -> int:
		return __current_life.value
	func clear_current_life() -> void:
		data[15].state = PB_SERVICE_STATE.UNFILLED
		__current_life.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_current_life(value : int) -> void:
		__current_life.value = value
	
	var __status: PBField
	func has_status() -> bool:
		if __status.value != null:
			return true
		return false
	func get_status() -> int:
		return __status.value
	func clear_status() -> void:
		data[16].state = PB_SERVICE_STATE.UNFILLED
		__status.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_status(value : int) -> void:
		__status.value = value
	
	var __removed: PBField
	func has_removed() -> bool:
		if __removed.value != null:
			return true
		return false
	func get_removed() -> bool:
		return __removed.value
	func clear_removed() -> void:
		data[17].state = PB_SERVICE_STATE.UNFILLED
		__removed.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_removed(value : bool) -> void:
		__removed.value = value
	
	var __move_kind: PBField
	func has_move_kind() -> bool:
		if __move_kind.value != null:
			return true
		return false
	func get_move_kind() -> int:
		return __move_kind.value
	func clear_move_kind() -> void:
		data[18].state = PB_SERVICE_STATE.UNFILLED
		__move_kind.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_move_kind(value : int) -> void:
		__move_kind.value = value
	
	var __collided: PBField
	func has_collided() -> bool:
		if __collided.value != null:
			return true
		return false
	func get_collided() -> bool:
		return __collided.value
	func clear_collided() -> void:
		data[19].state = PB_SERVICE_STATE.UNFILLED
		__collided.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_collided(value : bool) -> void:
		__collided.value = value
	
	var __height: PBField
	func has_height() -> bool:
		if __height.value != null:
			return true
		return false
	func get_height() -> int:
		return __height.value
	func clear_height() -> void:
		data[20].state = PB_SERVICE_STATE.UNFILLED
		__height.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_height(value : int) -> void:
		__height.value = value
	
	var __height_speed: PBField
	func has_height_speed() -> bool:
		if __height_speed.value != null:
			return true
		return false
	func get_height_speed() -> int:
		return __height_speed.value
	func clear_height_speed() -> void:
		data[21].state = PB_SERVICE_STATE.UNFILLED
		__height_speed.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_height_speed(value : int) -> void:
		__height_speed.value = value
	
	var __z_acceleration: PBField
	func has_z_acceleration() -> bool:
		if __z_acceleration.value != null:
			return true
		return false
	func get_z_acceleration() -> int:
		return __z_acceleration.value
	func clear_z_acceleration() -> void:
		data[22].state = PB_SERVICE_STATE.UNFILLED
		__z_acceleration.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_z_acceleration(value : int) -> void:
		__z_acceleration.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SkillDesc:
	extends RefCounted
	func _init():
		var service
		
		__skill_id = PBField.new("skill_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __skill_id
		data[__skill_id.tag] = service
		
		__with_cur = PBField.new("with_cur", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __with_cur
		data[__with_cur.tag] = service
		
		__cur = PBField.new("cur", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __cur
		service.func_ref = Callable(self, "new_cur")
		data[__cur.tag] = service
		
		__with_next = PBField.new("with_next", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __with_next
		data[__with_next.tag] = service
		
		__next = PBField.new("next", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __next
		service.func_ref = Callable(self, "new_next")
		data[__next.tag] = service
		
		__max_level = PBField.new("max_level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __max_level
		data[__max_level.tag] = service
		
		__level_inc = PBField.new("level_inc", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __level_inc
		data[__level_inc.tag] = service
		
		__enhance = PBField.new("enhance", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __enhance
		data[__enhance.tag] = service
		
		__held_level = PBField.new("held_level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 9, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __held_level
		data[__held_level.tag] = service
		
		__equip_percent = PBField.new("equip_percent", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 10, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __equip_percent
		data[__equip_percent.tag] = service
		
		__with_modifier = PBField.new("with_modifier", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 11, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __with_modifier
		data[__with_modifier.tag] = service
		
		__modifier = PBField.new("modifier", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 12, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __modifier
		service.func_ref = Callable(self, "new_modifier")
		data[__modifier.tag] = service
		
	var data = {}
	
	var __skill_id: PBField
	func has_skill_id() -> bool:
		if __skill_id.value != null:
			return true
		return false
	func get_skill_id() -> int:
		return __skill_id.value
	func clear_skill_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__skill_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_skill_id(value : int) -> void:
		__skill_id.value = value
	
	var __with_cur: PBField
	func has_with_cur() -> bool:
		if __with_cur.value != null:
			return true
		return false
	func get_with_cur() -> bool:
		return __with_cur.value
	func clear_with_cur() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__with_cur.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_with_cur(value : bool) -> void:
		__with_cur.value = value
	
	var __cur: PBField
	func has_cur() -> bool:
		if __cur.value != null:
			return true
		return false
	func get_cur() -> SkillDescLevel:
		return __cur.value
	func clear_cur() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__cur.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_cur() -> SkillDescLevel:
		__cur.value = SkillDescLevel.new()
		return __cur.value
	
	var __with_next: PBField
	func has_with_next() -> bool:
		if __with_next.value != null:
			return true
		return false
	func get_with_next() -> bool:
		return __with_next.value
	func clear_with_next() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__with_next.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_with_next(value : bool) -> void:
		__with_next.value = value
	
	var __next: PBField
	func has_next() -> bool:
		if __next.value != null:
			return true
		return false
	func get_next() -> SkillDescLevel:
		return __next.value
	func clear_next() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__next.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_next() -> SkillDescLevel:
		__next.value = SkillDescLevel.new()
		return __next.value
	
	var __max_level: PBField
	func has_max_level() -> bool:
		if __max_level.value != null:
			return true
		return false
	func get_max_level() -> int:
		return __max_level.value
	func clear_max_level() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__max_level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_max_level(value : int) -> void:
		__max_level.value = value
	
	var __level_inc: PBField
	func has_level_inc() -> bool:
		if __level_inc.value != null:
			return true
		return false
	func get_level_inc() -> int:
		return __level_inc.value
	func clear_level_inc() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__level_inc.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_level_inc(value : int) -> void:
		__level_inc.value = value
	
	var __enhance: PBField
	func has_enhance() -> bool:
		if __enhance.value != null:
			return true
		return false
	func get_enhance() -> int:
		return __enhance.value
	func clear_enhance() -> void:
		data[8].state = PB_SERVICE_STATE.UNFILLED
		__enhance.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_enhance(value : int) -> void:
		__enhance.value = value
	
	var __held_level: PBField
	func has_held_level() -> bool:
		if __held_level.value != null:
			return true
		return false
	func get_held_level() -> int:
		return __held_level.value
	func clear_held_level() -> void:
		data[9].state = PB_SERVICE_STATE.UNFILLED
		__held_level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_held_level(value : int) -> void:
		__held_level.value = value
	
	var __equip_percent: PBField
	func has_equip_percent() -> bool:
		if __equip_percent.value != null:
			return true
		return false
	func get_equip_percent() -> int:
		return __equip_percent.value
	func clear_equip_percent() -> void:
		data[10].state = PB_SERVICE_STATE.UNFILLED
		__equip_percent.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_equip_percent(value : int) -> void:
		__equip_percent.value = value
	
	var __with_modifier: PBField
	func has_with_modifier() -> bool:
		if __with_modifier.value != null:
			return true
		return false
	func get_with_modifier() -> bool:
		return __with_modifier.value
	func clear_with_modifier() -> void:
		data[11].state = PB_SERVICE_STATE.UNFILLED
		__with_modifier.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_with_modifier(value : bool) -> void:
		__with_modifier.value = value
	
	var __modifier: PBField
	func has_modifier() -> bool:
		if __modifier.value != null:
			return true
		return false
	func get_modifier() -> SkillDescAttrib:
		return __modifier.value
	func clear_modifier() -> void:
		data[12].state = PB_SERVICE_STATE.UNFILLED
		__modifier.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_modifier() -> SkillDescAttrib:
		__modifier.value = SkillDescAttrib.new()
		return __modifier.value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ChangeMap:
	extends RefCounted
	func _init():
		var service
		
		__map_id = PBField.new("map_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __map_id
		data[__map_id.tag] = service
		
		__pos = PBField.new("pos", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __pos
		service.func_ref = Callable(self, "new_pos")
		data[__pos.tag] = service
		
		__scene_w = PBField.new("scene_w", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __scene_w
		data[__scene_w.tag] = service
		
		__scene_h = PBField.new("scene_h", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __scene_h
		data[__scene_h.tag] = service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
	var data = {}
	
	var __map_id: PBField
	func has_map_id() -> bool:
		if __map_id.value != null:
			return true
		return false
	func get_map_id() -> int:
		return __map_id.value
	func clear_map_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__map_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_map_id(value : int) -> void:
		__map_id.value = value
	
	var __pos: PBField
	func has_pos() -> bool:
		if __pos.value != null:
			return true
		return false
	func get_pos() -> Vec2:
		return __pos.value
	func clear_pos() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__pos.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_pos() -> Vec2:
		__pos.value = Vec2.new()
		return __pos.value
	
	var __scene_w: PBField
	func has_scene_w() -> bool:
		if __scene_w.value != null:
			return true
		return false
	func get_scene_w() -> int:
		return __scene_w.value
	func clear_scene_w() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__scene_w.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_scene_w(value : int) -> void:
		__scene_w.value = value
	
	var __scene_h: PBField
	func has_scene_h() -> bool:
		if __scene_h.value != null:
			return true
		return false
	func get_scene_h() -> int:
		return __scene_h.value
	func clear_scene_h() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__scene_h.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_scene_h(value : int) -> void:
		__scene_h.value = value
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ChatMsg:
	extends RefCounted
	func _init():
		var service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__name = PBField.new("name", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __name
		data[__name.tag] = service
		
		__text = PBField.new("text", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __text
		data[__text.tag] = service
		
		__channel = PBField.new("channel", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __channel
		data[__channel.tag] = service
		
	var data = {}
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __name: PBField
	func has_name() -> bool:
		if __name.value != null:
			return true
		return false
	func get_name() -> String:
		return __name.value
	func clear_name() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__name.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_name(value : String) -> void:
		__name.value = value
	
	var __text: PBField
	func has_text() -> bool:
		if __text.value != null:
			return true
		return false
	func get_text() -> String:
		return __text.value
	func clear_text() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__text.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_text(value : String) -> void:
		__text.value = value
	
	var __channel: PBField
	func has_channel() -> bool:
		if __channel.value != null:
			return true
		return false
	func get_channel():
		return __channel.value
	func clear_channel() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__channel.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_channel(value) -> void:
		__channel.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class Ping:
	extends RefCounted
	func _init():
		var service
		
		__client_ms = PBField.new("client_ms", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __client_ms
		data[__client_ms.tag] = service
		
	var data = {}
	
	var __client_ms: PBField
	func has_client_ms() -> bool:
		if __client_ms.value != null:
			return true
		return false
	func get_client_ms() -> int:
		return __client_ms.value
	func clear_client_ms() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__client_ms.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_client_ms(value : int) -> void:
		__client_ms.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class Pong:
	extends RefCounted
	func _init():
		var service
		
		__client_ms = PBField.new("client_ms", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __client_ms
		data[__client_ms.tag] = service
		
		__server_ms = PBField.new("server_ms", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __server_ms
		data[__server_ms.tag] = service
		
		__tick = PBField.new("tick", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __tick
		data[__tick.tag] = service
		
	var data = {}
	
	var __client_ms: PBField
	func has_client_ms() -> bool:
		if __client_ms.value != null:
			return true
		return false
	func get_client_ms() -> int:
		return __client_ms.value
	func clear_client_ms() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__client_ms.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_client_ms(value : int) -> void:
		__client_ms.value = value
	
	var __server_ms: PBField
	func has_server_ms() -> bool:
		if __server_ms.value != null:
			return true
		return false
	func get_server_ms() -> int:
		return __server_ms.value
	func clear_server_ms() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__server_ms.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_server_ms(value : int) -> void:
		__server_ms.value = value
	
	var __tick: PBField
	func has_tick() -> bool:
		if __tick.value != null:
			return true
		return false
	func get_tick() -> int:
		return __tick.value
	func clear_tick() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__tick.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_tick(value : int) -> void:
		__tick.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class Kick:
	extends RefCounted
	func _init():
		var service
		
		__reason = PBField.new("reason", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __reason
		data[__reason.tag] = service
		
		__text = PBField.new("text", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __text
		data[__text.tag] = service
		
	var data = {}
	
	var __reason: PBField
	func has_reason() -> bool:
		if __reason.value != null:
			return true
		return false
	func get_reason():
		return __reason.value
	func clear_reason() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__reason.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_reason(value) -> void:
		__reason.value = value
	
	var __text: PBField
	func has_text() -> bool:
		if __text.value != null:
			return true
		return false
	func get_text() -> String:
		return __text.value
	func clear_text() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__text.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_text(value : String) -> void:
		__text.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class Vec2:
	extends RefCounted
	func _init():
		var service
		
		__x = PBField.new("x", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __x
		data[__x.tag] = service
		
		__y = PBField.new("y", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __y
		data[__y.tag] = service
		
	var data = {}
	
	var __x: PBField
	func has_x() -> bool:
		if __x.value != null:
			return true
		return false
	func get_x() -> int:
		return __x.value
	func clear_x() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__x.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_x(value : int) -> void:
		__x.value = value
	
	var __y: PBField
	func has_y() -> bool:
		if __y.value != null:
			return true
		return false
	func get_y() -> int:
		return __y.value
	func clear_y() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__y.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_y(value : int) -> void:
		__y.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
enum Result {
	OK = 0,
	INTERNAL_ERROR = 1,
	BAD_REQUEST = 2,
	UNAUTHORIZED = 3,
	NOT_FOUND = 4,
	ALREADY_EXISTS = 5,
	FULL = 6,
	INVALID_NAME = 7,
	WRONG_STATE = 8,
	ZONE_UNAVAILABLE = 9,
	VERSION_MISMATCH = 10,
	ACCOUNT_IN_USE = 11,
	ACCOUNT_FROZEN = 12,
	NO_GAME_TIME = 13,
	SERVER_BUSY = 14,
	RATE_LIMITED = 15,
	TIMEOUT = 16,
	REPLACED = 17,
	SERVER_SHUTDOWN = 18
}

enum EntityType {
	ENTITY_UNKNOWN = 0,
	ENTITY_PLAYER = 1,
	ENTITY_NPC = 2,
	ENTITY_MONSTER = 3,
	ENTITY_DROP = 4
}

class ZoneHello:
	extends RefCounted
	func _init():
		var service
		
		__protocol_version = PBField.new("protocol_version", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __protocol_version
		data[__protocol_version.tag] = service
		
		__gateway_id = PBField.new("gateway_id", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __gateway_id
		data[__gateway_id.tag] = service
		
	var data = {}
	
	var __protocol_version: PBField
	func has_protocol_version() -> bool:
		if __protocol_version.value != null:
			return true
		return false
	func get_protocol_version() -> int:
		return __protocol_version.value
	func clear_protocol_version() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__protocol_version.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_protocol_version(value : int) -> void:
		__protocol_version.value = value
	
	var __gateway_id: PBField
	func has_gateway_id() -> bool:
		if __gateway_id.value != null:
			return true
		return false
	func get_gateway_id() -> String:
		return __gateway_id.value
	func clear_gateway_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__gateway_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_gateway_id(value : String) -> void:
		__gateway_id.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ZoneHelloAck:
	extends RefCounted
	func _init():
		var service
		
		__protocol_version = PBField.new("protocol_version", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __protocol_version
		data[__protocol_version.tag] = service
		
		__zone_id = PBField.new("zone_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __zone_id
		data[__zone_id.tag] = service
		
		__zone_name = PBField.new("zone_name", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __zone_name
		data[__zone_name.tag] = service
		
		__tick_hz = PBField.new("tick_hz", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __tick_hz
		data[__tick_hz.tag] = service
		
		__capacity = PBField.new("capacity", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __capacity
		data[__capacity.tag] = service
		
		__map_id = PBField.new("map_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __map_id
		data[__map_id.tag] = service
		
		__scene_w = PBField.new("scene_w", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __scene_w
		data[__scene_w.tag] = service
		
		__scene_h = PBField.new("scene_h", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __scene_h
		data[__scene_h.tag] = service
		
		__session_prefix = PBField.new("session_prefix", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 9, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __session_prefix
		data[__session_prefix.tag] = service
		
	var data = {}
	
	var __protocol_version: PBField
	func has_protocol_version() -> bool:
		if __protocol_version.value != null:
			return true
		return false
	func get_protocol_version() -> int:
		return __protocol_version.value
	func clear_protocol_version() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__protocol_version.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_protocol_version(value : int) -> void:
		__protocol_version.value = value
	
	var __zone_id: PBField
	func has_zone_id() -> bool:
		if __zone_id.value != null:
			return true
		return false
	func get_zone_id() -> int:
		return __zone_id.value
	func clear_zone_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__zone_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_zone_id(value : int) -> void:
		__zone_id.value = value
	
	var __zone_name: PBField
	func has_zone_name() -> bool:
		if __zone_name.value != null:
			return true
		return false
	func get_zone_name() -> String:
		return __zone_name.value
	func clear_zone_name() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__zone_name.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_zone_name(value : String) -> void:
		__zone_name.value = value
	
	var __tick_hz: PBField
	func has_tick_hz() -> bool:
		if __tick_hz.value != null:
			return true
		return false
	func get_tick_hz() -> int:
		return __tick_hz.value
	func clear_tick_hz() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__tick_hz.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_tick_hz(value : int) -> void:
		__tick_hz.value = value
	
	var __capacity: PBField
	func has_capacity() -> bool:
		if __capacity.value != null:
			return true
		return false
	func get_capacity() -> int:
		return __capacity.value
	func clear_capacity() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__capacity.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_capacity(value : int) -> void:
		__capacity.value = value
	
	var __map_id: PBField
	func has_map_id() -> bool:
		if __map_id.value != null:
			return true
		return false
	func get_map_id() -> int:
		return __map_id.value
	func clear_map_id() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__map_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_map_id(value : int) -> void:
		__map_id.value = value
	
	var __scene_w: PBField
	func has_scene_w() -> bool:
		if __scene_w.value != null:
			return true
		return false
	func get_scene_w() -> int:
		return __scene_w.value
	func clear_scene_w() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__scene_w.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_scene_w(value : int) -> void:
		__scene_w.value = value
	
	var __scene_h: PBField
	func has_scene_h() -> bool:
		if __scene_h.value != null:
			return true
		return false
	func get_scene_h() -> int:
		return __scene_h.value
	func clear_scene_h() -> void:
		data[8].state = PB_SERVICE_STATE.UNFILLED
		__scene_h.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_scene_h(value : int) -> void:
		__scene_h.value = value
	
	var __session_prefix: PBField
	func has_session_prefix() -> bool:
		if __session_prefix.value != null:
			return true
		return false
	func get_session_prefix() -> int:
		return __session_prefix.value
	func clear_session_prefix() -> void:
		data[9].state = PB_SERVICE_STATE.UNFILLED
		__session_prefix.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_session_prefix(value : int) -> void:
		__session_prefix.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SessionOpen:
	extends RefCounted
	func _init():
		var service
		
		__sid = PBField.new("sid", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __sid
		data[__sid.tag] = service
		
		__account_id = PBField.new("account_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __account_id
		data[__account_id.tag] = service
		
		__role = PBField.new("role", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __role
		service.func_ref = Callable(self, "new_role")
		data[__role.tag] = service
		
		__account = PBField.new("account", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __account
		data[__account.tag] = service
		
	var data = {}
	
	var __sid: PBField
	func has_sid() -> bool:
		if __sid.value != null:
			return true
		return false
	func get_sid() -> int:
		return __sid.value
	func clear_sid() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__sid.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_sid(value : int) -> void:
		__sid.value = value
	
	var __account_id: PBField
	func has_account_id() -> bool:
		if __account_id.value != null:
			return true
		return false
	func get_account_id() -> int:
		return __account_id.value
	func clear_account_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__account_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_account_id(value : int) -> void:
		__account_id.value = value
	
	var __role: PBField
	func has_role() -> bool:
		if __role.value != null:
			return true
		return false
	func get_role() -> RoleData:
		return __role.value
	func clear_role() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__role.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_role() -> RoleData:
		__role.value = RoleData.new()
		return __role.value
	
	var __account: PBField
	func has_account() -> bool:
		if __account.value != null:
			return true
		return false
	func get_account() -> String:
		return __account.value
	func clear_account() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__account.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_account(value : String) -> void:
		__account.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SessionOpenAck:
	extends RefCounted
	func _init():
		var service
		
		__sid = PBField.new("sid", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __sid
		data[__sid.tag] = service
		
		__result = PBField.new("result", PB_DATA_TYPE.ENUM, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM])
		service = PBServiceField.new()
		service.field = __result
		data[__result.tag] = service
		
		__entity_id = PBField.new("entity_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __entity_id
		data[__entity_id.tag] = service
		
		__pos = PBField.new("pos", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __pos
		service.func_ref = Callable(self, "new_pos")
		data[__pos.tag] = service
		
		__map_id = PBField.new("map_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __map_id
		data[__map_id.tag] = service
		
		__scene_w = PBField.new("scene_w", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __scene_w
		data[__scene_w.tag] = service
		
		__scene_h = PBField.new("scene_h", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __scene_h
		data[__scene_h.tag] = service
		
	var data = {}
	
	var __sid: PBField
	func has_sid() -> bool:
		if __sid.value != null:
			return true
		return false
	func get_sid() -> int:
		return __sid.value
	func clear_sid() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__sid.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_sid(value : int) -> void:
		__sid.value = value
	
	var __result: PBField
	func has_result() -> bool:
		if __result.value != null:
			return true
		return false
	func get_result():
		return __result.value
	func clear_result() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__result.value = DEFAULT_VALUES_3[PB_DATA_TYPE.ENUM]
	func set_result(value) -> void:
		__result.value = value
	
	var __entity_id: PBField
	func has_entity_id() -> bool:
		if __entity_id.value != null:
			return true
		return false
	func get_entity_id() -> int:
		return __entity_id.value
	func clear_entity_id() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__entity_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_entity_id(value : int) -> void:
		__entity_id.value = value
	
	var __pos: PBField
	func has_pos() -> bool:
		if __pos.value != null:
			return true
		return false
	func get_pos() -> Vec2:
		return __pos.value
	func clear_pos() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__pos.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_pos() -> Vec2:
		__pos.value = Vec2.new()
		return __pos.value
	
	var __map_id: PBField
	func has_map_id() -> bool:
		if __map_id.value != null:
			return true
		return false
	func get_map_id() -> int:
		return __map_id.value
	func clear_map_id() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__map_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_map_id(value : int) -> void:
		__map_id.value = value
	
	var __scene_w: PBField
	func has_scene_w() -> bool:
		if __scene_w.value != null:
			return true
		return false
	func get_scene_w() -> int:
		return __scene_w.value
	func clear_scene_w() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__scene_w.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_scene_w(value : int) -> void:
		__scene_w.value = value
	
	var __scene_h: PBField
	func has_scene_h() -> bool:
		if __scene_h.value != null:
			return true
		return false
	func get_scene_h() -> int:
		return __scene_h.value
	func clear_scene_h() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__scene_h.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_scene_h(value : int) -> void:
		__scene_h.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class SessionClose:
	extends RefCounted
	func _init():
		var service
		
		__sid = PBField.new("sid", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __sid
		data[__sid.tag] = service
		
		__reason = PBField.new("reason", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __reason
		data[__reason.tag] = service
		
	var data = {}
	
	var __sid: PBField
	func has_sid() -> bool:
		if __sid.value != null:
			return true
		return false
	func get_sid() -> int:
		return __sid.value
	func clear_sid() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__sid.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_sid(value : int) -> void:
		__sid.value = value
	
	var __reason: PBField
	func has_reason() -> bool:
		if __reason.value != null:
			return true
		return false
	func get_reason() -> int:
		return __reason.value
	func clear_reason() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__reason.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_reason(value : int) -> void:
		__reason.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ClientPacket:
	extends RefCounted
	func _init():
		var service
		
		__sid = PBField.new("sid", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __sid
		data[__sid.tag] = service
		
		__msg_id = PBField.new("msg_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __msg_id
		data[__msg_id.tag] = service
		
		__payload = PBField.new("payload", PB_DATA_TYPE.BYTES, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BYTES])
		service = PBServiceField.new()
		service.field = __payload
		data[__payload.tag] = service
		
	var data = {}
	
	var __sid: PBField
	func has_sid() -> bool:
		if __sid.value != null:
			return true
		return false
	func get_sid() -> int:
		return __sid.value
	func clear_sid() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__sid.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_sid(value : int) -> void:
		__sid.value = value
	
	var __msg_id: PBField
	func has_msg_id() -> bool:
		if __msg_id.value != null:
			return true
		return false
	func get_msg_id() -> int:
		return __msg_id.value
	func clear_msg_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__msg_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_msg_id(value : int) -> void:
		__msg_id.value = value
	
	var __payload: PBField
	func has_payload() -> bool:
		if __payload.value != null:
			return true
		return false
	func get_payload() -> PackedByteArray:
		return __payload.value
	func clear_payload() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__payload.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BYTES]
	func set_payload(value : PackedByteArray) -> void:
		__payload.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ZonePacket:
	extends RefCounted
	func _init():
		var service
		
		var __sids_default: Array[int] = []
		__sids = PBField.new("sids", PB_DATA_TYPE.UINT64, PB_RULE.REPEATED, 1, true, __sids_default)
		service = PBServiceField.new()
		service.field = __sids
		data[__sids.tag] = service
		
		__msg_id = PBField.new("msg_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __msg_id
		data[__msg_id.tag] = service
		
		__payload = PBField.new("payload", PB_DATA_TYPE.BYTES, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BYTES])
		service = PBServiceField.new()
		service.field = __payload
		data[__payload.tag] = service
		
	var data = {}
	
	var __sids: PBField
	func get_sids() -> Array[int]:
		return __sids.value
	func clear_sids() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__sids.value.clear()
	func add_sids(value : int) -> void:
		__sids.value.append(value)
	
	var __msg_id: PBField
	func has_msg_id() -> bool:
		if __msg_id.value != null:
			return true
		return false
	func get_msg_id() -> int:
		return __msg_id.value
	func clear_msg_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__msg_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_msg_id(value : int) -> void:
		__msg_id.value = value
	
	var __payload: PBField
	func has_payload() -> bool:
		if __payload.value != null:
			return true
		return false
	func get_payload() -> PackedByteArray:
		return __payload.value
	func clear_payload() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__payload.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BYTES]
	func set_payload(value : PackedByteArray) -> void:
		__payload.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class PlayerSave:
	extends RefCounted
	func _init():
		var service
		
		__sid = PBField.new("sid", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __sid
		data[__sid.tag] = service
		
		__role = PBField.new("role", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __role
		service.func_ref = Callable(self, "new_role")
		data[__role.tag] = service
		
		__final = PBField.new("final", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __final
		data[__final.tag] = service
		
		__tick = PBField.new("tick", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __tick
		data[__tick.tag] = service
		
	var data = {}
	
	var __sid: PBField
	func has_sid() -> bool:
		if __sid.value != null:
			return true
		return false
	func get_sid() -> int:
		return __sid.value
	func clear_sid() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__sid.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_sid(value : int) -> void:
		__sid.value = value
	
	var __role: PBField
	func has_role() -> bool:
		if __role.value != null:
			return true
		return false
	func get_role() -> RoleData:
		return __role.value
	func clear_role() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__role.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_role() -> RoleData:
		__role.value = RoleData.new()
		return __role.value
	
	var __final: PBField
	func has_final() -> bool:
		if __final.value != null:
			return true
		return false
	func get_final() -> bool:
		return __final.value
	func clear_final() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__final.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_final(value : bool) -> void:
		__final.value = value
	
	var __tick: PBField
	func has_tick() -> bool:
		if __tick.value != null:
			return true
		return false
	func get_tick() -> int:
		return __tick.value
	func clear_tick() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__tick.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_tick(value : int) -> void:
		__tick.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ZoneStats:
	extends RefCounted
	func _init():
		var service
		
		__zone_id = PBField.new("zone_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __zone_id
		data[__zone_id.tag] = service
		
		__tick = PBField.new("tick", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __tick
		data[__tick.tag] = service
		
		__players = PBField.new("players", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __players
		data[__players.tag] = service
		
		__entities = PBField.new("entities", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __entities
		data[__entities.tag] = service
		
		__tick_ms_avg = PBField.new("tick_ms_avg", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __tick_ms_avg
		data[__tick_ms_avg.tag] = service
		
		__tick_ms_max = PBField.new("tick_ms_max", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __tick_ms_max
		data[__tick_ms_max.tag] = service
		
	var data = {}
	
	var __zone_id: PBField
	func has_zone_id() -> bool:
		if __zone_id.value != null:
			return true
		return false
	func get_zone_id() -> int:
		return __zone_id.value
	func clear_zone_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__zone_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_zone_id(value : int) -> void:
		__zone_id.value = value
	
	var __tick: PBField
	func has_tick() -> bool:
		if __tick.value != null:
			return true
		return false
	func get_tick() -> int:
		return __tick.value
	func clear_tick() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__tick.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_tick(value : int) -> void:
		__tick.value = value
	
	var __players: PBField
	func has_players() -> bool:
		if __players.value != null:
			return true
		return false
	func get_players() -> int:
		return __players.value
	func clear_players() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__players.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_players(value : int) -> void:
		__players.value = value
	
	var __entities: PBField
	func has_entities() -> bool:
		if __entities.value != null:
			return true
		return false
	func get_entities() -> int:
		return __entities.value
	func clear_entities() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__entities.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_entities(value : int) -> void:
		__entities.value = value
	
	var __tick_ms_avg: PBField
	func has_tick_ms_avg() -> bool:
		if __tick_ms_avg.value != null:
			return true
		return false
	func get_tick_ms_avg() -> int:
		return __tick_ms_avg.value
	func clear_tick_ms_avg() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__tick_ms_avg.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_tick_ms_avg(value : int) -> void:
		__tick_ms_avg.value = value
	
	var __tick_ms_max: PBField
	func has_tick_ms_max() -> bool:
		if __tick_ms_max.value != null:
			return true
		return false
	func get_tick_ms_max() -> int:
		return __tick_ms_max.value
	func clear_tick_ms_max() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__tick_ms_max.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_tick_ms_max(value : int) -> void:
		__tick_ms_max.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
enum Protocol {
	UNSPECIFIED = 0,
	VERSION = 1
}

enum MsgId {
	MSG_NONE = 0,
	C2G_HELLO = 1001,
	C2G_LOGIN = 1002,
	C2G_CHAR_LIST = 1003,
	C2G_CHAR_CREATE = 1004,
	C2G_ENTER_WORLD = 1005,
	C2G_LEAVE_WORLD = 1006,
	C2G_PING = 1007,
	C2G_MOVE = 1101,
	C2G_CHAT = 1102,
	C2G_ATTACK = 1103,
	C2G_ITEM_MOVE = 1104,
	C2G_ITEM_EQUIP = 1105,
	C2G_ITEM_UNEQUIP = 1106,
	C2G_ITEM_USE = 1107,
	C2G_ITEM_DROP = 1108,
	C2G_PICK_UP = 1109,
	C2G_ADD_POINT = 1110,
	C2G_ADD_SKILL_POINT = 1111,
	C2G_CAST_SKILL = 1112,
	C2G_REVIVE = 1113,
	C2G_RIDE = 1114,
	C2G_SKILL_DESC = 1115,
	C2G_SET_AURA = 1116,
	C2G_SIT = 1117,
	C2G_PK_STATE = 1118,
	C2G_TEAM = 1119,
	C2G_TRADE = 1120,
	C2G_NPC_DIALOG = 1121,
	C2G_SCRIPT_INPUT = 1125,
	C2G_GIVE_ITEMS = 1124,
	C2G_TASK_VALUE = 1123,
	C2G_DIALOG_ANSWER = 1122,
	G2C_HELLO_ACK = 2001,
	G2C_LOGIN_RES = 2002,
	G2C_CHAR_LIST_RES = 2003,
	G2C_CHAR_CREATE_RES = 2004,
	G2C_ENTER_WORLD_RES = 2005,
	G2C_PONG = 2007,
	G2C_KICK = 2008,
	G2C_ENTITY_SPAWN = 2101,
	G2C_ENTITY_DESPAWN = 2102,
	G2C_ENTITY_MOVE = 2103,
	G2C_CHAT_MSG = 2104,
	G2C_ENTITY_ACTION = 2105,
	G2C_ENTITY_LIFE = 2106,
	G2C_CHANGE_MAP = 2107,
	G2C_ENTITY_MOVES = 2108,
	G2C_ITEM_LIST = 2109,
	G2C_ITEM_ADD = 2110,
	G2C_ITEM_REMOVE = 2111,
	G2C_ITEM_MOVE = 2112,
	G2C_ITEM_RESULT = 2113,
	G2C_MONEY = 2114,
	G2C_PLAYER_ATTRIB = 2115,
	G2C_SKILL_LIST = 2116,
	G2C_SKILL_LEVEL = 2117,
	G2C_SKILL_FORBID = 2118,
	G2C_ENTITY_RIDE = 2119,
	G2C_ENTITY_CAMP = 2120,
	G2C_PLAYER_FACTION = 2121,
	G2C_ENTITY_STATE = 2122,
	G2C_SKILL_DESC = 2123,
	G2C_MISSLE = 2124,
	G2C_STATE_ICONS = 2125,
	G2C_NPC_GOLD = 2126,
	G2C_ENTITY_RES = 2127,
	G2C_PK_STATE = 2128,
	G2C_ENTITY_PK = 2129,
	G2C_TEAM_SELF = 2130,
	G2C_TEAM_EVENT = 2131,
	G2C_TRADE_STATE = 2132,
	G2C_TRADE_SYNC = 2133,
	G2C_TRADE_ITEM = 2134,
	G2C_TRADE_APPLY = 2135,
	G2C_TRADE_END = 2136,
	G2C_SYS_MSG = 2137,
	G2C_ENTITY_MENU_STATE = 2138,
	G2C_TASK_VALUE = 2140,
	G2C_TASK_VALUES = 2141,
	G2C_SCRIPT_ASK = 2144,
	G2C_GIVE_ITEM_MSG = 2143,
	G2C_TASK_TIP = 2142,
	G2C_SCRIPT_ACTION = 2139,
	GZ_ZONE_HELLO = 9001,
	ZG_ZONE_HELLO_ACK = 9002,
	GZ_SESSION_OPEN = 9003,
	ZG_SESSION_OPEN_ACK = 9004,
	GZ_SESSION_CLOSE = 9005,
	GZ_CLIENT_PACKET = 9006,
	ZG_ZONE_PACKET = 9007,
	ZG_PLAYER_SAVE = 9008,
	ZG_ZONE_STATS = 9009
}

class RolePosition:
	extends RefCounted
	func _init():
		var service
		
		__zone_id = PBField.new("zone_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __zone_id
		data[__zone_id.tag] = service
		
		__pos = PBField.new("pos", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __pos
		service.func_ref = Callable(self, "new_pos")
		data[__pos.tag] = service
		
		__map_id = PBField.new("map_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __map_id
		data[__map_id.tag] = service
		
	var data = {}
	
	var __zone_id: PBField
	func has_zone_id() -> bool:
		if __zone_id.value != null:
			return true
		return false
	func get_zone_id() -> int:
		return __zone_id.value
	func clear_zone_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__zone_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_zone_id(value : int) -> void:
		__zone_id.value = value
	
	var __pos: PBField
	func has_pos() -> bool:
		if __pos.value != null:
			return true
		return false
	func get_pos() -> Vec2:
		return __pos.value
	func clear_pos() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__pos.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_pos() -> Vec2:
		__pos.value = Vec2.new()
		return __pos.value
	
	var __map_id: PBField
	func has_map_id() -> bool:
		if __map_id.value != null:
			return true
		return false
	func get_map_id() -> int:
		return __map_id.value
	func clear_map_id() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__map_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_map_id(value : int) -> void:
		__map_id.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class RoleStats:
	extends RefCounted
	func _init():
		var service
		
		__hp = PBField.new("hp", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __hp
		data[__hp.tag] = service
		
		__hp_max = PBField.new("hp_max", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __hp_max
		data[__hp_max.tag] = service
		
		__mp = PBField.new("mp", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __mp
		data[__mp.tag] = service
		
		__mp_max = PBField.new("mp_max", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __mp_max
		data[__mp_max.tag] = service
		
		__stamina = PBField.new("stamina", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __stamina
		data[__stamina.tag] = service
		
		__stamina_max = PBField.new("stamina_max", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __stamina_max
		data[__stamina_max.tag] = service
		
		__strength = PBField.new("strength", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __strength
		data[__strength.tag] = service
		
		__dexterity = PBField.new("dexterity", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __dexterity
		data[__dexterity.tag] = service
		
		__vitality = PBField.new("vitality", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 9, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __vitality
		data[__vitality.tag] = service
		
		__energy = PBField.new("energy", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 10, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __energy
		data[__energy.tag] = service
		
		__move_speed = PBField.new("move_speed", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 11, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __move_speed
		data[__move_speed.tag] = service
		
		__lucky = PBField.new("lucky", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 12, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __lucky
		data[__lucky.tag] = service
		
		__attribute_point = PBField.new("attribute_point", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 13, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __attribute_point
		data[__attribute_point.tag] = service
		
		__skill_point = PBField.new("skill_point", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 14, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __skill_point
		data[__skill_point.tag] = service
		
		__reborn = PBField.new("reborn", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 15, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __reborn
		data[__reborn.tag] = service
		
		__camp = PBField.new("camp", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 16, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __camp
		data[__camp.tag] = service
		
		var __ext_point_default: Array[int] = []
		__ext_point = PBField.new("ext_point", PB_DATA_TYPE.INT32, PB_RULE.REPEATED, 17, true, __ext_point_default)
		service = PBServiceField.new()
		service.field = __ext_point
		data[__ext_point.tag] = service
		
	var data = {}
	
	var __hp: PBField
	func has_hp() -> bool:
		if __hp.value != null:
			return true
		return false
	func get_hp() -> int:
		return __hp.value
	func clear_hp() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__hp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_hp(value : int) -> void:
		__hp.value = value
	
	var __hp_max: PBField
	func has_hp_max() -> bool:
		if __hp_max.value != null:
			return true
		return false
	func get_hp_max() -> int:
		return __hp_max.value
	func clear_hp_max() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__hp_max.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_hp_max(value : int) -> void:
		__hp_max.value = value
	
	var __mp: PBField
	func has_mp() -> bool:
		if __mp.value != null:
			return true
		return false
	func get_mp() -> int:
		return __mp.value
	func clear_mp() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__mp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_mp(value : int) -> void:
		__mp.value = value
	
	var __mp_max: PBField
	func has_mp_max() -> bool:
		if __mp_max.value != null:
			return true
		return false
	func get_mp_max() -> int:
		return __mp_max.value
	func clear_mp_max() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__mp_max.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_mp_max(value : int) -> void:
		__mp_max.value = value
	
	var __stamina: PBField
	func has_stamina() -> bool:
		if __stamina.value != null:
			return true
		return false
	func get_stamina() -> int:
		return __stamina.value
	func clear_stamina() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__stamina.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_stamina(value : int) -> void:
		__stamina.value = value
	
	var __stamina_max: PBField
	func has_stamina_max() -> bool:
		if __stamina_max.value != null:
			return true
		return false
	func get_stamina_max() -> int:
		return __stamina_max.value
	func clear_stamina_max() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__stamina_max.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_stamina_max(value : int) -> void:
		__stamina_max.value = value
	
	var __strength: PBField
	func has_strength() -> bool:
		if __strength.value != null:
			return true
		return false
	func get_strength() -> int:
		return __strength.value
	func clear_strength() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__strength.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_strength(value : int) -> void:
		__strength.value = value
	
	var __dexterity: PBField
	func has_dexterity() -> bool:
		if __dexterity.value != null:
			return true
		return false
	func get_dexterity() -> int:
		return __dexterity.value
	func clear_dexterity() -> void:
		data[8].state = PB_SERVICE_STATE.UNFILLED
		__dexterity.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_dexterity(value : int) -> void:
		__dexterity.value = value
	
	var __vitality: PBField
	func has_vitality() -> bool:
		if __vitality.value != null:
			return true
		return false
	func get_vitality() -> int:
		return __vitality.value
	func clear_vitality() -> void:
		data[9].state = PB_SERVICE_STATE.UNFILLED
		__vitality.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_vitality(value : int) -> void:
		__vitality.value = value
	
	var __energy: PBField
	func has_energy() -> bool:
		if __energy.value != null:
			return true
		return false
	func get_energy() -> int:
		return __energy.value
	func clear_energy() -> void:
		data[10].state = PB_SERVICE_STATE.UNFILLED
		__energy.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_energy(value : int) -> void:
		__energy.value = value
	
	var __move_speed: PBField
	func has_move_speed() -> bool:
		if __move_speed.value != null:
			return true
		return false
	func get_move_speed() -> int:
		return __move_speed.value
	func clear_move_speed() -> void:
		data[11].state = PB_SERVICE_STATE.UNFILLED
		__move_speed.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_move_speed(value : int) -> void:
		__move_speed.value = value
	
	var __lucky: PBField
	func has_lucky() -> bool:
		if __lucky.value != null:
			return true
		return false
	func get_lucky() -> int:
		return __lucky.value
	func clear_lucky() -> void:
		data[12].state = PB_SERVICE_STATE.UNFILLED
		__lucky.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_lucky(value : int) -> void:
		__lucky.value = value
	
	var __attribute_point: PBField
	func has_attribute_point() -> bool:
		if __attribute_point.value != null:
			return true
		return false
	func get_attribute_point() -> int:
		return __attribute_point.value
	func clear_attribute_point() -> void:
		data[13].state = PB_SERVICE_STATE.UNFILLED
		__attribute_point.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_attribute_point(value : int) -> void:
		__attribute_point.value = value
	
	var __skill_point: PBField
	func has_skill_point() -> bool:
		if __skill_point.value != null:
			return true
		return false
	func get_skill_point() -> int:
		return __skill_point.value
	func clear_skill_point() -> void:
		data[14].state = PB_SERVICE_STATE.UNFILLED
		__skill_point.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_skill_point(value : int) -> void:
		__skill_point.value = value
	
	var __reborn: PBField
	func has_reborn() -> bool:
		if __reborn.value != null:
			return true
		return false
	func get_reborn() -> int:
		return __reborn.value
	func clear_reborn() -> void:
		data[15].state = PB_SERVICE_STATE.UNFILLED
		__reborn.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_reborn(value : int) -> void:
		__reborn.value = value
	
	var __camp: PBField
	func has_camp() -> bool:
		if __camp.value != null:
			return true
		return false
	func get_camp() -> int:
		return __camp.value
	func clear_camp() -> void:
		data[16].state = PB_SERVICE_STATE.UNFILLED
		__camp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_camp(value : int) -> void:
		__camp.value = value
	
	var __ext_point: PBField
	func get_ext_point() -> Array[int]:
		return __ext_point.value
	func clear_ext_point() -> void:
		data[17].state = PB_SERVICE_STATE.UNFILLED
		__ext_point.value.clear()
	func add_ext_point(value : int) -> void:
		__ext_point.value.append(value)
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ItemMagic:
	extends RefCounted
	func _init():
		var service
		
		__type = PBField.new("type", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __type
		data[__type.tag] = service
		
		var __value_default: Array[int] = []
		__value = PBField.new("value", PB_DATA_TYPE.INT32, PB_RULE.REPEATED, 2, true, __value_default)
		service = PBServiceField.new()
		service.field = __value
		data[__value.tag] = service
		
	var data = {}
	
	var __type: PBField
	func has_type() -> bool:
		if __type.value != null:
			return true
		return false
	func get_type() -> int:
		return __type.value
	func clear_type() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__type.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_type(value : int) -> void:
		__type.value = value
	
	var __value: PBField
	func get_value() -> Array[int]:
		return __value.value
	func clear_value() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__value.value.clear()
	func add_value(value : int) -> void:
		__value.value.append(value)
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class ItemData:
	extends RefCounted
	func _init():
		var service
		
		__id = PBField.new("id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __id
		data[__id.tag] = service
		
		__version = PBField.new("version", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __version
		data[__version.tag] = service
		
		__genre = PBField.new("genre", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __genre
		data[__genre.tag] = service
		
		__detail = PBField.new("detail", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __detail
		data[__detail.tag] = service
		
		__particular = PBField.new("particular", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __particular
		data[__particular.tag] = service
		
		__level = PBField.new("level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
		__series = PBField.new("series", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __series
		data[__series.tag] = service
		
		__count = PBField.new("count", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __count
		data[__count.tag] = service
		
		__durability = PBField.new("durability", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 9, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __durability
		data[__durability.tag] = service
		
		__ex_type = PBField.new("ex_type", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 10, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __ex_type
		data[__ex_type.tag] = service
		
		__gen_param = PBField.new("gen_param", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 11, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __gen_param
		data[__gen_param.tag] = service
		
		__group = PBField.new("group", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 12, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __group
		data[__group.tag] = service
		
		__ex_group = PBField.new("ex_group", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 13, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __ex_group
		data[__ex_group.tag] = service
		
		__group_serial = PBField.new("group_serial", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 14, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __group_serial
		data[__group_serial.tag] = service
		
		var __base_default: Array[ItemMagic] = []
		__base = PBField.new("base", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 15, true, __base_default)
		service = PBServiceField.new()
		service.field = __base
		service.func_ref = Callable(self, "add_base")
		data[__base.tag] = service
		
		var __require_default: Array[ItemMagic] = []
		__require = PBField.new("require", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 16, true, __require_default)
		service = PBServiceField.new()
		service.field = __require
		service.func_ref = Callable(self, "add_require")
		data[__require.tag] = service
		
		var __magic_default: Array[ItemMagic] = []
		__magic = PBField.new("magic", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 17, true, __magic_default)
		service = PBServiceField.new()
		service.field = __magic
		service.func_ref = Callable(self, "add_magic")
		data[__magic.tag] = service
		
		var __magic_ex_default: Array[ItemMagic] = []
		__magic_ex = PBField.new("magic_ex", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 18, true, __magic_ex_default)
		service = PBServiceField.new()
		service.field = __magic_ex
		service.func_ref = Callable(self, "add_magic_ex")
		data[__magic_ex.tag] = service
		
		__room = PBField.new("room", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 19, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __room
		data[__room.tag] = service
		
		__x = PBField.new("x", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 20, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __x
		data[__x.tag] = service
		
		__y = PBField.new("y", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 21, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __y
		data[__y.tag] = service
		
		__rand_seed = PBField.new("rand_seed", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 22, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __rand_seed
		data[__rand_seed.tag] = service
		
		var __magic_level_default: Array[int] = []
		__magic_level = PBField.new("magic_level", PB_DATA_TYPE.INT32, PB_RULE.REPEATED, 23, true, __magic_level_default)
		service = PBServiceField.new()
		service.field = __magic_level
		data[__magic_level.tag] = service
		
		__luck = PBField.new("luck", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 24, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __luck
		data[__luck.tag] = service
		
	var data = {}
	
	var __id: PBField
	func has_id() -> bool:
		if __id.value != null:
			return true
		return false
	func get_id() -> int:
		return __id.value
	func clear_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_id(value : int) -> void:
		__id.value = value
	
	var __version: PBField
	func has_version() -> bool:
		if __version.value != null:
			return true
		return false
	func get_version() -> int:
		return __version.value
	func clear_version() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__version.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_version(value : int) -> void:
		__version.value = value
	
	var __genre: PBField
	func has_genre() -> bool:
		if __genre.value != null:
			return true
		return false
	func get_genre() -> int:
		return __genre.value
	func clear_genre() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__genre.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_genre(value : int) -> void:
		__genre.value = value
	
	var __detail: PBField
	func has_detail() -> bool:
		if __detail.value != null:
			return true
		return false
	func get_detail() -> int:
		return __detail.value
	func clear_detail() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__detail.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_detail(value : int) -> void:
		__detail.value = value
	
	var __particular: PBField
	func has_particular() -> bool:
		if __particular.value != null:
			return true
		return false
	func get_particular() -> int:
		return __particular.value
	func clear_particular() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__particular.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_particular(value : int) -> void:
		__particular.value = value
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	var __series: PBField
	func has_series() -> bool:
		if __series.value != null:
			return true
		return false
	func get_series() -> int:
		return __series.value
	func clear_series() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__series.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_series(value : int) -> void:
		__series.value = value
	
	var __count: PBField
	func has_count() -> bool:
		if __count.value != null:
			return true
		return false
	func get_count() -> int:
		return __count.value
	func clear_count() -> void:
		data[8].state = PB_SERVICE_STATE.UNFILLED
		__count.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_count(value : int) -> void:
		__count.value = value
	
	var __durability: PBField
	func has_durability() -> bool:
		if __durability.value != null:
			return true
		return false
	func get_durability() -> int:
		return __durability.value
	func clear_durability() -> void:
		data[9].state = PB_SERVICE_STATE.UNFILLED
		__durability.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_durability(value : int) -> void:
		__durability.value = value
	
	var __ex_type: PBField
	func has_ex_type() -> bool:
		if __ex_type.value != null:
			return true
		return false
	func get_ex_type() -> int:
		return __ex_type.value
	func clear_ex_type() -> void:
		data[10].state = PB_SERVICE_STATE.UNFILLED
		__ex_type.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_ex_type(value : int) -> void:
		__ex_type.value = value
	
	var __gen_param: PBField
	func has_gen_param() -> bool:
		if __gen_param.value != null:
			return true
		return false
	func get_gen_param() -> int:
		return __gen_param.value
	func clear_gen_param() -> void:
		data[11].state = PB_SERVICE_STATE.UNFILLED
		__gen_param.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_gen_param(value : int) -> void:
		__gen_param.value = value
	
	var __group: PBField
	func has_group() -> bool:
		if __group.value != null:
			return true
		return false
	func get_group() -> int:
		return __group.value
	func clear_group() -> void:
		data[12].state = PB_SERVICE_STATE.UNFILLED
		__group.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_group(value : int) -> void:
		__group.value = value
	
	var __ex_group: PBField
	func has_ex_group() -> bool:
		if __ex_group.value != null:
			return true
		return false
	func get_ex_group() -> int:
		return __ex_group.value
	func clear_ex_group() -> void:
		data[13].state = PB_SERVICE_STATE.UNFILLED
		__ex_group.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_ex_group(value : int) -> void:
		__ex_group.value = value
	
	var __group_serial: PBField
	func has_group_serial() -> bool:
		if __group_serial.value != null:
			return true
		return false
	func get_group_serial() -> int:
		return __group_serial.value
	func clear_group_serial() -> void:
		data[14].state = PB_SERVICE_STATE.UNFILLED
		__group_serial.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_group_serial(value : int) -> void:
		__group_serial.value = value
	
	var __base: PBField
	func get_base() -> Array[ItemMagic]:
		return __base.value
	func clear_base() -> void:
		data[15].state = PB_SERVICE_STATE.UNFILLED
		__base.value.clear()
	func add_base() -> ItemMagic:
		var element = ItemMagic.new()
		__base.value.append(element)
		return element
	
	var __require: PBField
	func get_require() -> Array[ItemMagic]:
		return __require.value
	func clear_require() -> void:
		data[16].state = PB_SERVICE_STATE.UNFILLED
		__require.value.clear()
	func add_require() -> ItemMagic:
		var element = ItemMagic.new()
		__require.value.append(element)
		return element
	
	var __magic: PBField
	func get_magic() -> Array[ItemMagic]:
		return __magic.value
	func clear_magic() -> void:
		data[17].state = PB_SERVICE_STATE.UNFILLED
		__magic.value.clear()
	func add_magic() -> ItemMagic:
		var element = ItemMagic.new()
		__magic.value.append(element)
		return element
	
	var __magic_ex: PBField
	func get_magic_ex() -> Array[ItemMagic]:
		return __magic_ex.value
	func clear_magic_ex() -> void:
		data[18].state = PB_SERVICE_STATE.UNFILLED
		__magic_ex.value.clear()
	func add_magic_ex() -> ItemMagic:
		var element = ItemMagic.new()
		__magic_ex.value.append(element)
		return element
	
	var __room: PBField
	func has_room() -> bool:
		if __room.value != null:
			return true
		return false
	func get_room() -> int:
		return __room.value
	func clear_room() -> void:
		data[19].state = PB_SERVICE_STATE.UNFILLED
		__room.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_room(value : int) -> void:
		__room.value = value
	
	var __x: PBField
	func has_x() -> bool:
		if __x.value != null:
			return true
		return false
	func get_x() -> int:
		return __x.value
	func clear_x() -> void:
		data[20].state = PB_SERVICE_STATE.UNFILLED
		__x.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_x(value : int) -> void:
		__x.value = value
	
	var __y: PBField
	func has_y() -> bool:
		if __y.value != null:
			return true
		return false
	func get_y() -> int:
		return __y.value
	func clear_y() -> void:
		data[21].state = PB_SERVICE_STATE.UNFILLED
		__y.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_y(value : int) -> void:
		__y.value = value
	
	var __rand_seed: PBField
	func has_rand_seed() -> bool:
		if __rand_seed.value != null:
			return true
		return false
	func get_rand_seed() -> int:
		return __rand_seed.value
	func clear_rand_seed() -> void:
		data[22].state = PB_SERVICE_STATE.UNFILLED
		__rand_seed.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_rand_seed(value : int) -> void:
		__rand_seed.value = value
	
	var __magic_level: PBField
	func get_magic_level() -> Array[int]:
		return __magic_level.value
	func clear_magic_level() -> void:
		data[23].state = PB_SERVICE_STATE.UNFILLED
		__magic_level.value.clear()
	func add_magic_level(value : int) -> void:
		__magic_level.value.append(value)
	
	var __luck: PBField
	func has_luck() -> bool:
		if __luck.value != null:
			return true
		return false
	func get_luck() -> int:
		return __luck.value
	func clear_luck() -> void:
		data[24].state = PB_SERVICE_STATE.UNFILLED
		__luck.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_luck(value : int) -> void:
		__luck.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class RoleSkill:
	extends RefCounted
	func _init():
		var service
		
		__id = PBField.new("id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __id
		data[__id.tag] = service
		
		__level = PBField.new("level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
		__exp = PBField.new("exp", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __exp
		data[__exp.tag] = service
		
	var data = {}
	
	var __id: PBField
	func has_id() -> bool:
		if __id.value != null:
			return true
		return false
	func get_id() -> int:
		return __id.value
	func clear_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_id(value : int) -> void:
		__id.value = value
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	var __exp: PBField
	func has_exp() -> bool:
		if __exp.value != null:
			return true
		return false
	func get_exp() -> int:
		return __exp.value
	func clear_exp() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__exp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_exp(value : int) -> void:
		__exp.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class RoleTaskValue:
	extends RefCounted
	func _init():
		var service
		
		__id = PBField.new("id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __id
		data[__id.tag] = service
		
		__value = PBField.new("value", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __value
		data[__value.tag] = service
		
	var data = {}
	
	var __id: PBField
	func has_id() -> bool:
		if __id.value != null:
			return true
		return false
	func get_id() -> int:
		return __id.value
	func clear_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_id(value : int) -> void:
		__id.value = value
	
	var __value: PBField
	func has_value() -> bool:
		if __value.value != null:
			return true
		return false
	func get_value() -> int:
		return __value.value
	func clear_value() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__value.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_value(value : int) -> void:
		__value.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class RolePlayerEvent:
	extends RefCounted
	func _init():
		var service
		
		__id = PBField.new("id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __id
		data[__id.tag] = service
		
		__count = PBField.new("count", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __count
		data[__count.tag] = service
		
	var data = {}
	
	var __id: PBField
	func has_id() -> bool:
		if __id.value != null:
			return true
		return false
	func get_id() -> int:
		return __id.value
	func clear_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_id(value : int) -> void:
		__id.value = value
	
	var __count: PBField
	func has_count() -> bool:
		if __count.value != null:
			return true
		return false
	func get_count() -> int:
		return __count.value
	func clear_count() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__count.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_count(value : int) -> void:
		__count.value = value
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
class RoleData:
	extends RefCounted
	func _init():
		var service
		
		__player_id = PBField.new("player_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __player_id
		data[__player_id.tag] = service
		
		__account_id = PBField.new("account_id", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __account_id
		data[__account_id.tag] = service
		
		__name = PBField.new("name", PB_DATA_TYPE.STRING, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.STRING])
		service = PBServiceField.new()
		service.field = __name
		data[__name.tag] = service
		
		__level = PBField.new("level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __level
		data[__level.tag] = service
		
		__exp = PBField.new("exp", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __exp
		data[__exp.tag] = service
		
		__series = PBField.new("series", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __series
		data[__series.tag] = service
		
		__sex = PBField.new("sex", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 7, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __sex
		data[__sex.tag] = service
		
		__faction = PBField.new("faction", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __faction
		data[__faction.tag] = service
		
		__position = PBField.new("position", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 9, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __position
		service.func_ref = Callable(self, "new_position")
		data[__position.tag] = service
		
		__stats = PBField.new("stats", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 10, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __stats
		service.func_ref = Callable(self, "new_stats")
		data[__stats.tag] = service
		
		var __items_default: Array[ItemData] = []
		__items = PBField.new("items", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 11, true, __items_default)
		service = PBServiceField.new()
		service.field = __items
		service.func_ref = Callable(self, "add_items")
		data[__items.tag] = service
		
		__created_at_ms = PBField.new("created_at_ms", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 12, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __created_at_ms
		data[__created_at_ms.tag] = service
		
		__last_login_ms = PBField.new("last_login_ms", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 13, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __last_login_ms
		data[__last_login_ms.tag] = service
		
		__play_time_s = PBField.new("play_time_s", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 14, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __play_time_s
		data[__play_time_s.tag] = service
		
		__data_version = PBField.new("data_version", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 15, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __data_version
		data[__data_version.tag] = service
		
		__fight_mode = PBField.new("fight_mode", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 16, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __fight_mode
		data[__fight_mode.tag] = service
		
		__native_place = PBField.new("native_place", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 17, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __native_place
		data[__native_place.tag] = service
		
		__next_item_id = PBField.new("next_item_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 18, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __next_item_id
		data[__next_item_id.tag] = service
		
		__money = PBField.new("money", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 19, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __money
		data[__money.tag] = service
		
		__bank_money = PBField.new("bank_money", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 20, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __bank_money
		data[__bank_money.tag] = service
		
		var __skills_default: Array[RoleSkill] = []
		__skills = PBField.new("skills", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 21, true, __skills_default)
		service = PBServiceField.new()
		service.field = __skills
		service.func_ref = Callable(self, "add_skills")
		data[__skills.tag] = service
		
		__revive_map = PBField.new("revive_map", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 22, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __revive_map
		data[__revive_map.tag] = service
		
		__revive_pos = PBField.new("revive_pos", PB_DATA_TYPE.MESSAGE, PB_RULE.OPTIONAL, 23, true, DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE])
		service = PBServiceField.new()
		service.field = __revive_pos
		service.func_ref = Callable(self, "new_revive_pos")
		data[__revive_pos.tag] = service
		
		__revive_ref = PBField.new("revive_ref", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 24, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __revive_ref
		data[__revive_ref.tag] = service
		
		__faction_last = PBField.new("faction_last", PB_DATA_TYPE.INT32, PB_RULE.OPTIONAL, 25, true, DEFAULT_VALUES_3[PB_DATA_TYPE.INT32])
		service = PBServiceField.new()
		service.field = __faction_last
		data[__faction_last.tag] = service
		
		__faction_count = PBField.new("faction_count", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 26, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __faction_count
		data[__faction_count.tag] = service
		
		__pk_state = PBField.new("pk_state", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 27, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __pk_state
		data[__pk_state.tag] = service
		
		__pk_value = PBField.new("pk_value", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 28, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __pk_value
		data[__pk_value.tag] = service
		
		__pk_locked = PBField.new("pk_locked", PB_DATA_TYPE.BOOL, PB_RULE.OPTIONAL, 29, true, DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL])
		service = PBServiceField.new()
		service.field = __pk_locked
		data[__pk_locked.tag] = service
		
		__lead_exp = PBField.new("lead_exp", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 30, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __lead_exp
		data[__lead_exp.tag] = service
		
		__lead_level = PBField.new("lead_level", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 31, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __lead_level
		data[__lead_level.tag] = service
		
		var __task_values_default: Array[RoleTaskValue] = []
		__task_values = PBField.new("task_values", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 32, true, __task_values_default)
		service = PBServiceField.new()
		service.field = __task_values
		service.func_ref = Callable(self, "add_task_values")
		data[__task_values.tag] = service
		
		var __events_default: Array[RolePlayerEvent] = []
		__events = PBField.new("events", PB_DATA_TYPE.MESSAGE, PB_RULE.REPEATED, 33, true, __events_default)
		service = PBServiceField.new()
		service.field = __events
		service.func_ref = Callable(self, "add_events")
		data[__events.tag] = service
		
	var data = {}
	
	var __player_id: PBField
	func has_player_id() -> bool:
		if __player_id.value != null:
			return true
		return false
	func get_player_id() -> int:
		return __player_id.value
	func clear_player_id() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__player_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_player_id(value : int) -> void:
		__player_id.value = value
	
	var __account_id: PBField
	func has_account_id() -> bool:
		if __account_id.value != null:
			return true
		return false
	func get_account_id() -> int:
		return __account_id.value
	func clear_account_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__account_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_account_id(value : int) -> void:
		__account_id.value = value
	
	var __name: PBField
	func has_name() -> bool:
		if __name.value != null:
			return true
		return false
	func get_name() -> String:
		return __name.value
	func clear_name() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__name.value = DEFAULT_VALUES_3[PB_DATA_TYPE.STRING]
	func set_name(value : String) -> void:
		__name.value = value
	
	var __level: PBField
	func has_level() -> bool:
		if __level.value != null:
			return true
		return false
	func get_level() -> int:
		return __level.value
	func clear_level() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_level(value : int) -> void:
		__level.value = value
	
	var __exp: PBField
	func has_exp() -> bool:
		if __exp.value != null:
			return true
		return false
	func get_exp() -> int:
		return __exp.value
	func clear_exp() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__exp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_exp(value : int) -> void:
		__exp.value = value
	
	var __series: PBField
	func has_series() -> bool:
		if __series.value != null:
			return true
		return false
	func get_series() -> int:
		return __series.value
	func clear_series() -> void:
		data[6].state = PB_SERVICE_STATE.UNFILLED
		__series.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_series(value : int) -> void:
		__series.value = value
	
	var __sex: PBField
	func has_sex() -> bool:
		if __sex.value != null:
			return true
		return false
	func get_sex() -> int:
		return __sex.value
	func clear_sex() -> void:
		data[7].state = PB_SERVICE_STATE.UNFILLED
		__sex.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_sex(value : int) -> void:
		__sex.value = value
	
	var __faction: PBField
	func has_faction() -> bool:
		if __faction.value != null:
			return true
		return false
	func get_faction() -> int:
		return __faction.value
	func clear_faction() -> void:
		data[8].state = PB_SERVICE_STATE.UNFILLED
		__faction.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_faction(value : int) -> void:
		__faction.value = value
	
	var __position: PBField
	func has_position() -> bool:
		if __position.value != null:
			return true
		return false
	func get_position() -> RolePosition:
		return __position.value
	func clear_position() -> void:
		data[9].state = PB_SERVICE_STATE.UNFILLED
		__position.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_position() -> RolePosition:
		__position.value = RolePosition.new()
		return __position.value
	
	var __stats: PBField
	func has_stats() -> bool:
		if __stats.value != null:
			return true
		return false
	func get_stats() -> RoleStats:
		return __stats.value
	func clear_stats() -> void:
		data[10].state = PB_SERVICE_STATE.UNFILLED
		__stats.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_stats() -> RoleStats:
		__stats.value = RoleStats.new()
		return __stats.value
	
	var __items: PBField
	func get_items() -> Array[ItemData]:
		return __items.value
	func clear_items() -> void:
		data[11].state = PB_SERVICE_STATE.UNFILLED
		__items.value.clear()
	func add_items() -> ItemData:
		var element = ItemData.new()
		__items.value.append(element)
		return element
	
	var __created_at_ms: PBField
	func has_created_at_ms() -> bool:
		if __created_at_ms.value != null:
			return true
		return false
	func get_created_at_ms() -> int:
		return __created_at_ms.value
	func clear_created_at_ms() -> void:
		data[12].state = PB_SERVICE_STATE.UNFILLED
		__created_at_ms.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_created_at_ms(value : int) -> void:
		__created_at_ms.value = value
	
	var __last_login_ms: PBField
	func has_last_login_ms() -> bool:
		if __last_login_ms.value != null:
			return true
		return false
	func get_last_login_ms() -> int:
		return __last_login_ms.value
	func clear_last_login_ms() -> void:
		data[13].state = PB_SERVICE_STATE.UNFILLED
		__last_login_ms.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_last_login_ms(value : int) -> void:
		__last_login_ms.value = value
	
	var __play_time_s: PBField
	func has_play_time_s() -> bool:
		if __play_time_s.value != null:
			return true
		return false
	func get_play_time_s() -> int:
		return __play_time_s.value
	func clear_play_time_s() -> void:
		data[14].state = PB_SERVICE_STATE.UNFILLED
		__play_time_s.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_play_time_s(value : int) -> void:
		__play_time_s.value = value
	
	var __data_version: PBField
	func has_data_version() -> bool:
		if __data_version.value != null:
			return true
		return false
	func get_data_version() -> int:
		return __data_version.value
	func clear_data_version() -> void:
		data[15].state = PB_SERVICE_STATE.UNFILLED
		__data_version.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_data_version(value : int) -> void:
		__data_version.value = value
	
	var __fight_mode: PBField
	func has_fight_mode() -> bool:
		if __fight_mode.value != null:
			return true
		return false
	func get_fight_mode() -> bool:
		return __fight_mode.value
	func clear_fight_mode() -> void:
		data[16].state = PB_SERVICE_STATE.UNFILLED
		__fight_mode.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_fight_mode(value : bool) -> void:
		__fight_mode.value = value
	
	var __native_place: PBField
	func has_native_place() -> bool:
		if __native_place.value != null:
			return true
		return false
	func get_native_place() -> int:
		return __native_place.value
	func clear_native_place() -> void:
		data[17].state = PB_SERVICE_STATE.UNFILLED
		__native_place.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_native_place(value : int) -> void:
		__native_place.value = value
	
	var __next_item_id: PBField
	func has_next_item_id() -> bool:
		if __next_item_id.value != null:
			return true
		return false
	func get_next_item_id() -> int:
		return __next_item_id.value
	func clear_next_item_id() -> void:
		data[18].state = PB_SERVICE_STATE.UNFILLED
		__next_item_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_next_item_id(value : int) -> void:
		__next_item_id.value = value
	
	var __money: PBField
	func has_money() -> bool:
		if __money.value != null:
			return true
		return false
	func get_money() -> int:
		return __money.value
	func clear_money() -> void:
		data[19].state = PB_SERVICE_STATE.UNFILLED
		__money.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_money(value : int) -> void:
		__money.value = value
	
	var __bank_money: PBField
	func has_bank_money() -> bool:
		if __bank_money.value != null:
			return true
		return false
	func get_bank_money() -> int:
		return __bank_money.value
	func clear_bank_money() -> void:
		data[20].state = PB_SERVICE_STATE.UNFILLED
		__bank_money.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_bank_money(value : int) -> void:
		__bank_money.value = value
	
	var __skills: PBField
	func get_skills() -> Array[RoleSkill]:
		return __skills.value
	func clear_skills() -> void:
		data[21].state = PB_SERVICE_STATE.UNFILLED
		__skills.value.clear()
	func add_skills() -> RoleSkill:
		var element = RoleSkill.new()
		__skills.value.append(element)
		return element
	
	var __revive_map: PBField
	func has_revive_map() -> bool:
		if __revive_map.value != null:
			return true
		return false
	func get_revive_map() -> int:
		return __revive_map.value
	func clear_revive_map() -> void:
		data[22].state = PB_SERVICE_STATE.UNFILLED
		__revive_map.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_revive_map(value : int) -> void:
		__revive_map.value = value
	
	var __revive_pos: PBField
	func has_revive_pos() -> bool:
		if __revive_pos.value != null:
			return true
		return false
	func get_revive_pos() -> Vec2:
		return __revive_pos.value
	func clear_revive_pos() -> void:
		data[23].state = PB_SERVICE_STATE.UNFILLED
		__revive_pos.value = DEFAULT_VALUES_3[PB_DATA_TYPE.MESSAGE]
	func new_revive_pos() -> Vec2:
		__revive_pos.value = Vec2.new()
		return __revive_pos.value
	
	var __revive_ref: PBField
	func has_revive_ref() -> bool:
		if __revive_ref.value != null:
			return true
		return false
	func get_revive_ref() -> int:
		return __revive_ref.value
	func clear_revive_ref() -> void:
		data[24].state = PB_SERVICE_STATE.UNFILLED
		__revive_ref.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_revive_ref(value : int) -> void:
		__revive_ref.value = value
	
	var __faction_last: PBField
	func has_faction_last() -> bool:
		if __faction_last.value != null:
			return true
		return false
	func get_faction_last() -> int:
		return __faction_last.value
	func clear_faction_last() -> void:
		data[25].state = PB_SERVICE_STATE.UNFILLED
		__faction_last.value = DEFAULT_VALUES_3[PB_DATA_TYPE.INT32]
	func set_faction_last(value : int) -> void:
		__faction_last.value = value
	
	var __faction_count: PBField
	func has_faction_count() -> bool:
		if __faction_count.value != null:
			return true
		return false
	func get_faction_count() -> int:
		return __faction_count.value
	func clear_faction_count() -> void:
		data[26].state = PB_SERVICE_STATE.UNFILLED
		__faction_count.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_faction_count(value : int) -> void:
		__faction_count.value = value
	
	var __pk_state: PBField
	func has_pk_state() -> bool:
		if __pk_state.value != null:
			return true
		return false
	func get_pk_state() -> int:
		return __pk_state.value
	func clear_pk_state() -> void:
		data[27].state = PB_SERVICE_STATE.UNFILLED
		__pk_state.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_pk_state(value : int) -> void:
		__pk_state.value = value
	
	var __pk_value: PBField
	func has_pk_value() -> bool:
		if __pk_value.value != null:
			return true
		return false
	func get_pk_value() -> int:
		return __pk_value.value
	func clear_pk_value() -> void:
		data[28].state = PB_SERVICE_STATE.UNFILLED
		__pk_value.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_pk_value(value : int) -> void:
		__pk_value.value = value
	
	var __pk_locked: PBField
	func has_pk_locked() -> bool:
		if __pk_locked.value != null:
			return true
		return false
	func get_pk_locked() -> bool:
		return __pk_locked.value
	func clear_pk_locked() -> void:
		data[29].state = PB_SERVICE_STATE.UNFILLED
		__pk_locked.value = DEFAULT_VALUES_3[PB_DATA_TYPE.BOOL]
	func set_pk_locked(value : bool) -> void:
		__pk_locked.value = value
	
	var __lead_exp: PBField
	func has_lead_exp() -> bool:
		if __lead_exp.value != null:
			return true
		return false
	func get_lead_exp() -> int:
		return __lead_exp.value
	func clear_lead_exp() -> void:
		data[30].state = PB_SERVICE_STATE.UNFILLED
		__lead_exp.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_lead_exp(value : int) -> void:
		__lead_exp.value = value
	
	var __lead_level: PBField
	func has_lead_level() -> bool:
		if __lead_level.value != null:
			return true
		return false
	func get_lead_level() -> int:
		return __lead_level.value
	func clear_lead_level() -> void:
		data[31].state = PB_SERVICE_STATE.UNFILLED
		__lead_level.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_lead_level(value : int) -> void:
		__lead_level.value = value
	
	var __task_values: PBField
	func get_task_values() -> Array[RoleTaskValue]:
		return __task_values.value
	func clear_task_values() -> void:
		data[32].state = PB_SERVICE_STATE.UNFILLED
		__task_values.value.clear()
	func add_task_values() -> RoleTaskValue:
		var element = RoleTaskValue.new()
		__task_values.value.append(element)
		return element
	
	var __events: PBField
	func get_events() -> Array[RolePlayerEvent]:
		return __events.value
	func clear_events() -> void:
		data[33].state = PB_SERVICE_STATE.UNFILLED
		__events.value.clear()
	func add_events() -> RolePlayerEvent:
		var element = RolePlayerEvent.new()
		__events.value.append(element)
		return element
	
	func _to_string() -> String:
		return PBPacker.message_to_string(data)
		
	func to_bytes() -> PackedByteArray:
		return PBPacker.pack_message(data)
		
	func from_bytes(bytes : PackedByteArray, offset : int = 0, limit : int = -1) -> int:
		var cur_limit = bytes.size()
		if limit != -1:
			cur_limit = limit
		var result = PBPacker.unpack_message(data, bytes, offset, cur_limit)
		if result == cur_limit:
			if PBPacker.check_required(data):
				if limit == -1:
					return PB_ERR.NO_ERRORS
			else:
				return PB_ERR.REQUIRED_FIELDS
		elif limit == -1 && result > 0:
			return PB_ERR.PARSE_INCOMPLETE
		return result
	
################ USER DATA END #################
