#pragma once

#include "rpcvalue.h"
#include "../../c/ccpcp.h"

#include <istream>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT AbstractStreamReader
{
public:
	class SHVCHAINPACK_DECL_EXPORT ParseException : public std::exception
	{
		using Super = std::exception;
	public:
		ParseException(const std::string msg, long pos) : m_msg(msg), m_pos(pos) {}

		const char *what() const noexcept override;
		long pos() const {return m_pos;}
		const std::string& msg() const {return m_msg;}
	private:
		std::string m_msg;
		long m_pos = -1;
	};
	friend size_t unpack_underflow_handler(ccpcp_unpack_context *ctx);
public:
	AbstractStreamReader(std::istream &in);
	virtual ~AbstractStreamReader();

	RpcValue read();

	virtual void read(RpcValue::MetaData &meta_data) = 0;
	virtual void read(RpcValue &val) = 0;
protected:
	std::istream &m_in;
	char m_unpackBuff[1];
	//static constexpr size_t CONTAINER_STATE_CNT = 100;
	//ccpcp_container_state m_containerStates[CONTAINER_STATE_CNT];
	//ccpcp_container_stack m_containerStack;
	ccpcp_unpack_context m_inCtx;
};

} // namespace chainpack
} // namespace shv

