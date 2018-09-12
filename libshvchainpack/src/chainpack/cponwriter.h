#pragma once

#include "abstractstreamwriter.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT CponWriterOptions
{
	bool m_translateIds = false;
	bool m_hexBlob = false;
	std::string m_indent;
	bool m_jsonFormat = false;
public:
	//CponWriterOptions() {}

	bool isTranslateIds() const {return m_translateIds;}
	CponWriterOptions& setTranslateIds(bool b) {m_translateIds = b; return *this;}

	bool isHexBlob() const {return m_hexBlob;}
	CponWriterOptions& setHexBlob(bool b) {m_hexBlob = b; return *this;}

	const std::string& indent() const {return m_indent;}
	CponWriterOptions& setIndent(const std::string& i) {m_indent = i; return *this;}

	bool isJsonFormat() const {return m_jsonFormat;}
	CponWriterOptions& setJsonFormat(bool b) {m_jsonFormat = b; return *this;}
};

class SHVCHAINPACK_DECL_EXPORT CponWriter : public AbstractStreamWriter
{
	using Super = AbstractStreamWriter;

public:
	CponWriter(std::ostream &out) : Super(out) {}
	CponWriter(std::ostream &out, const CponWriterOptions &opts);

	CponWriter& operator <<(const RpcValue &value) {write(value); return *this;}
	CponWriter& operator <<(const RpcValue::MetaData &meta_data) {write(meta_data); return *this;}

	void write(const RpcValue &val) override;
	void write(const RpcValue::MetaData &meta_data) override;

	void writeContainerBegin(RpcValue::Type container_type) override;
	void writeContainerEnd(RpcValue::Type container_type) override;

	void writeIMapKey(RpcValue::Int key) override;
	void writeListElement(const RpcValue &val) override {writeListElement(val, false);}
	void writeMapElement(const std::string &key, const RpcValue &val) override {writeMapElement(key, val, false);}
	void writeMapElement(RpcValue::Int key, const RpcValue &val) override {writeMapElement(key, val, false);}

	// terminating separator id OK in Cpon, but world is prettier without it
	void writeListElement(const RpcValue &val, bool without_separator);
	void writeMapElement(const std::string &key, const RpcValue &val, bool without_separator);
	void writeMapElement(RpcValue::Int key, const RpcValue &val, bool without_separator);
private:
	CponWriter& write_p(std::nullptr_t);
	CponWriter& write_p(bool value);
	CponWriter& write_p(int32_t value);
	CponWriter& write_p(uint32_t value);
	CponWriter& write_p(int64_t value);
	CponWriter& write_p(uint64_t value);
	CponWriter& write_p(double value);
	CponWriter& write_p(RpcValue::Decimal value);
	CponWriter& write_p(RpcValue::DateTime value);
	CponWriter& write_p(const std::string &value);
	//CponWriter& write(const RpcValue::Blob &value);
	CponWriter& write_p(const RpcValue::List &values);
	//CponWriter& write(const RpcValue::Array &values);
	CponWriter& write_p(const RpcValue::Map &values);
	CponWriter& write_p(const RpcValue::IMap &values, const RpcValue::MetaData *meta_data = nullptr);
private:
	CponWriterOptions m_opts;
};

} // namespace chainpack
} // namespace shv

