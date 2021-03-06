import struct
import enum
import logging_config
import logging
from math import floor, trunc
from datetime import datetime, timezone, timedelta
from dateutil.tz import tzoffset
import meta

class uint(int):
	pass

class UtcAndTz:
	def __init__(s, dt: datetime, tz = 0):
		assert dt.utcoffset() == timedelta(0),    dt.utcoffset()
		s.dt = dt
		s.tz = tz
	def __repr__(s):
		return "UtcAndTz(" + s.dt.__repr__() + "," + s.tz.__repr__() + ")"
	def __eq__(s, x):
		return x.dt == s.dt and x.tz == s.tz

debug = logging.debug
ARRAY_FLAG_MASK = 64

SHV_EPOCH_MSEC = 1517529600000

class ChainpackException(Exception):
	pass

class ChainpackTypeException(ChainpackException):
	pass

class ChainpackDeserializationException(ChainpackException):
	pass


class MetaData(dict):
	pass


class TypeInfo(enum.IntFlag):
	INVALID = -1
	#/// types
	Null=128
	UInt=129
	Int=130
	Double=131
	Bool=132
	Blob=133
	String=134
	DateTimeEpoch=135#deprecated
	List=136
	Map=137
	IMap=138
	MetaIMap=139
	Decimal=140
	DateTime=141

	#/// arrays
	#// if bit 6 is set, then packed value is an Array of corresponding values
	Null_Array = Null | ARRAY_FLAG_MASK
	UInt_Array = UInt | ARRAY_FLAG_MASK
	Int_Array = Int | ARRAY_FLAG_MASK
	Double_Array = Double | ARRAY_FLAG_MASK
	Bool_Array = Bool | ARRAY_FLAG_MASK
	Blob_Array = Blob | ARRAY_FLAG_MASK
	String_Array = String | ARRAY_FLAG_MASK
	DateTime_Array = DateTimeEpoch | ARRAY_FLAG_MASK
	List_Array = List | ARRAY_FLAG_MASK
	Map_Array = Map | ARRAY_FLAG_MASK
	IMap_Array = IMap | ARRAY_FLAG_MASK
	MetaIMap_Array = MetaIMap | ARRAY_FLAG_MASK
	#/// auxiliary types used for optimization
	FALSE=253
	TRUE=254
	TERMINATION = 255


class Type(enum.IntFlag):
	INVALID = -1
	Null=134
	UInt=135
	Int=136
	Double=137
	Bool=138
	Blob=139
	String=140
	DateTime=141
	List=142
	Array=143
	Map=144
	IMap=145
	MetaIMap=146
	Decimal=147


def typeToTypeInfo(type: Type):
	if type == Type.INVALID:  raise Exception("There is no type info for type Invalid");
	if type == Type.Array:    raise Exception("There is no type info for type Array");
	if type == Type.Null:     return TypeInfo.Null;
	if type == Type.UInt:     return TypeInfo.UInt;
	if type == Type.Int:      return TypeInfo.Int;
	if type == Type.Double:   return TypeInfo.Double;
	if type == Type.Bool:     return TypeInfo.Bool;
	if type == Type.Blob:     return TypeInfo.Blob;
	if type == Type.String:   return TypeInfo.String;
	if type == Type.List:     return TypeInfo.List;
	if type == Type.Map:      return TypeInfo.Map;
	if type == Type.IMap:     return TypeInfo.IMap;
	if type == Type.DateTime: return TypeInfo.DateTime;
	if type == Type.MetaIMap: return TypeInfo.MetaIMap;
	raise Exception("Unknown RpcValue::Type!");

def typeInfoToType(type_info: TypeInfo) -> Type:
	if type_info == TypeInfo.Null:     return Type.Null;
	if type_info == TypeInfo.UInt:     return Type.UInt;
	if type_info == TypeInfo.Int:      return Type.Int;
	if type_info == TypeInfo.Double:   return Type.Double;
	if type_info == TypeInfo.Bool:     return Type.Bool;
	if type_info == TypeInfo.Blob:     return Type.Blob;
	if type_info == TypeInfo.String:   return Type.String;
	if type_info == TypeInfo.DateTime: return Type.DateTime;
	if type_info == TypeInfo.List:     return Type.List;
	if type_info == TypeInfo.Map:      return Type.Map;
	if type_info == TypeInfo.IMap:     return Type.IMap;
	if type_info == TypeInfo.MetaIMap: return Type.MetaIMap;
	raise Exception("There is no Type for TypeInfo %s"%(type_info));

