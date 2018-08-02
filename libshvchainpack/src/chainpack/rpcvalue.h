#pragma once

#include "../shvchainpackglobal.h"
#include "exception.h"
#include "metatypes.h"
#include "shareddata.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <initializer_list>

#ifndef CHAINPACK_UINT
	#define CHAINPACK_UINT unsigned
#endif
namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT RpcValue
{
public:
	class AbstractValueData;

	enum class Type {
		Invalid,
		Null,
		UInt,
		Int,
		Double,
		Bool,
		Blob,
		String,
		DateTime,
		List,
		Array,
		Map,
		IMap,
		Decimal,
		//MetaData,
	};
	static const char* typeToName(Type t);

	using Int = int; //int64_t;
	using UInt = unsigned; //uint64_t;
	class SHVCHAINPACK_DECL_EXPORT Decimal
	{
		struct Num {
			int64_t precision: 16, mantisa: 48;
		};
		Num m_num = {0, 0};
	public:
		Decimal() : m_num{-1, 0} {}
		Decimal(int64_t mantisa, int16_t precision) : m_num{precision, mantisa} {}
		static Decimal fromDouble(double n, int16_t precision)
		{
			if(precision > 0) {
				//m_num.mantisa = (int)(n * std::pow(10, precision) + 0.5);
				for(; precision > 0; precision--) n *= 10;
			}
			return Decimal((Int)(n + 0.5), precision);
		}
		int64_t mantisa() const {return m_num.mantisa;}
		int16_t precision() const {return m_num.precision;}
		double toDouble() const
		{
			double ret = mantisa();
			int prec = precision();
			if(prec > 0)
				for(; prec > 0; prec--) ret /= 10;
			else
				for(; prec < 0; prec++) ret *= 10;
			return ret;
		}
		bool isValid() const {return !(mantisa() == 0 && precision() != 0);}
		std::string toString() const;
	};
	class SHVCHAINPACK_DECL_EXPORT DateTime
	{
	public:
		enum class MsecPolicy {Auto, Always, Never};
		static constexpr bool IncludeTimeZone = true;
	public:
		DateTime() : m_dtm{TZ_INVALID, 0} {}
		int64_t msecsSinceEpoch() const { return m_dtm.msec; }
		int minutesFromUtc() const { return m_dtm.tz * 15; }

		static DateTime now();
		static DateTime fromLocalString(const std::string &local_date_time_str);
		static DateTime fromUtcString(const std::string &utc_date_time_str);
		static DateTime fromMSecsSinceEpoch(int64_t msecs, int utc_offset_min = 0);

		void setTimeZone(int utc_offset_min) {m_dtm.tz = utc_offset_min / 15;}

		std::string toLocalString() const;
		std::string toIsoString() const {return toIsoString(MsecPolicy::Auto, IncludeTimeZone);}
		std::string toIsoString(MsecPolicy msec_policy, bool include_tz) const;

		bool operator ==(const DateTime &o) const
		{
			if(!o.isValid())
				return !o.isValid();
			if(!isValid())
				return false;
			return (m_dtm.msec == o.m_dtm.msec);
		}
		bool operator <(const DateTime &o) const
		{
			if(!o.isValid())
				return false;
			if(!isValid())
				return true;
			return m_dtm.msec < o.m_dtm.msec;
		}
		bool isValid() const
		{
			return m_dtm.tz != TZ_INVALID;
		}
	private:
		static constexpr int8_t TZ_INVALID = -64;
		struct MsTz {
			int64_t tz: 7, msec: 57;
		};
		MsTz m_dtm = {0, 0};
	};
	using String = std::string;
	struct SHVCHAINPACK_DECL_EXPORT Blob : public std::basic_string<char>
	{
	private:
		using Super = std::basic_string<char>;
	public:
		using Super::Super; // expose base class constructors
		Blob() : Super() {}
		Blob(const Super &str) : Super(str) {}
		Blob(Super &&str) : Super(std::move(str)) {}
		//const std::string& toString() const {return *this;}
	};
	class List : public std::vector<RpcValue>
	{
		using Super = std::vector<RpcValue>;
		using Super::Super; // expose base class constructors
	public:
		RpcValue value(size_t ix) const
		{
			if(ix >= size())
				return RpcValue();
			return operator [](ix);
		}
	};
	class Map : public std::map<String, RpcValue>
	{
		using Super = std::map<String, RpcValue>;
		using Super::Super; // expose base class constructors
	public:
		RpcValue value(const std::string &key, const RpcValue &default_val = RpcValue()) const
		{
			auto it = find(key);
			if(it == end())
				return default_val;
			return it->second;
		}
		bool hasKey(const std::string &key) const
		{
			auto it = find(key);
			return !(it == end());
		}
	};
	class IMap : public std::map<RpcValue::UInt, RpcValue>
	{
		using Super = std::map<RpcValue::UInt, RpcValue>;
		using Super::Super; // expose base class constructors
	public:
		RpcValue value(unsigned key, const RpcValue &default_val = RpcValue()) const
		{
			auto it = find(key);
			if(it == end())
				return default_val;
			return it->second;
		}
		bool hasKey(unsigned key) const
		{
			auto it = find(key);
			return !(it == end());
		}
	};
	union ArrayElement
	{
		int64_t int_value;
		uint64_t uint_value;
		double double_value;
		bool bool_value;
		std::nullptr_t null_value;
		DateTime datetime_value;
		Decimal decimal_value;

		ArrayElement() {}
		ArrayElement(std::nullptr_t) : null_value(nullptr) {}
		ArrayElement(int32_t i) : int_value(i) {}
		ArrayElement(int64_t i) : int_value(i) {}
		ArrayElement(uint32_t i) : uint_value(i) {}
		ArrayElement(uint64_t i) : uint_value(i) {}
		ArrayElement(double d) : double_value(d) {}
		ArrayElement(bool b) : bool_value(b) {}
		ArrayElement(DateTime dt) : datetime_value(dt) {}
		ArrayElement(Decimal dt) : decimal_value(dt) {}
	};
	class SHVCHAINPACK_DECL_EXPORT Array : public std::vector<ArrayElement>
	{
		using Super = std::vector<ArrayElement>;
	public:
		Array() {}
		Array(Type type) : m_type(type) {}
		template<typename T, size_t sz>
		Array(const T(&arr)[sz])
		{
			reserve(sz);
			m_type = RpcValue::guessType<T>();
			for (size_t i = 0; i < sz; ++i) {
				ArrayElement el(arr[i]);
				push_back(std::move(el));
			}
		}
		bool operator ==(const Array &o) const
		{
			for (size_t i = 0; i < size(); ++i) {
				if(!(valueAt(i) == o.valueAt(i)))
					return false;
			}
			return true;
		}
		Type type() const {return m_type;}
		RpcValue valueAt(size_t ix) const
		{
			switch(type()) {
			case RpcValue::Type::Null: return RpcValue(nullptr);
			case RpcValue::Type::Int: return RpcValue(Super::at(ix).int_value);
			case RpcValue::Type::UInt: return RpcValue(Super::at(ix).uint_value);
			case RpcValue::Type::Double: return RpcValue(Super::at(ix).double_value);
			case RpcValue::Type::Bool: return RpcValue(Super::at(ix).bool_value);
			case RpcValue::Type::DateTime: return RpcValue(Super::at(ix).datetime_value);
			case RpcValue::Type::Decimal: return RpcValue(Super::at(ix).decimal_value);
			default: SHVCHP_EXCEPTION("Unsupported array type");
			}
		}
		static ArrayElement makeElement(const RpcValue &val)
		{
			ArrayElement el;
			switch(val.type()) {
			case RpcValue::Type::Null: el.null_value = nullptr; break;
			case RpcValue::Type::Int: el.int_value = val.toInt(); break;
			case RpcValue::Type::UInt: el.uint_value = val.toUInt(); break;
			case RpcValue::Type::Double: el.double_value = val.toDouble(); break;
			case RpcValue::Type::Bool: el.bool_value = val.toBool(); break;
			case RpcValue::Type::DateTime: el.datetime_value = val.toDateTime(); break;
			case RpcValue::Type::Decimal: el.decimal_value = val.toDecimal(); break;
			default: SHVCHP_EXCEPTION("Unsupported array type");
			}
			return el;
		}
	private:
		Type m_type = Type::Invalid;
	};
	class SHVCHAINPACK_DECL_EXPORT MetaData
	{
	public:
		MetaData() {}
		MetaData(MetaData &&o);
		MetaData(RpcValue::IMap &&imap);
		MetaData(RpcValue::Map &&smap);
		MetaData(RpcValue::IMap &&imap, RpcValue::Map &&smap);
		MetaData(const MetaData &o);
		~MetaData();

		MetaData& operator =(MetaData &&o) {swap(o); return *this;}

		int metaTypeId() const {return value(meta::Tag::MetaTypeId).toInt();}
		void setMetaTypeId(RpcValue::Int id) {setValue(meta::Tag::MetaTypeId, id);}
		int metaTypeNameSpaceId() const {return value(meta::Tag::MetaTypeNameSpaceId).toInt();}
		void setMetaTypeNameSpaceId(RpcValue::Int id) {setValue(meta::Tag::MetaTypeNameSpaceId, id);}
		std::vector<RpcValue::UInt> iKeys() const;
		std::vector<RpcValue::String> sKeys() const;
		RpcValue value(RpcValue::UInt key) const;
		RpcValue value(RpcValue::String key) const;
		void setValue(RpcValue::UInt key, const RpcValue &val);
		void setValue(RpcValue::String key, const RpcValue &val);
		bool isEmpty() const;
		bool operator==(const MetaData &o) const;
		const RpcValue::IMap& iValues() const;
		const RpcValue::Map& sValues() const;
		std::string toStdString() const;
	private:
		MetaData& operator =(const MetaData &o);
		void swap(MetaData &o);
	private:
		RpcValue::IMap *m_imap = nullptr;
		RpcValue::Map *m_smap = nullptr;
	};

	// Constructors for the various types of JSON value.
	RpcValue() noexcept;                // Null
	//RpcValue(RpcValue&&) = default;
#ifdef RPCVALUE_COPY_AND_SWAP
	RpcValue(const RpcValue &other) noexcept : m_ptr(other.m_ptr) {}
	RpcValue(RpcValue &&other) noexcept : RpcValue() { swap(other); }
#endif
	RpcValue(std::nullptr_t) noexcept;  // Null
	RpcValue(bool value);               // Bool

	RpcValue(int32_t value);                // Int
	RpcValue(uint32_t value);                // UInt
	RpcValue(int64_t value);                // Int
	RpcValue(uint64_t value);                // UInt
	RpcValue(double value);             // Double
	RpcValue(Decimal value);             // Decimal
	RpcValue(const DateTime &value);
	RpcValue(const Blob &value); // Blob
	RpcValue(Blob &&value);
	RpcValue(const uint8_t *value, size_t size);
	RpcValue(const std::string &value); // String
	RpcValue(std::string &&value);      // String
	RpcValue(const char *value);       // String
	RpcValue(const List &values);      // List
	RpcValue(List &&values);           // List
	RpcValue(const Array &values);
	RpcValue(Array &&values);
	RpcValue(const Map &values);     // Map
	RpcValue(Map &&values);          // Map
	RpcValue(const IMap &values);     // IMap
	RpcValue(IMap &&values);          // IMap

	// Implicit constructor: anything with a to_json() function.
	template <class T, class = decltype(&T::to_json)>
	RpcValue(const T & t) : RpcValue(t.to_json()) {}

	// Implicit constructor: map-like objects (std::map, std::unordered_map, etc)
	template <class M, typename std::enable_if<
				  std::is_constructible<RpcValue::String, typename M::key_type>::value
				  && std::is_constructible<RpcValue, typename M::mapped_type>::value,
				  int>::type = 0>
	RpcValue(const M & m) : RpcValue(Map(m.begin(), m.end())) {}

	// Implicit constructor: vector-like objects (std::list, std::vector, std::set, etc)
	template <class V, typename std::enable_if<
				  std::is_constructible<RpcValue, typename V::value_type>::value,
				  int>::type = 0>
	RpcValue(const V & v) : RpcValue(List(v.begin(), v.end())) {}

	// This prevents ChainPack(some_pointer) from accidentally producing a bool. Use
	// ChainPack(bool(some_pointer)) if that behavior is desired.
	RpcValue(void *) = delete;

	~RpcValue();

	Type type() const;
	Type arrayType() const;

	const MetaData &metaData() const;
	RpcValue metaValue(RpcValue::UInt key) const;
	RpcValue metaValue(const RpcValue::String &key) const;
	void setMetaData(MetaData &&meta_data);
	void setMetaValue(UInt key, const RpcValue &val);
	void setMetaValue(const String &key, const RpcValue &val);

	template<typename T> static Type guessType();

	bool isValid() const;
	bool isNull() const { return type() == Type::Null; }
	bool isInt() const { return type() == Type::Int; }
	bool isUInt() const { return type() == Type::UInt; }
	bool isDouble() const { return type() == Type::Double; }
	bool isBool() const { return type() == Type::Bool; }
	bool isString() const { return type() == Type::String; }
	bool isBlob() const { return type() == Type::Blob; }
	bool isDateTime() const { return type() == Type::DateTime; }
	bool isList() const { return type() == Type::List; }
	bool isArray() const { return type() == Type::Array; }
	bool isMap() const { return type() == Type::Map; }
	bool isIMap() const { return type() == Type::IMap; }

	double toDouble() const;
	Decimal toDecimal() const;
	Int toInt() const;
	UInt toUInt() const;
	int64_t toInt64() const;
	uint64_t toUInt64() const;
	bool toBool() const;
	DateTime toDateTime() const;
	const RpcValue::String &toString() const;
	const Blob &toBlob() const;
	const List &toList() const;
	const Array &toArray() const;
	const Map &toMap() const;
	const IMap &toIMap() const;

	size_t count() const;
	RpcValue at(UInt i) const;
	RpcValue at(const RpcValue::String &key) const;
	RpcValue operator[](UInt i) const {return at(i);}
	RpcValue operator[](const RpcValue::String &key) const {return at(key);}
	void set(UInt ix, const RpcValue &val);
	void set(const RpcValue::String &key, const RpcValue &val);
	void append(const RpcValue &val);

	std::string toPrettyString(const std::string &indent = std::string()) const;
	std::string toStdString() const;
	std::string toCpon() const;
	static RpcValue fromCpon(const std::string & str, std::string *err = nullptr);

	std::string toChainPack() const;

	bool operator== (const RpcValue &rhs) const;
#ifdef RPCVALUE_COPY_AND_SWAP
	RpcValue& operator= (RpcValue rhs) noexcept
	{
		swap(rhs);
		return *this;
	}
	void swap(RpcValue& other) noexcept;
#endif
	/*
	bool operator<  (const ChainPack &rhs) const;
	bool operator!= (const ChainPack &rhs) const { return !(*this == rhs); }
	bool operator<= (const ChainPack &rhs) const { return !(rhs < *this); }
	bool operator>  (const ChainPack &rhs) const { return  (rhs < *this); }
	bool operator>= (const ChainPack &rhs) const { return !(*this < rhs); }
	*/
public:
	class AbstractValueData : public SharedData
	{
	public:
		AbstractValueData();
		virtual ~AbstractValueData();

		static int nn();

		virtual RpcValue::Type type() const {return RpcValue::Type::Invalid;}
		virtual RpcValue::Type arrayType() const {return RpcValue::Type::Invalid;}

		virtual const RpcValue::MetaData &metaData() const = 0;
		virtual void setMetaData(RpcValue::MetaData &&meta_data) = 0;
		virtual void setMetaValue(RpcValue::UInt key, const RpcValue &val) = 0;
		virtual void setMetaValue(RpcValue::String key, const RpcValue &val) = 0;

		virtual bool equals(const AbstractValueData * other) const = 0;
		//virtual bool less(const Data * other) const = 0;

		virtual bool isNull() const {return false;}
		virtual double toDouble() const {return 0;}
		virtual RpcValue::Decimal toDecimal() const { return RpcValue::Decimal{}; }
		virtual RpcValue::Int toInt() const {return 0;}
		virtual RpcValue::UInt toUInt() const {return 0;}
		virtual int64_t toInt64() const {return 0;}
		virtual uint64_t toUInt64() const {return 0;}
		virtual bool toBool() const {return false;}
		virtual RpcValue::DateTime toDateTime() const { return RpcValue::DateTime{}; }
		virtual const std::string &toString() const;
		virtual const RpcValue::Blob &toBlob() const;
		virtual const RpcValue::List &toList() const;
		virtual const RpcValue::Array &toArray() const;
		virtual const RpcValue::Map &toMap() const;
		virtual const RpcValue::IMap &toIMap() const;
		virtual size_t count() const {return 0;}

		virtual RpcValue at(RpcValue::UInt i) const;
		virtual RpcValue at(const RpcValue::String &key) const;
		virtual void set(RpcValue::UInt ix, const RpcValue &val);
		virtual void set(const RpcValue::String &key, const RpcValue &val);
		virtual void append(const RpcValue &);

		virtual std::string toStdString() const = 0;
	};
private:
	ExplicitlySharedDataPointer<AbstractValueData> m_ptr;
};

template<typename T> RpcValue::Type guessType() { throw std::runtime_error("guessing of this type is not implemented"); }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::Int>() { return Type::Int; }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::UInt>() { return Type::UInt; }
template<> inline RpcValue::Type RpcValue::guessType<uint16_t>() { return Type::UInt; }
template<> inline RpcValue::Type RpcValue::guessType<bool>() { return Type::Bool; }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::DateTime>() { return Type::DateTime; }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::Decimal>() { return Type::Decimal; }

class RpcValueGenList
{
public:
	RpcValueGenList(const RpcValue &v) : m_val(v) {}

	RpcValue value(size_t ix) const
	{
		if(m_val.isList())
			return m_val.toList().value(ix);
		else if(ix == 0)
			return m_val;
		return RpcValue();
	}
	bool size() const
	{
		if(m_val.isList())
			return m_val.toList().size();
		return m_val.isValid()? 1: 0;
	}
	bool empty() const {return size() == 0;}
private:
	RpcValue m_val;
};

}}
