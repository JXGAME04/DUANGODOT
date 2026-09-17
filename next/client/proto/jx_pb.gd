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
		
		__faction = PBField.new("faction", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 6, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
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
		__faction.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
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
	REVIVE = 4
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
	
class ChatReq:
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
	
class RoleItem:
	extends RefCounted
	func _init():
		var service
		
		__item_uid = PBField.new("item_uid", PB_DATA_TYPE.UINT64, PB_RULE.OPTIONAL, 1, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64])
		service = PBServiceField.new()
		service.field = __item_uid
		data[__item_uid.tag] = service
		
		__template_id = PBField.new("template_id", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 2, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __template_id
		data[__template_id.tag] = service
		
		__count = PBField.new("count", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 3, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __count
		data[__count.tag] = service
		
		__slot = PBField.new("slot", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 4, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __slot
		data[__slot.tag] = service
		
		__container = PBField.new("container", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 5, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
		service = PBServiceField.new()
		service.field = __container
		data[__container.tag] = service
		
	var data = {}
	
	var __item_uid: PBField
	func has_item_uid() -> bool:
		if __item_uid.value != null:
			return true
		return false
	func get_item_uid() -> int:
		return __item_uid.value
	func clear_item_uid() -> void:
		data[1].state = PB_SERVICE_STATE.UNFILLED
		__item_uid.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT64]
	func set_item_uid(value : int) -> void:
		__item_uid.value = value
	
	var __template_id: PBField
	func has_template_id() -> bool:
		if __template_id.value != null:
			return true
		return false
	func get_template_id() -> int:
		return __template_id.value
	func clear_template_id() -> void:
		data[2].state = PB_SERVICE_STATE.UNFILLED
		__template_id.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_template_id(value : int) -> void:
		__template_id.value = value
	
	var __count: PBField
	func has_count() -> bool:
		if __count.value != null:
			return true
		return false
	func get_count() -> int:
		return __count.value
	func clear_count() -> void:
		data[3].state = PB_SERVICE_STATE.UNFILLED
		__count.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_count(value : int) -> void:
		__count.value = value
	
	var __slot: PBField
	func has_slot() -> bool:
		if __slot.value != null:
			return true
		return false
	func get_slot() -> int:
		return __slot.value
	func clear_slot() -> void:
		data[4].state = PB_SERVICE_STATE.UNFILLED
		__slot.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_slot(value : int) -> void:
		__slot.value = value
	
	var __container: PBField
	func has_container() -> bool:
		if __container.value != null:
			return true
		return false
	func get_container() -> int:
		return __container.value
	func clear_container() -> void:
		data[5].state = PB_SERVICE_STATE.UNFILLED
		__container.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
	func set_container(value : int) -> void:
		__container.value = value
	
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
		
		__faction = PBField.new("faction", PB_DATA_TYPE.UINT32, PB_RULE.OPTIONAL, 8, true, DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32])
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
		
		var __items_default: Array[RoleItem] = []
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
		__faction.value = DEFAULT_VALUES_3[PB_DATA_TYPE.UINT32]
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
	func get_items() -> Array[RoleItem]:
		return __items.value
	func clear_items() -> void:
		data[11].state = PB_SERVICE_STATE.UNFILLED
		__items.value.clear()
	func add_items() -> RoleItem:
		var element = RoleItem.new()
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