def chainpackTypeFromPythonType(v):
	if isinstance(v, (datetime, UtcAndTz)):
		return Type.DateTime,
	t = type(v)
	if t == InvalidValue:return Type.INVALID,
	if t == type(None):  return Type.Null,
	if t == bool:        return Type.Bool,
	if t == uint:        return Type.UInt,
	if t == int:         return Type.Int, Type.UInt,
	if t == float:       return Type.Double,
	if t == str:         return Type.String,
	if t == list:        return Type.List, Type.Array
	if t == dict:        return Type.Map, Type.IMap
	if t in (bytearray, bytes): return Type.Blob,
	raise ChainpackTypeException("failed deducing chainpack type for python type %s"%t)

class InvalidValue():
	pass

invalid_value = InvalidValue()

class RpcValue():
	def __init__(s, value, t = None):
		if isinstance(value, RpcValueArray):
			raise Exception("must construct RpcValueArray")
		if type(value) == RpcValue:
			s._value = value._value
			s._type = value._type
			s._metaData = value._metaData
		else:
			s._metaData = {}
			if isinstance(value, list):
				s._value = []
				for i in value:
					s._value.append(RpcValue(i))
			elif isinstance(value, dict):
				s._value = {}
				for k,v in value.items():
					if t == Type.IMap:
						assert isinstance(k, int)
						s._value[k] = RpcValue(v)
					else:
						assert isinstance(k, str)
						s._value[k] = RpcValue(v)
			elif isinstance(value, enum.IntFlag):
				s._value = int(value)
			else:
				s._value = value

			if t == None:
				t = chainpackTypeFromPythonType(s._value)[0]
			s._type = t
		if s._type not in chainpackTypeFromPythonType(s._value):
			raise ChainpackTypeException("python type %s for value %s does not match chainpack type %s" % (type(s._value), s._value, s._type))

	def __eq__(s, x):
		return isinstance(x, RpcValue) and s.type == x.type and s.value == x.value and s._metaData == x._metaData

	def assertEquals(s, x):
		assert s.type == x.type
		assert s.value == x.value
		assert s._metaData == x._metaData

	def toPython(s):
		if isinstance(s._value, dict):
			r = {}
			for k,v in s._value.items():
				assert(isinstance(k, (str, int)))
				r[k] = v.toPython()
			return r
		elif isinstance(s._value, list):
			r = []
			for i in s._value:
				r.append(i.toPython())
			return r
		else:
			return s._value

	def __repr__(s):
		out = ""
		if len(s._metaData):
			out += '<@meta:' + str(s._metaData) + '@>'
		out += '<' + s._type._name_ + '>';
		out += str(type(s._value))
		out += s._value.__repr__()
		value_repr = s._value.__repr__()
		if isinstance(s._value, str):
			value_repr = '"' + value_repr[1:-1] + '"'
		return "RpcValue(" + out  + ")"
		#return value_repr

	def __len__(s):
		return len(s._value)

	@property
	def value(s):
		return s._value

	@property
	def type(s):
		return s._type

	def isValid(s):
		return s._type != Type.INVALID

	def toInt(s) -> int:
		assert s._type in [Type.Int, Type.UInt]
		return s._value

	def toUInt(s) -> uint:
		assert s._type in [Type.UInt]
		return uint(s._value)

	def toBool(s) -> bool:
		assert s._type in [Type.Bool]
		return s._value

	def toString(s):
		assert s._type in [Type.String]
		return s._value

	def isIMap(s):
		return s._type == Type.IMap

	def setMetaValue(s, tag, value):
		value = RpcValue(value)
		s._metaData[tag] = value


def optimizeRpcValueIntoType(pack: RpcValue) -> int:
		if (not pack.isValid()):
			raise ChainpackTypeException("Cannot serialize invalid ChainPack.");
		t: Type = pack._type
		if(t == Type.Bool) :
			return [TypeInfo.FALSE, TypeInfo.TRUE][pack.toBool()]
		elif t == Type.UInt:
			n = pack.toInt() # type: int
			if n in range(64):
				return n
		elif t == Type.Int:
			n = pack.toInt()
			if n in range(64):
				return n + 64
		elif t == Type.Null:
			return TypeInfo.Null
		return None


