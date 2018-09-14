#include "chainpack.h"
#include "chainpackreader.h"
#include "../../c/cchainpack.h"

#include <iostream>
#include <cmath>

namespace shv {
namespace chainpack {

#define PARSE_EXCEPTION(msg) {\
	char buff[40]; \
	m_in.readsome(buff, sizeof(buff)); \
	if(exception_aborts) { \
		std::clog << __FILE__ << ':' << __LINE__;  \
		std::clog << ' ' << (msg) << " at pos: " << m_in.tellg() << " near to: " << buff << std::endl; \
		abort(); \
	} \
	else { \
		throw ChainPackReader::ParseException(msg + std::string(" at pos: ") + std::to_string(m_in.tellg()) + " near to: " + buff); \
	} \
}

namespace {
enum {exception_aborts = 0};
/*
const int MAX_RECURSION_DEPTH = 1000;

class DepthScope
{
public:
	DepthScope(int &depth) : m_depth(depth) {m_depth++;}
	~DepthScope() {m_depth--;}
private:
	int &m_depth;
};
*/
}

ChainPackReader &ChainPackReader::operator >>(RpcValue &value)
{
	read(value);
	return *this;
}

ChainPackReader &ChainPackReader::operator >>(RpcValue::MetaData &meta_data)
{
	read(meta_data);
	return *this;
}

void ChainPackReader::unpackNext()
{
	cchainpack_unpack_next(&m_inCtx);
	if(m_inCtx.err_no != CCPCP_RC_OK)
		PARSE_EXCEPTION("Cpon parse error: " + std::to_string(m_inCtx.err_no));
}

void ChainPackReader::read(RpcValue &val, std::string &err)
{
	err.clear();
	try {
		read(val);
	}
	catch (ParseException &e) {
		err = e.what();
	}
}

uint64_t ChainPackReader::readUIntData(bool *ok)
{
	return cchainpack_unpack_uint_data(&m_inCtx, ok);
}

uint64_t ChainPackReader::readUIntData(std::istream &in, bool *ok)
{
	ChainPackReader rd(in);
	return rd.readUIntData(ok);
}

void ChainPackReader::read(RpcValue &val)
{
	//if (m_depth > MAX_RECURSION_DEPTH)
	//	PARSE_EXCEPTION("maximum nesting depth exceeded");
	//DepthScope{m_depth};

	RpcValue::MetaData md;
	read(md);

	unpackNext();

	switch(m_inCtx.item.type) {
	case CCPCP_ITEM_INVALID: {
		// end of input
		break;
	}
	case CCPCP_ITEM_LIST: {
		parseList(val);
		break;
	}
	case CCPCP_ITEM_MAP: {
		parseMap(val);
		break;
	}
	case CCPCP_ITEM_IMAP: {
		parseIMap(val);
		break;
	}
	case CCPCP_ITEM_CONTAINER_END: {
		break;
	}
	case CCPCP_ITEM_NULL: {
		val = RpcValue(nullptr);
		break;
	}
	case CCPCP_ITEM_STRING: {
		ccpcp_string *it = &(m_inCtx.item.as.String);
		std::string str;
		while(m_inCtx.item.type == CCPCP_ITEM_STRING) {
			str += std::string(it->chunk_start, it->chunk_size);
			if(it->last_chunk)
				break;
			unpackNext();
			if(m_inCtx.item.type != CCPCP_ITEM_STRING)
				PARSE_EXCEPTION("Unfinished string");
		}
		val = str;
		break;
	}
	case CCPCP_ITEM_BOOLEAN: {
		val = m_inCtx.item.as.Bool;
		break;
	}
	case CCPCP_ITEM_INT: {
		val = m_inCtx.item.as.Int;
		break;
	}
	case CCPCP_ITEM_UINT: {
		val = m_inCtx.item.as.UInt;
		break;
	}
	case CCPCP_ITEM_DECIMAL: {
		auto *it = &(m_inCtx.item.as.Decimal);
		val = RpcValue::Decimal(it->mantisa, it->dec_places);
		break;
	}
	case CCPCP_ITEM_DOUBLE: {
		val = m_inCtx.item.as.Double;
		break;
	}
	case CCPCP_ITEM_DATE_TIME: {
		auto *it = &(m_inCtx.item.as.DateTime);
		val = RpcValue::DateTime::fromMSecsSinceEpoch(it->msecs_since_epoch, it->minutes_from_utc);
		break;
	}
	default:
		PARSE_EXCEPTION("Invalid type.");
	}
	if(!md.isEmpty())
		val.setMetaData(std::move(md));
}

void ChainPackReader::parseList(RpcValue &val)
{
	RpcValue::List lst;
	while (true) {
		RpcValue v;
		read(v);
		if(m_inCtx.item.type == CCPCP_ITEM_CONTAINER_END) {
			m_inCtx.item.type = CCPCP_ITEM_INVALID;
			break;
		}
		lst.push_back(v);
	}
	val = lst;
}

void ChainPackReader::parseMetaData(RpcValue::MetaData &meta_data)
{
	while (true) {
		RpcValue key;
		read(key);
		if(m_inCtx.item.type == CCPCP_ITEM_CONTAINER_END) {
			m_inCtx.item.type = CCPCP_ITEM_INVALID;
			break;
		}
		RpcValue val;
		read(val);
		if(key.isString())
			meta_data.setValue(key.toString(), val);
		else
			meta_data.setValue(key.toInt(), val);
	}
}

void ChainPackReader::parseMap(RpcValue &val)
{
	RpcValue::Map map;
	while (true) {
		RpcValue key;
		read(key);
		if(m_inCtx.item.type == CCPCP_ITEM_CONTAINER_END) {
			m_inCtx.item.type = CCPCP_ITEM_INVALID;
			break;
		}
		RpcValue val;
		read(val);
		map[key.toString()] = val;
	}
	val = map;
}

void ChainPackReader::parseIMap(RpcValue &val)
{
	RpcValue::IMap map;
	while (true) {
		RpcValue key;
		read(key);
		if(m_inCtx.item.type == CCPCP_ITEM_CONTAINER_END) {
			m_inCtx.item.type = CCPCP_ITEM_INVALID;
			break;
		}
		RpcValue val;
		read(val);
		map[key.toInt()] = val;
	}
	val = map;
}

void ChainPackReader::read(RpcValue::MetaData &meta_data)
{
	const uint8_t *b = (const uint8_t*)ccpcp_unpack_take_byte(&m_inCtx);
	m_inCtx.current--;
	if(b && *b == CP_MetaMap) {
		cchainpack_unpack_next(&m_inCtx);
		parseMetaData(meta_data);
	}
}

} // namespace chainpack
} // namespace shv
