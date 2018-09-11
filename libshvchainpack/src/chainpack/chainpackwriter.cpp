#include "chainpackwriter.h"
#include "cpon.h"
#include "exception.h"

#include "../../c/cchainpack.h"

#include <iostream>
#include <cmath>
#include <sstream>

#include <string.h>

namespace shv {
namespace chainpack {

void ChainPackWriter::writeUIntData(uint64_t n)
{
	cchainpack_pack_uint_data(&m_outCtx, n);
	flush();
}

size_t ChainPackWriter::write(const RpcValue &value)
{
	auto len = m_out.tellp();
	if(!value.metaData().isEmpty()) {
		write(value.metaData());
	}
	switch (value.type()) {
	case RpcValue::Type::Null: write_p(nullptr); break;
	case RpcValue::Type::UInt: write_p(value.toUInt64()); break;
	case RpcValue::Type::Int: write_p(value.toInt64()); break;
	case RpcValue::Type::Double: write_p(value.toDouble()); break;
	case RpcValue::Type::Bool: write_p(value.toBool()); break;
	//case RpcValue::Type::Blob: write_p(value.toBlob()); break;
	case RpcValue::Type::String: write_p(value.toString()); break;
	case RpcValue::Type::DateTime: write_p(value.toDateTime()); break;
	case RpcValue::Type::List: write_p(value.toList()); break;
	case RpcValue::Type::Map: write_p(value.toMap()); break;
	case RpcValue::Type::IMap: write_p(value.toIMap()); break;
	case RpcValue::Type::Decimal: write_p(value.toDecimal()); break;
	case RpcValue::Type::Invalid:
		if(WRITE_INVALID_AS_NULL) {
			write_p(nullptr);
		}
		break;
	}
	flush();
	return (size_t)(m_out.tellp() - len);
}

size_t ChainPackWriter::write(const RpcValue::MetaData &meta_data)
{
	auto len = m_out.tellp();
	if(!meta_data.isEmpty()) {
		cchainpack_pack_meta_begin(&m_outCtx);
		const RpcValue::IMap &cim = meta_data.iValues();
		for (const auto &kv : cim) {
			writeMapElement(kv.first, kv.second);
		}
		const RpcValue::Map &csm = meta_data.sValues();
		for (const auto &kv : csm) {
			writeMapElement(kv.first, kv.second);
		}
		cchainpack_pack_container_end(&m_outCtx);
	}
	flush();
	return (size_t)(m_out.tellp() - len);
}

void ChainPackWriter::writeContainerBegin(RpcValue::Type container_type)
{
	switch (container_type) {
	case RpcValue::Type::List:
		cchainpack_pack_list_begin(&m_outCtx);
		break;
	case RpcValue::Type::Map:
		cchainpack_pack_map_begin(&m_outCtx);
		break;
	case RpcValue::Type::IMap:
		cchainpack_pack_imap_begin(&m_outCtx);
		break;
	default:
		SHVCHP_EXCEPTION(std::string("Cannot write begin of container type: ") + RpcValue::typeToName(container_type));
	}
}

void ChainPackWriter::writeContainerEnd(RpcValue::Type container_type)
{
	(void)container_type;
	cchainpack_pack_container_end(&m_outCtx);
	flush();
}

void ChainPackWriter::writeIMapKey(RpcValue::Int key)
{
	write(key);
}

void ChainPackWriter::writeListElement(const RpcValue &val)
{
	write(val);
}

void ChainPackWriter::writeMapElement(const std::string &key, const RpcValue &val)
{
	write(key);
	write(val);
}

void ChainPackWriter::writeMapElement(RpcValue::Int key, const RpcValue &val)
{
	write(key);
	write(val);
}

ChainPackWriter &ChainPackWriter::write_p(std::nullptr_t)
{
	cchainpack_pack_null(&m_outCtx);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(bool value)
{
	cchainpack_pack_boolean(&m_outCtx, value);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(int32_t value)
{
	cchainpack_pack_int(&m_outCtx, value);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(uint32_t value)
{
	cchainpack_pack_uint(&m_outCtx, value);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(int64_t value)
{
	cchainpack_pack_int(&m_outCtx, value);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(uint64_t value)
{
	cchainpack_pack_uint(&m_outCtx, value);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(double value)
{
	cchainpack_pack_double(&m_outCtx, value);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(RpcValue::Decimal value)
{
	cchainpack_pack_decimal(&m_outCtx, value.mantisa(), value.precision());
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(RpcValue::DateTime value)
{
	cchainpack_pack_date_time(&m_outCtx, value.msecsSinceEpoch(), value.minutesFromUtc());
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(const std::string &value)
{
	cchainpack_pack_string(&m_outCtx, value.data(), value.size());
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(const RpcValue::Map &values)
{
	writeContainerBegin(RpcValue::Type::Map);
	for (const auto &kv : values) {
		writeMapElement(kv.first, kv.second);
	}
	writeContainerEnd(RpcValue::Type::Map);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(const RpcValue::IMap &values)
{
	writeContainerBegin(RpcValue::Type::IMap);
	for (const auto &kv : values) {
		writeMapElement(kv.first, kv.second);
	}
	writeContainerEnd(RpcValue::Type::IMap);
	return *this;
}

ChainPackWriter &ChainPackWriter::write_p(const RpcValue::List &values)
{
	writeContainerBegin(RpcValue::Type::List);
	for (size_t ix = 0; ix < values.size(); ix++) {
		const RpcValue &value = values[ix];
		writeListElement(value);
	}
	writeContainerEnd(RpcValue::Type::List);
	return *this;
}

} // namespace chainpack
} // namespace shv