class ChainPackProtocol(bytearray):
	DOUBLE_FMT = '!d'

	def __init__(s, value=None):
		if type(value) == RpcValue:
			s.write(RpcValue(value))
		elif isinstance(value, (bytes, bytearray)):
			super().__init__(value)

	def __str__(s):
		return super().__repr__() + str([bin(x) for x in s])

	def add(s, x: bytearray):
		for i in x:
			s.append(i)

	def get(s):
		if (not len(s)):
			raise ChainpackDeserializationException("unexpected end of stream!");
		return s.pop(0)

	def peek(s):
		return s[0]

	def pop_front(s, size: int):
		for i in range(size):
			s.pop(0)

	def read(s):
		metadata = s.readMetaData();
		t: int = s.get();
		if t < 128:
			if(t & 64) :
				#// tiny Int
				n: int = t & 63;
				ret = RpcValue(n, Type.Int);
			else:
				#// tiny UInt
				n: int = t & 63;
				ret = RpcValue(n, Type.UInt);
		elif t in [TypeInfo.TRUE, TypeInfo.FALSE]:
			ret = RpcValue(t == TypeInfo.TRUE)
		else:
			is_array: bool = t & ARRAY_FLAG_MASK;
			type = t & ~ARRAY_FLAG_MASK;
			ret = s.readData(type, is_array);
		if len(metadata):
			ret._metaData = metadata
		return ret;

	def write(s, value: RpcValue) -> int:
		if(not value.isValid()):
			raise ChainpackTypeException("Cannot serialize invalid ChainPack.");
		x = ChainPackProtocol.pack(value)
		s.add(x)
		return len(x)

	@classmethod
	def pack(cls, value):
		#print("pack:", RpcValue(value))
		assert(value._type != TypeInfo.INVALID)
		out = ChainPackProtocol()
		out.writeMetaData(value._metaData);
		t = optimizeRpcValueIntoType(value)
		if t != None:
			out.append(t)
		else:
			if(value._type == Type.Array):
				t = typeToTypeInfo(value.element_type) | ARRAY_FLAG_MASK
			else:
				t = typeToTypeInfo(value._type)
			out.append(t)
			out.writeData(value)
		return out

	def readMetaData(s) -> MetaData:
		ret = MetaData()
		while True:
			type_info: int = s.peek();
			if type_info == TypeInfo.MetaIMap:
				s.pop(0)
				for k,v in s.readData_IMap().value.items():
					ret[k] = v
			else:
				break;
		return ret;

	def writeMetaData(s, md: MetaData):
		imap = RpcValue(md, Type.IMap)
		if len(imap):
			s.append(TypeInfo.MetaIMap)
			s.writeData_IMap(imap.value);

