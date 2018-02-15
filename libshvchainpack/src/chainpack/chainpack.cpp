#include "chainpack.h"

#include "exception.h"
//#include "../core/log.h"

#include <iostream>
#include <cassert>
#include <limits>

namespace shv {
namespace chainpack {
#if 0
namespace {
/*
template<typename T>
int significant_bits_part_length(T n)
{
	constexpr int bitlen = sizeof(T) * 8;
	constexpr T mask = T{1} << (bitlen - 1);
	int len = bitlen;
	for (; n && !(n & mask); --len) {
		n <<= 1;
	}
	return n? len: 0;
}

// number of bytes needed to encode bit_len
int bytes_needed(int bit_len)
{
	int cnt;
	if(bit_len <= 28)
		cnt = (bit_len - 1) / 7 + 1;
	else
		cnt = (bit_len - 1) / 8 + 2;
	return cnt;
}
*/
/* UInt
 0 ...  7 bits  1  byte  |0|x|x|x|x|x|x|x|<-- LSB
 8 ... 14 bits  2  bytes |1|0|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
15 ... 21 bits  3  bytes |1|1|0|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
22 ... 28 bits  4  bytes |1|1|1|0|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
29+       bits  5+ bytes |1|1|1|1|n|n|n|n| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| ... <-- LSB
                    n ==  0 ->  4 bytes number (32 bit number)
                    n ==  1 ->  5 bytes number
                    n == 14 -> 18 bytes number
                    n == 15 -> for future (number of bytes will be specified in next byte)
*/
/*
template<typename T>
void writeData_int_helper(std::ostream &out, T num, int bit_len)
{
	int byte_cnt = bytes_needed(bit_len);
	uint8_t bytes[byte_cnt];
	for (int i = byte_cnt-1; i >= 0; --i) {
		uint8_t r = num & 255;
		bytes[i] = r;
		num = num >> 8;
	}

	uint8_t &head = bytes[0];
	if(bit_len <= 28) {
		uint8_t mask = 0xf0 << (4 - byte_cnt);
		head = head & ~mask;
		mask <<= 1;
		head = head | mask;
	}
	else {
		head = 0xf0 | (byte_cnt - 5);
	}

	for (int i = 0; i < byte_cnt; ++i) {
		uint8_t r = bytes[i];
		out << r;
	}
}

template<typename T>
void writeData_UInt(std::ostream &out, T num)
{
	constexpr int UINT_BYTES_MAX = 18;
	if(sizeof(num) > UINT_BYTES_MAX)
		SHVCHP_EXCEPTION("writeData_UInt: value too big to pack!");

	int bitlen = significant_bits_part_length(num);
	writeData_int_helper<T>(out, num, bitlen);
}
*/
template<typename T>
T readData_UInt(std::istream &data, int *pbitlen = nullptr)
{
	T num = 0;
	int bitlen = 0;
	do {
		if(data.eof())
			break;
		uint8_t head = data.get();

		int bytes_to_read_cnt;
		if     ((head & 128) == 0) {bytes_to_read_cnt = 0; num = head & 127; bitlen = 7;}
		else if((head &  64) == 0) {bytes_to_read_cnt = 1; num = head & 63; bitlen = 6 + 8;}
		else if((head &  32) == 0) {bytes_to_read_cnt = 2; num = head & 31; bitlen = 5 + 2*8;}
		else if((head &  16) == 0) {bytes_to_read_cnt = 3; num = head & 15; bitlen = 4 + 3*8;}
		else {
			bytes_to_read_cnt = (head & 0xf) + 4;
			bitlen = bytes_to_read_cnt * 8;
		}

		for (int i = 0; i < bytes_to_read_cnt; ++i) {
			if(data.eof()) {
				bitlen = 0;
				break;
			}
			uint8_t r = data.get();
			num = (num << 8) + r;
		};
	} while(false);
	if(pbitlen)
		*pbitlen = bitlen;
	return num;
}

/*
 0 ...  7 bits  1  byte  |0|s|x|x|x|x|x|x|<-- LSB
 8 ... 14 bits  2  bytes |1|0|s|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
15 ... 21 bits  3  bytes |1|1|0|s|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
22 ... 28 bits  4  bytes |1|1|1|0|s|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
29+       bits  5+ bytes |1|1|1|1|n|n|n|n| |s|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| ... <-- LSB
                    n ==  0 ->  4 bytes number (32 bit number)
                    n ==  1 ->  5 bytes number
                    n == 14 -> 18 bytes number
                    n == 15 -> for future (number of bytes will be specified in next byte)
*/
/*
// return max bit length >= bit_len, which can be encoded by same number of bytes
int expand_bit_len(int bit_len)
{
	int ret;
	int byte_cnt = bytes_needed(bit_len);
	if(bit_len <= 28) {
		ret = byte_cnt * (8 - 1) - 1;
	}
	else {
		ret = (byte_cnt - 1) * 8 - 1;
	}
	return ret;
}

template<typename T>
void writeData_Int(std::ostream &out, T snum)
{
	using UT = typename std::make_unsigned<T>::type;
	UT num = snum < 0? -snum: snum;
	bool neg = snum < 0? true: false;

	int bitlen = significant_bits_part_length(num);
	bitlen++; // add sign bit
	if(neg) {
		int sign_pos = expand_bit_len(bitlen);
		UT sign_bit_mask = UT{1} << sign_pos;
		num |= sign_bit_mask;
	}
	writeData_int_helper(out, num, bitlen);
}
*/
template<typename T>
T readData_Int(std::istream &data)
{
	int bitlen;
	using UT = typename std::make_unsigned<T>::type;
	UT num = readData_UInt<UT>(data, &bitlen);
	UT sign_bit_mask = UT{1} << (bitlen - 1);
	bool neg = num & sign_bit_mask;
	T snum = num;
	if(neg) {
		snum &= ~sign_bit_mask;
		snum = -snum;
	}
	return snum;
}

double readData_Double(std::istream &data)
{
	union U {uint64_t n; double d;} u;
	u.n = 0;
	int shift = 0;
	for (size_t i = 0; i < sizeof(u.n); ++i) {
		uint8_t r = data.get();
		uint64_t n1 = r;
		n1 <<= shift;
		shift += 8;
		u.n += n1;
	}
	return u.d;
}
/*
void writeData_Double(std::ostream &out, double d)
{
	union U {uint64_t n; double d;} u;
	assert(sizeof(u.n) == sizeof(u.d));
	u.d = d;
	for (size_t i = 0; i < sizeof(u.n); ++i) {
		uint8_t r = u.n & 255;
		u.n = u.n >> 8;
		out << r;
	}
}
*/
RpcValue::Decimal readData_Decimal(std::istream &data)
{
	RpcValue::Int mant = readData_Int<RpcValue::Int>(data);
	int prec = readData_Int<RpcValue::Int>(data);
	return RpcValue::Decimal(mant, prec);
}
/*
void writeData_Decimal(std::ostream &out, const RpcValue::Decimal &d)
{
	writeData_Int(out, d.mantisa());
	writeData_Int(out, d.precision());
}

template<typename T>
void writeData_Blob(std::ostream &out, const T &blob)
{
	using S = typename T::size_type;
	S l = blob.length();
	writeData_UInt<S>(out, l);
	//out.reserve(out.size() + l);
	for (S i = 0; i < l; ++i)
		out << (uint8_t)blob[i];
}
*/
template<typename T>
T readData_Blob(std::istream &data)
{
	using S = typename T::size_type;
	unsigned int len = readData_UInt<unsigned int>(data);
	T ret;
	//ret.reserve(len);
	for (S i = 0; i < len; ++i)
		ret += data.get();
	return ret;
}
/*
void writeData_DateTime(std::ostream &out, const RpcValue::DateTime &dt)
{
	int64_t msecs = dt.msecsSinceEpoch() - RpcValue::DateTime::SHV_EPOCH_MSEC;
	int offset = (dt.offsetFromUtc() / 15) & 0b01111111;
	int ms = msecs % 1000;
	if(ms == 0)
		msecs /= 1000;
	if(offset != 0) {
		msecs <<= 7;
		msecs |= offset;
	}
	msecs <<= 2;
	if(offset != 0)
		msecs |= 1;
	if(ms == 0)
		msecs |= 2;
	writeData_Int(out, msecs);
}
*/
RpcValue::DateTime readData_DateTimeEpoch(std::istream &data)
{
	RpcValue::DateTime dt = RpcValue::DateTime::fromMSecsSinceEpoch(readData_Int<int64_t>(data));
	return dt;
}

RpcValue::DateTime readData_DateTime(std::istream &data)
{
	int64_t d = readData_Int<int64_t>(data);
	int8_t offset = 0;
	bool has_tz_offset = d & 1;
	bool has_not_msec = d & 2;
	d >>= 2;
	if(has_tz_offset) {
		offset = d & 0b01111111;
		offset <<= 1;
		offset >>= 1;
		d >>= 7;
	}
	if(has_not_msec)
		d *= 1000;
	d += RpcValue::DateTime::SHV_EPOCH_MSEC;
	RpcValue::DateTime dt = RpcValue::DateTime::fromMSecsSinceEpoch(d, offset * (int)15);
	return dt;
}
/*
void writeData_DateTimeUtc(std::ostream &out, const RpcValue::DateTime &dt)
{
	int64_t msecs = dt.msecsSinceEpoch() - RpcValue::DateTime::SHV_EPOCH_MSEC;
	writeData_Int(out, msecs);
}

RpcValue::DateTime readData_DateTimeUtc(std::istream &data)
{
	int64_t d = readData_Int<int64_t>(data);
	d += RpcValue::DateTime::SHV_EPOCH_MSEC;
	RpcValue::DateTime dt = RpcValue::DateTime::fromMSecsSinceEpoch(d);
	return dt;
}
*/
} // namespace

ChainPack::TypeInfo::Enum ChainPack::typeToTypeInfo(RpcValue::Type type)
{
	switch (type) {
	case RpcValue::Type::Invalid:
		SHVCHP_EXCEPTION("There is no type info for type Invalid");
	case RpcValue::Type::Array:
		SHVCHP_EXCEPTION("There is no type info for type Array");
	case RpcValue::Type::Null: return TypeInfo::Null;
	case RpcValue::Type::UInt: return TypeInfo::UInt;
	case RpcValue::Type::Int: return TypeInfo::Int;
	case RpcValue::Type::Double: return TypeInfo::Double;
	case RpcValue::Type::Bool: return TypeInfo::Bool;
	case RpcValue::Type::Blob: return TypeInfo::Blob;
	case RpcValue::Type::String: return TypeInfo::String;
	case RpcValue::Type::List: return TypeInfo::List;
	case RpcValue::Type::Map: return TypeInfo::Map;
	case RpcValue::Type::IMap: return TypeInfo::IMap;
	//case RpcValue::Type::DateTimeEpoch: return TypeInfo::DateTimeEpoch;
	case RpcValue::Type::DateTime: return TypeInfo::DateTime;
	case RpcValue::Type::Decimal: return TypeInfo::Decimal;
	}
	SHVCHP_EXCEPTION("Unknown RpcValue::Type!");
	return TypeInfo::INVALID; // just to remove mingw warning
}
#endif
RpcValue::Type ChainPack::typeInfoToType(ChainPack::TypeInfo::Enum type_info)
{
	switch (type_info) {
	case ChainPack::TypeInfo::Null: return RpcValue::Type::Null;
	case ChainPack::TypeInfo::UInt: return RpcValue::Type::UInt;
	case ChainPack::TypeInfo::Int: return RpcValue::Type::Int;
	case ChainPack::TypeInfo::Double: return RpcValue::Type::Double;
	case ChainPack::TypeInfo::Bool: return RpcValue::Type::Bool;
	case ChainPack::TypeInfo::Blob: return RpcValue::Type::Blob;
	case ChainPack::TypeInfo::String: return RpcValue::Type::String;
	case ChainPack::TypeInfo::DateTimeEpoch: return RpcValue::Type::DateTime; // deprecated
	case ChainPack::TypeInfo::DateTime: return RpcValue::Type::DateTime;
	case ChainPack::TypeInfo::List: return RpcValue::Type::List;
	case ChainPack::TypeInfo::Map: return RpcValue::Type::Map;
	case ChainPack::TypeInfo::IMap: return RpcValue::Type::IMap;
	//case ChainPackProtocol::TypeInfo::MetaIMap: return RpcValue::Type::MetaIMap;
	case ChainPack::TypeInfo::Decimal: return RpcValue::Type::Decimal;
	default:
		SHVCHP_EXCEPTION(std::string("There is type for type info ") + ChainPack::TypeInfo::name(type_info));
	}
}

const char *ChainPack::TypeInfo::name(ChainPack::TypeInfo::Enum e)
{
	switch (e) {
	case INVALID: return "INVALID";
	case FALSE: return "FALSE";
	case TRUE: return "TRUE";

	case Null: return "Null";
	case UInt: return "UInt";
	case Int: return "Int";
	case Double: return "Double";
	case Bool: return "Bool";
	case Blob: return "Blob";
	case String: return "String";
	case List: return "List";
	case Map: return "Map";
	case IMap: return "IMap";
	case DateTimeEpoch: return "DateTimeEpoch";
	case DateTime: return "DateTime";
	case MetaIMap: return "MetaIMap";
	case MetaSMap: return "MetaSMap";
	case Decimal: return "Decimal";
		/*
	case Null_Array: return "Null_Array";
	case UInt_Array: return "UInt_Array";
	case Int_Array: return "Int_Array";
	case Double_Array: return "Double_Array";
	case Bool_Array: return "Bool_Array";
	case Blob_Array: return "Blob_Array";
	case String_Array: return "String_Array";
	case DateTimeEpoch_Array: return "DateTimeEpoch_Array_NOT_USED";
	case DateTimeUtc_Array: return "DateTimeUtc_Array_NOT_USED";
	case DateTimeTZ_Array: return "DateTimeTZ_Array";
	case List_Array: return "List_Array";
	case Map_Array: return "Map_Array";
	case IMap_Array: return "IMap_Array";
	case MetaIMap_Array: return "MetaIMap_Array";
	case Decimal_Array: return "Decimal_Array";
	*/
	case TERM: return "TERM";
	}
	//SHVCHP_EXCEPTION("Unknown TypeInfo: " + Utils::toString((int)e));
	return "";
}
/*
void ChainPack::writeData_Array(std::ostream &out, const RpcValue::Array &array)
{
	unsigned size = array.size();
	writeData_UInt(out, size);
	for (unsigned i = 0; i < size; ++i) {
		const RpcValue &cp = array.valueAt(i);
		writeData(out, cp);
	}
}

RpcValue::Array ChainPack::readData_Array(ChainPack::TypeInfo::Enum array_type_info, std::istream &data)
{
	RpcValue::Type type = typeInfoToType(array_type_info);
	RpcValue::Array ret(type);
	RpcValue::UInt size = readData_UInt<RpcValue::UInt>(data);
	ret.reserve(size);
	for (unsigned i = 0; i < size; ++i) {
		RpcValue cp = readData(array_type_info, false, data);
		ret.push_back(RpcValue::Array::makeElement(cp));
	}
	return ret;
}

void ChainPack::writeData_List(std::ostream &out, const RpcValue::List &list)
{
	unsigned size = list.size();
	//write_UIntData(out, size);
	for (unsigned i = 0; i < size; ++i) {
		const RpcValue &cp = list.at(i);
		write(out, cp);
	}
	out << (uint8_t)TypeInfo::TERM;
}

RpcValue::List ChainPack::readData_List(std::istream &data)
{
	RpcValue::List lst;
	while(true) {
		int b = data.peek();
		if(b < 0)
			SHVCHP_EXCEPTION("Unexpected EOF!");
		if(b == ChainPack::TypeInfo::TERM) {
			data.get();
			break;
		}
		RpcValue cp = read(data);
		lst.push_back(cp);
	}
	return lst;
}

void ChainPack::writeData_Map(std::ostream &out, const RpcValue::Map &map)
{
	//unsigned size = map.size();
	//write_UIntData(out, size);
	for (const auto &kv : map) {
		writeMapElement(out, kv.first, kv.second);
	}
	out << (uint8_t)TypeInfo::TERM;
}

RpcValue::Map ChainPack::readData_Map(std::istream &data)
{
	RpcValue::Map ret;
	while(true) {
		int b = data.peek();
		if(b < 0)
			SHVCHP_EXCEPTION("Unexpected EOF!");
		if(b == ChainPack::TypeInfo::TERM) {
			data.get();
			break;
		}
		RpcValue::String key = readData_Blob<RpcValue::String>(data);
		RpcValue cp = read(data);
		ret[key] = cp;
	}
	return ret;
}

void ChainPack::writeData_IMap(std::ostream &out, const RpcValue::IMap &map)
{
	//unsigned size = map.size();
	//write_UIntData(out, size);
	for (const auto &kv : map) {
		writeMapElement(out, kv.first, kv.second);
	}
	out << (uint8_t)TypeInfo::TERM;
}

RpcValue::IMap ChainPack::readData_IMap(std::istream &data)
{
	RpcValue::IMap ret;
	while(true) {
		int b = data.peek();
		if(b == ChainPack::TypeInfo::TERM) {
			data.get();
			break;
		}
		RpcValue::UInt key = readData_UInt<RpcValue::UInt>(data);
		RpcValue cp = read(data);
		ret[key] = cp;
	}
	return ret;
}

int ChainPackProtocol::write(std::ostream &out, const RpcValue &val)
{
	std::ostream::pos_type len = out.tellp();
	if(!val.isValid()) {
		if(WRITE_INVALID_AS_NULL)
			return write(out, RpcValue(nullptr));
		else
			SHVCHP_EXCEPTION("Cannot serialize invalid ChainPack.");
	}
	else {
		writeMetaData(out, val.metaData());
		if(!writeTypeInfo(out, val))
			writeData(out, val);
		return (out.tellp() - len);
	}
}

void ChainPack::writeMetaData(std::ostream &out, const RpcValue::MetaData &meta_data)
{
	if(!meta_data.isEmpty()) {
		const RpcValue::IMap &cim = meta_data.iValues();
		if(!cim.empty()) {
			out << (uint8_t)ChainPack::TypeInfo::MetaIMap;
			writeData_IMap(out, cim);
		}
		const RpcValue::Map &csm = meta_data.sValues();
		if(!csm.empty()) {
			out << (uint8_t)ChainPack::TypeInfo::MetaSMap;
			writeData_Map(out, csm);
		}
	}
}

bool ChainPack::writeTypeInfo(std::ostream &out, const RpcValue &pack)
{
	if(!pack.isValid())
		SHVCHP_EXCEPTION("Cannot serialize invalid ChainPack.");
	bool ret = false;
	RpcValue::Type type = pack.type();
	int t = TypeInfo::INVALID;
	if(type == RpcValue::Type::Bool) {
		t = pack.toBool()? ChainPack::TypeInfo::TRUE: ChainPack::TypeInfo::FALSE;
		ret = true;
	}
	else if(type == RpcValue::Type::UInt) {
		auto n = pack.toUInt();
		if(n < 64) {
			/// TinyUInt
			t = n;
			ret = true;
		}
	}
	else if(type == RpcValue::Type::Int) {
		auto n = pack.toInt();
		if(n >= 0 && n < 64) {
			/// TinyInt
			t = 64 + n;
			ret = true;
		}
	}
	else if(type == RpcValue::Type::Array) {
		t = typeToTypeInfo(pack.arrayType());
		t |= ARRAY_FLAG_MASK;
	}
	if(t == TypeInfo::INVALID) {
		t = typeToTypeInfo(pack.type());
	}
	out << (uint8_t)t;
	return ret;
}

void ChainPack::writeData(std::ostream &out, const RpcValue &val)
{
	RpcValue::Type type = val.type();
	switch (type) {
	case RpcValue::Type::Null: break;
	case RpcValue::Type::Bool: out << (uint8_t)(val.toBool() ? 1 : 0); break;
	case RpcValue::Type::UInt: { RpcValue::UInt u = val.toUInt(); writeData_UInt(out, u); break; }
	case RpcValue::Type::Int: { RpcValue::Int n = val.toInt(); writeData_Int<RpcValue::Int>(out, n); break; }
	case RpcValue::Type::Double: writeData_Double(out, val.toDouble()); break;
	case RpcValue::Type::Decimal: writeData_Decimal(out, val.toDecimal()); break;
	case RpcValue::Type::DateTime: writeData_DateTime(out, val.toDateTime()); break;
	case RpcValue::Type::String: writeData_Blob(out, val.toString()); break;
	case RpcValue::Type::Blob: writeData_Blob(out, val.toBlob()); break;
	case RpcValue::Type::List: writeData_List(out, val.toList()); break;
	case RpcValue::Type::Array: writeData_Array(out, val.toArray()); break;
	case RpcValue::Type::Map: writeData_Map(out, val.toMap()); break;
	case RpcValue::Type::IMap: writeData_IMap(out, val.toIMap()); break;
	case RpcValue::Type::Invalid:
	//case RpcValue::Type::MetaIMap:
		SHVCHP_EXCEPTION("Internal error: attempt to write helper type directly. type: " + std::string(RpcValue::typeToName(type)));
	}
}

uint64_t ChainPack::readUIntData(std::istream &data, bool *ok)
{
	int bitlen;
	uint64_t ret = readData_UInt<uint64_t>(data, &bitlen);
	if(ok)
		*ok = (bitlen > 0);
	return ret;
}

void ChainPack::writeUIntData(std::ostream &out, uint64_t n)
{
	writeData_UInt(out, n);
}

RpcValue ChainPack::read(std::istream &data)
{
	RpcValue ret;
	RpcValue::MetaData meta_data = readMetaData(data);
	uint8_t type = data.get();
	if(type < 128) {
		if(type & 64) {
			// tiny Int
			RpcValue::Int n = type & 63;
			ret = RpcValue(n);
		}
		else {
			// tiny UInt
			RpcValue::UInt n = type & 63;
			ret = RpcValue(n);
		}
	}
	else if(type == ChainPack::TypeInfo::FALSE) {
		ret = RpcValue(false);
	}
	else if(type == ChainPack::TypeInfo::TRUE) {
		ret = RpcValue(true);
	}
	if(!ret.isValid()) {
		bool is_array = type & ARRAY_FLAG_MASK;
		type = type & ~ARRAY_FLAG_MASK;
		ret = readData((ChainPack::TypeInfo::Enum)type, is_array, data);
	}
	if(!meta_data.isEmpty())
		ret.setMetaData(std::move(meta_data));
	return ret;
}
*/
#if 0
RpcValue::MetaData ChainPack::readMetaData(std::istream &data)
{
	RpcValue::IMap imap;
	RpcValue::Map smap;
	while(true) {
		bool has_meta = true;
		uint8_t type_info = data.peek();
		switch(type_info) {
		/*
		case ChainPackProtocol::TypeInfo::META_TYPE_ID:  {
			data.get();
			RpcValue::UInt u = read_UIntData<RpcValue::UInt>(data);
			ret.setMetaTypeId(u);
			break;
		}
		case ChainPackProtocol::TypeInfo::META_TYPE_NAMESPACE_ID:  {
			data.get();
			RpcValue::UInt u = read_UIntData<RpcValue::UInt>(data);
			ret.setMetaTypeNameSpaceId(u);
			break;
		}
		*/
		case ChainPack::TypeInfo::MetaIMap:  {
			data.get();
			imap = readData_IMap(data);
			break;
		}
		case ChainPack::TypeInfo::MetaSMap:  {
			data.get();
			smap = readData_Map(data);
			break;
		}
		default:
			has_meta = false;
			break;
		}
		if(!has_meta)
			break;
	}
	return RpcValue::MetaData(std::move(imap), std::move(smap));
}

ChainPack::TypeInfo::Enum ChainPack::readTypeInfo(std::istream &data, RpcValue &meta, int &tiny_uint)
{
	uint8_t t = data.get();
	if(t & 128) {
		t = t & ~128;
		meta = read(data);
	}
	if(t >= 64) {
		/// TinyUInt
		t -= 64;
		tiny_uint = t;
		return ChainPack::TypeInfo::UInt;
	}
	else {
		ChainPack::TypeInfo::Enum type = (ChainPack::TypeInfo::Enum)t;
		return type;
	}
}

RpcValue ChainPack::readData(ChainPack::TypeInfo::Enum type, bool is_array, std::istream &data)
{
	RpcValue ret;
	if(is_array) {
		RpcValue::Array val = readData_Array(type, data);
		ret = RpcValue(val);
	}
	else {
		switch (type) {
		case ChainPack::TypeInfo::Null: { ret = RpcValue(nullptr); break; }
		case ChainPack::TypeInfo::UInt: { RpcValue::UInt u = readData_UInt<RpcValue::UInt>(data); ret = RpcValue(u); break; }
		case ChainPack::TypeInfo::Int: { RpcValue::Int i = readData_Int<RpcValue::Int>(data); ret = RpcValue(i); break; }
		case ChainPack::TypeInfo::Double: { double d = readData_Double(data); ret = RpcValue(d); break; }
		case ChainPack::TypeInfo::Decimal: { RpcValue::Decimal d = readData_Decimal(data); ret = RpcValue(d); break; }
		case ChainPack::TypeInfo::TRUE: { bool b = true; ret = RpcValue(b); break; }
		case ChainPack::TypeInfo::FALSE: { bool b = false; ret = RpcValue(b); break; }
		case ChainPack::TypeInfo::DateTimeEpoch: { RpcValue::DateTime val = readData_DateTimeEpoch(data); ret = RpcValue(val); break; }
		case ChainPack::TypeInfo::DateTime: { RpcValue::DateTime val = readData_DateTime(data); ret = RpcValue(val); break; }
		case ChainPack::TypeInfo::String: { RpcValue::String val = readData_Blob<RpcValue::String>(data); ret = RpcValue(val); break; }
		case ChainPack::TypeInfo::Blob: { RpcValue::Blob val = readData_Blob<RpcValue::Blob>(data); ret = RpcValue(val); break; }
		case ChainPack::TypeInfo::List: { RpcValue::List val = readData_List(data); ret = RpcValue(val); break; }
		case ChainPack::TypeInfo::Map: { RpcValue::Map val = readData_Map(data); ret = RpcValue(val); break; }
		case ChainPack::TypeInfo::IMap: { RpcValue::IMap val = readData_IMap(data); ret = RpcValue(val); break; }
		case ChainPack::TypeInfo::Bool: { uint8_t t = data.get(); ret = RpcValue(t != 0); break; }
		default:
			SHVCHP_EXCEPTION("Internal error: attempt to read helper type directly. type: " + Utils::toString(type) + " " + TypeInfo::name(type));
		}
	}
	return ret;
}
/*
void ChainPack::writeContainerBegin(std::ostream &out, const RpcValue::Type &container_type)
{
	switch (container_type) {
	case RpcValue::Type::List:
	case RpcValue::Type::Map:
	case RpcValue::Type::IMap:
	//case RpcValue::Type::MetaIMap:
	{
		TypeInfo::Enum t = typeToTypeInfo(container_type);
		out << (uint8_t)t;
		break;
	}
	default:
		SHVCHP_EXCEPTION("Cannot begin write container of type: " + std::string(RpcValue::typeToName(container_type)));
	}
}

void ChainPack::writeContainerEnd(std::ostream &out)
{
	out << (uint8_t)TypeInfo::TERM;
}

void ChainPack::writeListElement(std::ostream &out, const RpcValue &val)
{
	write(out, val);
}

void ChainPack::writeMapElement(std::ostream &out, const std::string &key, const RpcValue &val)
{
	writeData_Blob<RpcValue::String>(out, key);
	write(out, val);

}

void ChainPack::writeMapElement(std::ostream &out, const RpcValue::UInt &key, const RpcValue &val)
{
	writeData_UInt(out, key);
	write(out, val);
}

void ChainPack::writeArrayBegin(std::ostream &out, const RpcValue::Type &array_type, size_t array_size)
{
	int t = typeToTypeInfo(array_type);
	t |= ARRAY_FLAG_MASK;
	out << (uint8_t)t;
	writeData_UInt(out, array_size);
}
*/
#endif

}}
