#pragma once

#include "../shvchainpackglobal.h"
#include "utils1.h"

#include <stdexcept>
#include <string>

namespace shv {
namespace chainpack1 {

class SHVCHAINPACK_DECL_EXPORT Exception : public std::exception
{
public:
	static constexpr bool Throw = true;
public:
	Exception(const std::string& _msg, const std::string& _where = std::string());
	~Exception() throw() override {}
public:
	//virtual void log();
	virtual std::string message() const {return m_msg;}
	virtual std::string where() const {return m_where;}
	const char* what() const noexcept override;
	//operator const char *() const  throw(){return what();}
	static void setAbortOnException(bool on) {s_abortOnException = on;}
	static bool isAbortOnException() {return s_abortOnException;}
protected:
	static bool s_abortOnException;
	std::string m_msg;
	std::string m_where;
};

}}

#define SHVCHP_EXCEPTION(e) throw shv::chainpack1::Exception(e, std::string(__FILE__) + ":" + shv::chainpack1::Utils::toString(__LINE__));
