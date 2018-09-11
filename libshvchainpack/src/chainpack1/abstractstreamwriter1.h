#pragma once

#include "rpcvalue1.h"

#include <ostream>

namespace shv {
namespace chainpack1 {

class SHVCHAINPACK_DECL_EXPORT AbstractStreamWriter
{
public:
	AbstractStreamWriter(std::ostream &out);
	virtual ~AbstractStreamWriter();

	virtual size_t write(const RpcValue::MetaData &meta_data) = 0;
	virtual size_t write(const RpcValue &val) = 0;

	//virtual void writeMetaDataBegin() = 0;
	//virtual void writeMetaDataEnd() = 0;
	virtual void writeIMapKey(RpcValue::UInt key) = 0;
	virtual void writeContainerBegin(RpcValue::Type container_type) = 0;
	virtual void writeListElement(const RpcValue &val) = 0;
	virtual void writeMapElement(const std::string &key, const RpcValue &val) = 0;
	virtual void writeMapElement(RpcValue::UInt key, const RpcValue &val) = 0;
	virtual void writeArrayBegin(RpcValue::Type array_type, size_t array_size) = 0;
	virtual void writeArrayElement(const RpcValue &val) = 0;
	virtual void writeContainerEnd(RpcValue::Type container_type) = 0;

protected:
	static constexpr bool WRITE_INVALID_AS_NULL = true;
protected:
	std::ostream &m_out;
};

} // namespace chainpack
} // namespace shv