#	def readTypeInfo(s) -> (TypeInfo, RpcValue, int):
#		t: int = s.get()
#		meta = None
#		if(t & 128):
#			t = t & ~128;
#			meta = s.read();
#		if(t >= 64):
#			return TypeInfo.UInt, meta, t - 64;
#		else: return t, meta

	def readData(s, t: TypeInfo, is_array: bool) -> RpcValue:
		if(is_array):
			return s.readData_Array(t);
		else:
			if   t == TypeInfo.Null:     return RpcValue(None)
			elif t == TypeInfo.UInt:     return RpcValue(s.readData_UInt(), Type.UInt)
			elif t == TypeInfo.Int:      return RpcValue(s.readData_Int())
			elif t == TypeInfo.Double:   return RpcValue(s.read_fmt(s.DOUBLE_FMT))
			elif t == TypeInfo.TRUE:     return RpcValue(True)
			elif t == TypeInfo.FALSE:    return RpcValue(False)
			elif t == TypeInfo.DateTime: return RpcValue(s.read_DateTime())
			elif t == TypeInfo.String:   return RpcValue(s.readData_String())
			elif t == TypeInfo.Blob:     return RpcValue(s.read_Blob())
			elif t == TypeInfo.List:     return RpcValue(s.readData_List())
			elif t == TypeInfo.Map:      return RpcValue(s.readData_Map())
			elif t == TypeInfo.IMap:     return RpcValue(s.readData_IMap(), TypeInfo.IMap)
			elif t == TypeInfo.Bool:     return RpcValue(s.get() != b'\0')
			else: raise	ChainpackTypeException("Internal error: attempt to read meta type directly. type: " + str(t) + " " + t.name)

	def writeData(s, val: RpcValue):
		v = val.value
		t = val.type # type: Type
		if   t == Type.Null:     return
		elif t == Type.Bool:     s.append([b'0', b'1'][v])
		elif t == Type.UInt:     s.writeData_UInt(v)
		elif t == Type.Int:      s.writeData_Int(v)
		elif t == Type.Double:   s.write_fmt(s.DOUBLE_FMT, v)
		elif t == Type.DateTime: s.write_DateTime(v)
		elif t == Type.String:   s.writeData_String(v)
		elif t == Type.Blob:     s.write_Blob(v)
		elif t == Type.List:     s.writeData_List(v)
		elif t == Type.Array:    s.writeData_Array(val)
		elif t == Type.Map:      s.writeData_Map(v)
		elif t == Type.IMap:     s.writeData_IMap(v)
		elif t == Type.INVALID:  raise ChainpackTypeException("Internal error: attempt to write invalid type data")
		elif t == Type.MetaIMap: raise ChainpackTypeException("Internal error: attempt to write metatype directly")

	def read_fmt(s, fmt):
		size = struct.calcsize(fmt)
		r = struct.unpack(fmt, s[:8])[0]
		s.pop_front(size)
		return r

	def write_fmt(s, fmt, value):
		print(type(value))
		s.add(struct.pack(fmt, value))

	def writeData_List(s, v: list):
		for i in v:
			s.write(i)
		s.append(TypeInfo.TERMINATION)

	def readData_List(s) -> list:
		r = []
		while True:
			i = s.peek()
			if i == TypeInfo.TERMINATION:
				s.pop(0)
				break
			r.append(s.read())
		return r

	def write_Blob(s, b):
		assert type(b) in (bytearray, bytes)
		s.writeData_UInt(len(b))
		for i in b:
			s.append(i)

	def read_Blob(s):
		r = bytearray()
		for i in range(s.readData_UInt()):
			r.append(s.get())
		return r

	def writeData_String(s, v):
		b = v.encode('utf-8')
		s.write_Blob(b)

	def readData_String(s) -> str:
		return s.read_Blob().decode('utf-8')

	def write_DateTime(s, v):
		if isinstance(v, datetime):
			dt = v
			tz = 0
		elif isinstance(v, UtcAndTz):
			dt = v.dt
			tz = v.tz
		else:
			assert False, v
		out = round(dt.timestamp() * 1000) - SHV_EPOCH_MSEC
		has_millis = (out % 1000 != 0)
		if not has_millis:
			out = out // 1000
		if(tz != 0):
			out <<= 7;
			assert -64 <= tz <= 63
			if tz < 0:
				tz = (1 << 6) | (~(-1-tz) & 0b111111)
			out |= tz;
		out <<= 2;
		if(tz != 0):
			out |= 1;
		if not has_millis:
			out |= 2;
		s.writeData_Int(out)

	def read_DateTime(s):
		d = s.readData_Int()
		offset = 0;
		has_tz_offset = d & 1;
		has_not_msec = d & 2;
		d >>= 2;
		if(has_tz_offset):
			offset = d & 0b01111111;
			if offset & (1 << 6):
				offset -= (1 << 7)
			d >>= 7;
		if(has_not_msec):
			d *= 1000;
		d2 = d + SHV_EPOCH_MSEC
		d3 = d2 / 1000
		dt = datetime.utcfromtimestamp(d3)
		dt = dt.replace(tzinfo=timezone.utc)
		return UtcAndTz(dt, offset)

	def readData_IMap(s) -> RpcValue:
		ret = RpcValue({}, Type.IMap)
		#map_size: int = s.read_UIntData()
		#for i in range(map_size):
		while True:
			if s.peek() == TypeInfo.TERMINATION:
				s.pop(0)
				break
			key = s.readData_UInt()
			ret.value[key] = s.read()
		return ret

	def writeData_IMap(s, map: dict) -> None:
		assert type(map) == dict
		for k, v in map.items():
			if not isinstance(k, int) or k < 0:
				raise ChainpackTypeException('k.type != Type.UInt')
			s.writeData_UInt(k)
			s.write(v)
		s.append(TypeInfo.TERMINATION)

	def readData_Map(s) -> RpcValue:
		ret = RpcValue({}, Type.Map)
		while True:
			if s.peek() == TypeInfo.TERMINATION:
				s.pop(0)
				break
			key = s.readData_String()
			ret.value[key] = s.read()
		return ret

	def writeData_Map(s, map: dict) -> None:
		assert type(map) == dict
		for k, v in map.items():
			assert isinstance(k, str)
			s.writeData_String(k)
			s.write(v)
		s.append(TypeInfo.TERMINATION)

	@staticmethod
	def bytes_needed(bit_len: int) -> int:
		if (bit_len <= 28):
			return trunc((bit_len - 1) / 7) + 1;
		else:
			return trunc((bit_len - 1) / 8) + 2;

	@staticmethod
	def expand_bit_len(bit_len: int):
		byte_cnt = ChainPackProtocol.bytes_needed(bit_len);
		if(bit_len <= 28):
			return byte_cnt * (8 - 1) - 1;
		else:
			return (byte_cnt - 1) * 8 - 1;

	def writeData_UInt(s, num):
		assert num >= 0
		UINT_BYTES_MAX = 18;
		if num.bit_length() > UINT_BYTES_MAX * 8:
			SHV_EXCEPTION("writeData_UInt: value too big to pack!");
		s.writeData_int_helper(num, num.bit_length());

	def writeData_Int(s, snum):
		num = -snum if snum < 0 else snum
		neg = snum < 0
		bitlen = num.bit_length() + 1
		if (neg):
			sign_pos = s.expand_bit_len(bitlen);
			sign_bit_mask = 1 << sign_pos;
			num |= sign_bit_mask;
		s.writeData_int_helper(num, bitlen);

	def writeData_int_helper(s,	num, bit_len: int):
		byte_cnt = s.bytes_needed(bit_len);
		b = bytearray()
		for i in range(byte_cnt - 1, -1, -1):
			byte = num & 255
			b.insert(0, byte)
			num = num >> 8;
		head = b[0];
		if (bit_len <= 28):
			mask = (0xf0 << (4 - byte_cnt)) & 255;
			head = (head & ~mask) & 255;
			mask = (mask << 1) & 255;
			head = head | mask;
		else:
			head = 0xf0 | (byte_cnt - 5);
		b[0] = head & 255
		s.add(b)

	def readData_Int(s):
		num, bitlen = s._readData_UInt();
		sign_bit_mask = 1 << (bitlen - 1);
		neg = num & sign_bit_mask;
		snum = num;
		if (neg):
			snum &= ~sign_bit_mask;
			snum = -snum;
		return snum;

	def readData_UInt(s):
		return s._readData_UInt()[0]

	def _readData_UInt(s):
		head = s.get()
		num = 0
		if  ((head & 128) == 0): bytes_to_read_cnt = 0; num = head & 127; bitlen = 7;
		elif ((head & 64) == 0): bytes_to_read_cnt = 1; num = head & 63; bitlen = 6 + 8;
		elif ((head & 32) == 0): bytes_to_read_cnt = 2; num = head & 31; bitlen = 5 + 2 * 8;
		elif ((head & 16) == 0): bytes_to_read_cnt = 3; num = head & 15; bitlen = 4 + 3 * 8;
		else:
			bytes_to_read_cnt = (head & 0xf) + 4;
			bitlen = bytes_to_read_cnt * 8;
		for i in range(bytes_to_read_cnt):
			num = (num << 8) + s.get()
		return num, bitlen

	def readData_Array(s, item_type_info: TypeInfo):
		item_type: Type = typeInfoToType(item_type_info)
		ret = RpcValueArray(item_type)
		size: int = s.readData_UInt()
		for i in range(size):
			ret._value.append(s.readData(item_type_info, False))
		return ret

	def writeData_Array(s, array):
		assert isinstance(array, RpcValueArray)
		size = len(array._value)
		s.writeData_UInt(size);
		for i in array._value:
			assert i._type == array.element_type
			s.writeData(i);

	def readData_Decimal(s):
		mant = s.readData_Int();
		prec = readData_Int();
		return RpcValue((mant, prec), Type.Decimal);

	def writeData_Decimal(s, d):
		assert d._type == Type.Decimal
		assert type(d._value) == tuple
		s.writeData_Int(d._value[0]);
		s.writeData_Int(d._value[1]);


class RpcValueArray(RpcValue):
	def __init__(s, element_type, value = None):
		s.element_type = element_type
		if value == None:
			value = []
		super().__init__(value, Type.Array)

	def assertEquals(s, x):
		assert s.element_type == x.element_type
		super().assertEquals(x)

