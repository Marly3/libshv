#pragma once

#include "rpcvalue.h"
#include "../../c/ccpcp.h"

#include <ostream>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT AbstractStreamWriter
{
	friend void pack_overflow_handler(ccpcp_pack_context *ctx, size_t size_hint);
public:
	AbstractStreamWriter(std::ostream &out);
	virtual ~AbstractStreamWriter();

	virtual void write(const RpcValue::MetaData &meta_data) = 0;
	virtual void write(const RpcValue &val) = 0;

	virtual void writeIMapKey(RpcValue::Int key) = 0;
	virtual void writeContainerBegin(RpcValue::Type container_type) = 0;
	virtual void writeListElement(const RpcValue &val) = 0;
	virtual void writeMapElement(const std::string &key, const RpcValue &val) = 0;
	virtual void writeMapElement(RpcValue::Int key, const RpcValue &val) = 0;
	virtual void writeContainerEnd(RpcValue::Type container_type) = 0;
protected:
	static constexpr bool WRITE_INVALID_AS_NULL = true;
private:
	void flush();
protected:
	std::ostream &m_out;
	char m_packBuff[32];
	ccpcp_pack_context m_outCtx;
};

} // namespace chainpack
} // namespace shv

