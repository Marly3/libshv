#pragma once

#include "abstractstreamwriter.h"
#include "rpcvalue.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT CponProtocol final
{
public:
	//enum class Format {Compact};
	class SHVCHAINPACK_DECL_EXPORT ParseException : public std::exception
	{
		using Super = std::exception;
	public:
		ParseException(std::string &&message, int pos) : m_message(std::move(message)), m_pos(pos) {}
		const std::string& mesage() const {return m_message;}
		int pos() const {return m_pos;}
		const char *what() const noexcept override {return m_message.data();}
	private:
		std::string m_message;
		int m_pos;
	};
public:
	size_t pos() const {return m_pos;}
	static RpcValue read(const std::string & in, size_t pos = 0, size_t *new_pos = nullptr);
	static RpcValue::MetaData readMetaData(const std::string & in, size_t pos = 0, size_t *new_pos = nullptr);
	//static RpcValue read(const std::istream & in);
	//static std::string write(const RpcValue &value);
	class SHVCHAINPACK_DECL_EXPORT WriteOptions
	{
		bool m_translateIds = false;
	public:
		WriteOptions() {}
		bool translateIds() const {return m_translateIds;}
		WriteOptions& translateIds(bool b) {m_translateIds = b; return *this;}

	};
	static void write(std::ostream &out, const RpcValue &value, const WriteOptions &opts = WriteOptions());
	static void writeMetaData(std::ostream &out, const RpcValue::MetaData &meta_data, const WriteOptions &opts = WriteOptions());
private:
	CponProtocol(const std::string &str, size_t pos);
	RpcValue parseAtPos();

	void skipWhiteSpace();
	bool skipComment();

	char skipGarbage();
	char nextValidChar();
	char currentChar();

	uint64_t parseDecimalUnsigned(int radix);
	RpcValue::IMap parseIMapContent(char closing_bracket);
	RpcValue::MetaData parseMetaDataContent(char closing_bracket);
	bool parseStringHelper(std::string &val);

	bool parseMetaData(RpcValue::MetaData &meta_data);

	bool parseNull(RpcValue &val);
	bool parseBool(RpcValue &val);
	bool parseString(RpcValue &val);
	bool parseBlob(RpcValue &val);
	bool parseNumber(RpcValue &val);
	bool parseList(RpcValue &val);
	bool parseArray(RpcValue &ret_val);
	bool parseMap(RpcValue &val);
	bool parseIMap(RpcValue &val);
	bool parseDateTime(RpcValue &val);
private:
	void encodeUtf8(long pt, std::string & out);
public:
	static void write(std::nullptr_t, std::ostream &out);
	static void write(double value, std::ostream &out);
	static void write(RpcValue::Int value, std::ostream &out);
	static void write(RpcValue::UInt value, std::ostream &out);
	static void write(bool value, std::ostream &out);
	static void write(RpcValue::DateTime value, std::ostream &out);
	static void write(RpcValue::Decimal value, std::ostream &out);
	static void write(const std::string &value, std::ostream &out);
	static void write(const RpcValue::Blob &value, std::ostream &out);
	static void write(const RpcValue::List &values, std::ostream &out);
	static void write(const RpcValue::Array &values, std::ostream &out);
	static void write(const RpcValue::Map &values, std::ostream &out);
	static void write(const RpcValue::IMap &values, std::ostream &out, const WriteOptions &opts, const RpcValue::MetaData &meta_data);
	static void write(const RpcValue::MetaData &value, std::ostream &out, const WriteOptions &opts);
private:
	const std::string &m_str;
	//std::string &m_err;
	size_t m_pos = 0;
	int m_depth = 0;
	//bool m_verboseIds = true;
	static constexpr bool WRITE_INVALID_AS_NULL = true;
};

class SHVCHAINPACK_DECL_EXPORT CponWriter : public AbstractStreamWriter
{
	using Super = AbstractStreamWriter;
public:
	class SHVCHAINPACK_DECL_EXPORT WriteOptions
	{
		bool m_translateIds = false;
	public:
		WriteOptions() {}
		bool translateIds() const {return m_translateIds;}
		WriteOptions& translateIds(bool b) {m_translateIds = b; return *this;}
	};

	enum class Begin {Map, IMap, List, Meta, Array};
	enum class End {Map, IMap, List, Meta, Array};

	class ListElement : public RpcValue {};
	class MapElement {};
	class IMapElement {};
public:
	CponWriter(std::ostream &out) : m_out(out) {}
	CponWriter(std::ostream &out, const WriteOptions &opts) : CponWriter(out) {m_opts = opts;}

	CponWriter& operator <<(Begin manip);
	CponWriter& operator <<(const ListElement &el);
	CponWriter& operator <<(const MapElement &el);
	CponWriter& operator <<(const IMapElement &el);
	CponWriter& operator <<(End manip);

	CponWriter& operator <<(std::nullptr_t);
	CponWriter& operator <<(bool value);
	CponWriter& operator <<(RpcValue::Int value);
	CponWriter& operator <<(RpcValue::UInt value);
	CponWriter& operator <<(double value);
	CponWriter& operator <<(RpcValue::Decimal value);
	CponWriter& operator <<(RpcValue::DateTime value);
	CponWriter& operator <<(const std::string &value);
	CponWriter& operator <<(const RpcValue::Blob &value);
	CponWriter& operator <<(const RpcValue::List &values);
	CponWriter& operator <<(const RpcValue::Array &values);
	CponWriter& operator <<(const RpcValue::Map &values);
	CponWriter& operator <<(const RpcValue::IMap &values);
	CponWriter& operator <<(const RpcValue::MetaData &value);
private:
	std::ostream &m_out;
	WriteOptions m_opts;
};

} // namespace chainpack
} // namespace shv
