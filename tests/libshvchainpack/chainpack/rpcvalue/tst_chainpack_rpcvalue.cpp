#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/chainpackprotocol.h>

#include <QtTest/QtTest>
#include <QDebug>

#include <cassert>
#include <string>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <list>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <type_traits>

using namespace shv::chainpack;
using std::string;

// Check that ChainPack has the properties we want.
#define CHECK_TRAIT(x) static_assert(std::x::value, #x)
CHECK_TRAIT(is_nothrow_constructible<RpcValue>);
CHECK_TRAIT(is_nothrow_default_constructible<RpcValue>);
CHECK_TRAIT(is_copy_constructible<RpcValue>);
CHECK_TRAIT(is_nothrow_move_constructible<RpcValue>);
CHECK_TRAIT(is_copy_assignable<RpcValue>);
CHECK_TRAIT(is_nothrow_move_assignable<RpcValue>);
CHECK_TRAIT(is_nothrow_destructible<RpcValue>);

namespace {
QDebug operator<<(QDebug debug, const std::string &s)
{
	//QDebugStateSaver saver(debug);
	debug << s.c_str();

	return debug;
}

template< typename T >
std::string int_to_hex( T i )
{
	std::stringstream stream;
	stream << "0x"
			  //<< std::setfill ('0') << std::setw(sizeof(T)*2)
		   << std::hex << i;
	return stream.str();
}

}

class TestRpcValue: public QObject
{
	Q_OBJECT
private:
	void textTest()
	{
		qDebug() << "============= chainpack text test ============";
		qDebug() << "--------------- List test";
		{
			string err;
			for(auto test : {"[]", "[ ]", " [ ]", "[ 1, 2,3  , ]"}) {
				const RpcValue cp = RpcValue::parseCpon(test, &err);
				qDebug() << test << "--->" << cp.toCpon();
				QVERIFY(err.empty());
			}
		}
		{
			string err;
			const string test = R"(
								[
									"foo bar",
									123,
									true,
									false,
									null,
									456u,
									0.123,
									123.456n,
									x"48656c6c6f",
									d"2018-01-14T01:17:33.256-1045"
									]
								)";
			const auto cp = RpcValue::parseCpon(test, &err);
			QVERIFY(err.empty());
			qDebug() << "List test: " << cp.toCpon().c_str();
			QVERIFY(cp[0] == "foo bar");
			QVERIFY(cp[1] == 123);
			QVERIFY(cp[2] == true);
			QVERIFY(cp[3] == false);
			QVERIFY(cp[4] == RpcValue(nullptr));
			QVERIFY(cp[5] == RpcValue::UInt(456));
			QVERIFY(cp[6] == 0.123);
			QVERIFY(cp[7] == RpcValue::Decimal(123456, 3));
			QVERIFY(cp[8] == RpcValue::Blob("Hello"));
			RpcValue::DateTime dt = cp[9].toDateTime();
			QVERIFY(dt.msecsSinceEpoch() % 1000 == 256);
			QVERIFY(dt.offsetFromUtc() == -(10*60+45));

		}
		{
			string err;
			for(auto test : {"{}", "{ }", " { }", R"({ "1": 2,"3": 45  , })"}) {
				const RpcValue cp = RpcValue::parseCpon(test, &err);
				qDebug() << test << "--->" << cp.toCpon();
				QVERIFY(err.empty());
			}
		}
		{
			string err;
			for(auto test : {"i{}", "i{ }", " i{ }", "i{ 1: 2,3: 45  , }"}) {
				const RpcValue cp = RpcValue::parseCpon(test, &err);
				qDebug() << test << "--->" << cp.toCpon();
				QVERIFY(err.empty());
			}
		}
		{
			string err;
			for(auto test : {"a[]", "a[ ]", " a[ ]", "a[ 1, 2,3  , ]"}) {
				const RpcValue cp = RpcValue::parseCpon(test, &err);
				qDebug() << test << "--->" << cp.toCpon();
				QVERIFY(err.empty());
			}
		}
		{
			string err;
			const string imap_test = R"(
									 <1:"foo", 2u: "bar">
									 i{
									   1:"v1",
									   2:42,
									   3:[
									     "a",
									     123,
									     true,
									     false,
									     null,
									     123, 456u, 0.123, 123.456n
									   ]
									 }
									 )";
			const auto json = RpcValue::parseCpon(imap_test, &err);
			QVERIFY(err.empty());
			qDebug() << "imap_test: " << json.toCpon().c_str();
		}

		qDebug() << "--------------- Map test";
		{
			string err;
			const string test = "{}";
			const auto cp = RpcValue::parseCpon(test, &err);
			QVERIFY(cp == RpcValue::Map());
		}

		const string simple_test = R"({"k1":"v1", "k2":42, "k3":["a",123,true,false,null]})";

		string err;
		const auto json = RpcValue::parseCpon(simple_test, &err);

		qDebug() << "k1: " << json["k1"].toCpon().c_str();
		qDebug() << "k3: " << json["k3"].toCpon().c_str();

		RpcValue cp = json;
		//cp.setMeta(json["k1"]);
		qDebug() << "cp: " << cp.toCpon();

		string comment_test = R"({
							  // comment /* with nested comment */
							  "a": 1,
							  // comment
							  // continued
							  "b": "text",
							  /* multi
							  line
							  comment
							  // line-comment-inside-multiline-comment
							  */
							  // and single-line comment
							  // and single-line comment /* multiline inside single line */
							  "c": [1, 2, 3]
							  // and single-line comment at end of object
							  })";

		string err_comment;
		auto json_comment = RpcValue::parseCpon( comment_test, &err_comment);
		QVERIFY(!json_comment.isNull());
		QVERIFY(err_comment.empty());

		comment_test = "{\"a\": 1}//trailing line comment";
		json_comment = RpcValue::parseCpon( comment_test, &err_comment);
		QVERIFY(!json_comment.isNull());
		QVERIFY(err_comment.empty());

		comment_test = "{\"a\": 1}/*trailing multi-line comment*/";
		json_comment = RpcValue::parseCpon( comment_test, &err_comment);
		QVERIFY(!json_comment.isNull());
		QVERIFY(err_comment.empty());

		string failing_comment_test = "{\n/* unterminated comment\n\"a\": 1,\n}";
		string err_failing_comment;
		RpcValue json_failing_comment = RpcValue::parseCpon( failing_comment_test, &err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());

		failing_comment_test = "{\n/* unterminated trailing comment }";
		json_failing_comment = RpcValue::parseCpon( failing_comment_test, &err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());

		failing_comment_test = "{\n/ / bad comment }";
		json_failing_comment = RpcValue::parseCpon( failing_comment_test, &err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());

		failing_comment_test = "{// bad comment }";
		json_failing_comment = RpcValue::parseCpon( failing_comment_test, &err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());
		/*
		failing_comment_test = "{\n\"a\": 1\n}/";
		json_failing_comment = RpcValue::parseCpon( failing_comment_test, &err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());
		*/
		failing_comment_test = "{/* bad\ncomment *}";
		json_failing_comment = RpcValue::parseCpon( failing_comment_test, &err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());

		{
			std::list<int> l1 { 1, 2, 3 };
			std::vector<int> l2 { 1, 2, 3 };
			std::set<int> l3 { 1, 2, 3 };
			QCOMPARE(RpcValue(l1), RpcValue(l2));
			QCOMPARE(RpcValue(l2), RpcValue(l3));
			std::map<string, string> m1 { { "k1", "v1" }, { "k2", "v2" } };
			std::unordered_map<string, string> m2 { { "k1", "v1" }, { "k2", "v2" } };
			QCOMPARE(RpcValue(m1), RpcValue(m2));
		}
		// ChainPack literals
		const RpcValue obj = RpcValue::Map({
											   { "k1", "v1" },
											   { "k2", 42.0 },
											   { "k3", RpcValue::List({ "a", 123.0, true, false, nullptr }) },
										   });

		qDebug() << "obj: " << obj.toCpon();
		QVERIFY(obj.toCpon() == "{\"k1\":\"v1\", \"k2\":42, \"k3\":[\"a\", 123, true, false, null]}");

		QCOMPARE(RpcValue("a").toDouble(), 0.);
		QCOMPARE(RpcValue("a").toString().c_str(), "a");
		QCOMPARE(RpcValue().toDouble(), 0.);

		const string unicode_escape_test =
				R"([ "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234" ])";

		const char utf8[] = "blah" "\xf0\x9f\x92\xa9" "blah" "\xed\xa0\xbd" "blah"
							"\xed\xb2\xa9" "blah" "\0" "blah" "\xe1\x88\xb4";

		RpcValue uni = RpcValue::parseCpon(unicode_escape_test, &err);
		QVERIFY(uni[0].toString().size() == (sizeof utf8) - 1);
		QVERIFY(std::memcmp(uni[0].toString().data(), utf8, sizeof utf8) == 0);

		RpcValue my_json = RpcValue::Map {
		{ "key1", "value1" },
		{ "key2", false },
		{ "key3", RpcValue::List { 1, 2, 3 } },
	};
		std::string json_obj_str = my_json.toCpon();
		qDebug() << "json_obj_str: " << json_obj_str.c_str();
		QCOMPARE(json_obj_str.c_str(), "{\"key1\":\"value1\", \"key2\":false, \"key3\":[1, 2, 3]}");

		class Point {
		public:
			int x;
			int y;
			Point (int x, int y) : x(x), y(y) {}
			RpcValue to_json() const { return RpcValue::List { x, y }; }
		};

		std::vector<Point> points = { { 1, 2 }, { 10, 20 }, { 100, 200 } };
		std::string points_json = RpcValue(points).toCpon();
		qDebug() << "points_json: " << points_json.c_str();
		QVERIFY(points_json == "[[1, 2], [10, 20], [100, 200]]");
	}

	static std::string binary_dump(const RpcValue::Blob &out)
	{
		std::string ret;
		for (size_t i = 0; i < out.size(); ++i) {
			uint8_t u = out[i];
			//ret += std::to_string(u);
			if(i > 0)
				ret += '|';
			for (size_t j = 0; j < 8*sizeof(u); ++j) {
				ret += (u & (((uint8_t)128) >> j))? '1': '0';
			}
		}
		return ret;
	}

	static inline char hex_nibble(char i)
	{
		if(i < 10)
			return '0' + i;
		return 'A' + (i - 10);
	}

	static std::string hex_dump(const RpcValue::Blob &out)
	{
		std::string ret;
		for (size_t i = 0; i < out.size(); ++i) {
			char h = out[i] / 16;
			char l = out[i] % 16;
			ret += hex_nibble(h);
			ret += hex_nibble(l);
		}
		return ret;
	}

	void binaryTest()
	{
		qDebug() << "============= chainpack binary test ============";
		for (int i = ChainPackProtocol::TypeInfo::Null; i <= ChainPackProtocol::TypeInfo::DateTime; ++i) {
			RpcValue::Blob out;
			out += i;
			ChainPackProtocol::TypeInfo::Enum e = (ChainPackProtocol::TypeInfo::Enum)i;
			std::ostringstream str;
			str << std::setw(3) << i << " " << std::hex << i << " " << binary_dump(out).c_str() << " "  << ChainPackProtocol::TypeInfo::name(e);
			qDebug() << str.str();
		}
		/*
		for (int i = ChainPackProtocol::TypeInfo::Null_Array; i <= ChainPackProtocol::TypeInfo::DateTimeTZ_Array; ++i) {
			RpcValue::Blob out;
			out += i;
			ChainPackProtocol::TypeInfo::Enum e = (ChainPackProtocol::TypeInfo::Enum)i;
			std::ostringstream str;
			str << std::setw(3) << i << " " << std::hex << i << " " << binary_dump(out).c_str() << " "  << ChainPackProtocol::TypeInfo::name(e);
			qDebug() << str.str();
		}
		*/
		for (int i = ChainPackProtocol::TypeInfo::FALSE; i <= ChainPackProtocol::TypeInfo::TERM; ++i) {
			RpcValue::Blob out;
			out += i;
			ChainPackProtocol::TypeInfo::Enum e = (ChainPackProtocol::TypeInfo::Enum)i;
			std::ostringstream str;
			str << std::setw(3) << i << " " << std::hex << i << " " << binary_dump(out).c_str() << " "  << ChainPackProtocol::TypeInfo::name(e);
			qDebug() << str.str();
		}
		{
			qDebug() << "------------- NULL";
			RpcValue cp1{nullptr};
			std::stringstream out;
			int len = ChainPackProtocol::write(out, cp1);
			QVERIFY(len == 1);
			RpcValue cp2 = ChainPackProtocol::read(out);
			qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
			QVERIFY(cp1.type() == cp2.type());
		}
		{
			qDebug() << "------------- tiny uint";
			for (RpcValue::UInt n = 0; n < 64; ++n) {
				RpcValue cp1{n};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				if(n < 10)
					qDebug() << n << " " << cp1.toCpon() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
				QVERIFY(len == 1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toUInt() == cp2.toUInt());
			}
		}
		{
			qDebug() << "------------- uint";
			for (unsigned i = 0; i < sizeof(RpcValue::UInt); ++i) {
				for (unsigned j = 0; j < 3; ++j) {
					RpcValue::UInt n = RpcValue::UInt{1} << (i*8 + j*3+1);
					RpcValue cp1{n};
					std::stringstream out;
					int len = ChainPackProtocol::write(out, cp1);
					//QVERIFY(len > 1);
					RpcValue cp2 = ChainPackProtocol::read(out);
					//if(n < 100*step)
					qDebug() << n << int_to_hex(n) << "..." << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
					QVERIFY(cp1.type() == cp2.type());
					QCOMPARE(cp1.toUInt(), cp2.toUInt());
				}
			}
		}
		qDebug() << "------------- tiny int";
		for (RpcValue::Int n = 0; n < 64; ++n) {
			RpcValue cp1{n};
			std::stringstream out;
			int len = ChainPackProtocol::write(out, cp1);
			if(n < 10)
				qDebug() << n << " " << cp1.toCpon() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
			QVERIFY(len == 1);
			RpcValue cp2 = ChainPackProtocol::read(out);
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.toInt() == cp2.toInt());
		}
		{
			qDebug() << "------------- int";
			{
				for (int sig = 1; sig >= -1; sig-=2) {
					for (unsigned i = 0; i < sizeof(RpcValue::Int); ++i) {
						for (unsigned j = 0; j < 3; ++j) {
							RpcValue::Int n = sig * (RpcValue::Int{1} << (i*8 + j*2+2));
							//qDebug() << sig << i << j << (i*8 + j*3+1) << n;
							RpcValue cp1{n};
							std::stringstream out;
							int len = ChainPackProtocol::write(out, cp1);
							//QVERIFY(len > 1);
							RpcValue cp2 = ChainPackProtocol::read(out);
							//if(n < 100*step)
							qDebug() << n << int_to_hex(n) << "..." << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
							QVERIFY(cp1.type() == cp2.type());
							QCOMPARE(cp1.toUInt(), cp2.toUInt());
						}
					}
				}
			}
		}
		{
			qDebug() << "------------- double";
			{
				auto n_max = 1000000.;
				auto n_min = -1000000.;
				auto step = (n_max - n_min) / 100;
				for (auto n = n_min; n < n_max; n += step) {
					RpcValue cp1{n};
					std::stringstream out;
					int len = ChainPackProtocol::write(out, cp1);
					QVERIFY(len > 1);
					RpcValue cp2 = ChainPackProtocol::read(out);
					if(n > -3*step && n < 3*step)
						qDebug() << n << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
					QVERIFY(cp1.type() == cp2.type());
					QVERIFY(cp1.toDouble() == cp2.toDouble());
				}
			}
			{
				double n_max = std::numeric_limits<double>::max();
				double n_min = std::numeric_limits<double>::min();
				double step = -1.23456789e10;
				//qDebug() << n_min << " - " << n_max << ": " << step << " === " << (n_max / step / 10);
				for (double n = n_min; n < n_max / -step / 10; n *= step) {
					RpcValue cp1{n};
					std::stringstream out;
					int len = ChainPackProtocol::write(out, cp1);
					QVERIFY(len > 1);
					RpcValue cp2 = ChainPackProtocol::read(out);
					if(n > -100 && n < 100)
						qDebug() << n << " - " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
					QVERIFY(cp1.type() == cp2.type());
					QVERIFY(cp1.toDouble() == cp2.toDouble());
				}
			}
		}
		{
			qDebug() << "------------- Decimal";
			{
				RpcValue::Int mant = 123456789;
				int prec_max = 16;
				int prec_min = -16;
				int step = 1;
				for (int prec = prec_min; prec <= prec_max; prec += step) {
					RpcValue cp1{RpcValue::Decimal(mant, prec)};
					std::stringstream out;
					int len = ChainPackProtocol::write(out, cp1);
					QVERIFY(len > 1);
					RpcValue cp2 = ChainPackProtocol::read(out);
					qDebug() << mant << prec << " - " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
					QVERIFY(cp1.type() == cp2.type());
					QVERIFY(cp1 == cp2);
				}
			}
		}
		{
			qDebug() << "------------- bool";
			for(bool b : {false, true}) {
				RpcValue cp1{b};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				QVERIFY(len == 1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << b << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toBool() == cp2.toBool());
			}
		}
		{
			qDebug() << "------------- Blob";
			RpcValue::Blob blob{"blob containing zero character"};
			blob[blob.size() - 9] = 0;
			RpcValue cp1{blob};
			std::stringstream out;
			int len = ChainPackProtocol::write(out, cp1);
			RpcValue cp2 = ChainPackProtocol::read(out);
			qDebug() << blob << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.toBlob() == cp2.toBlob());
		}
		{
			qDebug() << "------------- string";
			RpcValue::String str{"string containing zero character"};
			str[str.size() - 10] = 0;
			RpcValue cp1{str};
			std::stringstream out;
			int len = ChainPackProtocol::write(out, cp1);
			RpcValue cp2 = ChainPackProtocol::read(out);
			qDebug() << str << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str());
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.toString() == cp2.toString());
		}
		{
			qDebug() << "------------- DateTime";
			for(std::string str : {
				"2018-02-02 0:00:00.001",
				"2018-02-02 01:00:00.001+01",
				"2018-12-02 0:00:00",
				"2018-01-01 0:00:00",
				"2019-01-01 0:00:00",
				"2020-01-01 0:00:00",
				"2021-01-01 0:00:00",
				"2031-01-01 0:00:00",
				"2041-01-01 0:00:00",
				"2041-03-04 0:00:00-1015",
				"2041-03-04 0:00:00.123-1015",
				"1970-01-01 0:00:00",
				"2017-05-03 5:52:03",
				"2017-05-03T15:52:03.923Z",
				"2017-05-03T15:52:31.123+10",
				"2017-05-03T15:52:03Z",
				"2017-05-03T15:52:03.000-0130",
				"2017-05-03T15:52:03.923+00",
			}) {
				RpcValue::DateTime dt = RpcValue::DateTime::fromUtcString(str);
				RpcValue cp1{dt};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				std::string pack = out.str();
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << str << " " << dt.toUtcString().c_str() << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(pack);
				//qDebug() << cp1.toDateTime().msecsSinceEpoch() << cp1.toDateTime().offsetFromUtc();
				//qDebug() << cp2.toDateTime().msecsSinceEpoch() << cp2.toDateTime().offsetFromUtc();
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toDateTime() == cp2.toDateTime());
			}
		}
		{
			qDebug() << "------------- Array";
			{
				RpcValue::Array t{RpcValue::Type::Int};
				t.push_back(RpcValue::ArrayElement(RpcValue::Int(11)));
				t.push_back(RpcValue::Int(12));
				t.push_back(RpcValue::Int(13));
				t.push_back(RpcValue::Int(14));
				RpcValue cp1{t};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toList() == cp2.toList());
			}
			{
				static constexpr size_t N = 10;
				uint16_t samples[N];
				for (size_t i = 0; i < N; ++i) {
					samples[i] = i+1;
				}
				RpcValue::Array t{samples};
				RpcValue cp1{t};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toList() == cp2.toList());
			}
			/*
			{
				static constexpr size_t N = 10;
				std::stringstream out;
				ChainPackProtocol::writeArrayBegin(out, N, RpcValue::Type::String);
				std::string s("foo-bar");
				for (size_t i = 0; i < N; ++i) {
					ChainPackProtocol::writeArrayElement(out, RpcValue(s + shv::core::Utils::toString(i)));
				}
				RpcValue cp2 = ChainPackProtocol::read(out);
				const RpcValue::Array array = cp2.toArray();
				for (size_t i = 0; i < array.size(); ++i) {
					QVERIFY(RpcValue(s + shv::core::Utils::toString(i)) == array.valueAt(i));
				}
			}
			*/
			{
				static constexpr size_t N = 10;
				std::stringstream out;
				ChainPackProtocol::writeArrayBegin(out, RpcValue::Type::Bool, N);
				bool b = false;
				for (size_t i = 0; i < N; ++i) {
					ChainPackProtocol::writeArrayElement(out, RpcValue(b));
					b = !b;
				}
				RpcValue cp2 = ChainPackProtocol::read(out);
				const RpcValue::Array array = cp2.toArray();
				b = false;
				for (size_t i = 0; i < array.size(); ++i) {
					QVERIFY(RpcValue(b) == array.valueAt(i));
					b = !b;
				}
			}
			{
				static constexpr size_t N = 10;
				std::stringstream out;
				ChainPackProtocol::writeArrayBegin(out, RpcValue::Type::Null, N);
				RpcValue cp2 = ChainPackProtocol::read(out);
				const RpcValue::Array array = cp2.toArray();
				for (size_t i = 0; i < array.size(); ++i) {
					QVERIFY(RpcValue(nullptr) == array.valueAt(i));
				}
			}
		}
		{
			qDebug() << "------------- List";
			{
				const std::string s = R"(["a",123,true,[1,2,3],null])";
				string err;
				RpcValue cp1 = RpcValue::parseCpon(s, &err);
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << s << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toList() == cp2.toList());
			}
			{
				RpcValue cp1{RpcValue::List{1,2,3}};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toList() == cp2.toList());
			}
			{
				static constexpr size_t N = 10;
				std::stringstream out;
				ChainPackProtocol::writeContainerBegin(out, RpcValue::Type::List);
				std::string s("foo-bar");
				for (size_t i = 0; i < N; ++i) {
					ChainPackProtocol::writeListElement(out, RpcValue(s + shv::chainpack::Utils::toString(i)));
				}
				ChainPackProtocol::writeContainerEnd(out);
				//out.exceptions(std::iostream::eofbit);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << cp2.toCpon() << " dump: " << binary_dump(out.str()).c_str();
				const RpcValue::List list = cp2.toList();
				QVERIFY(list.size() == N);
				for (size_t i = 0; i < list.size(); ++i) {
					QVERIFY(RpcValue(s + shv::chainpack::Utils::toString(i)) == list.at(i));
				}
			}
		}
		{
			qDebug() << "------------- Map";
			{
				RpcValue cp1{{
						{"foo", RpcValue::List{11,12,13}},
						{"bar", 2},
						{"baz", 3},
							 }};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toMap() == cp2.toMap());
			}
			{
				RpcValue::Map m{
						{"foo", RpcValue::List{11,12,13}},
						{"bar", 2},
						{"baz", 3},
							 };
				std::stringstream out;

				ChainPackProtocol::writeContainerBegin(out, RpcValue::Type::Map);
				for(auto it : m)
					ChainPackProtocol::writeMapElement(out, it.first, it.second);
				ChainPackProtocol::writeContainerEnd(out);

				RpcValue::Map m2 = ChainPackProtocol::read(out).toMap();
				for(auto it : m2) {
					QVERIFY(it.second == m[it.first]);
				}
			}
		}
		{
			qDebug() << "------------- IMap";
			{
				RpcValue::IMap map {
					{1, "foo"},
					{2, "bar"},
					{333, 15u},
				};
				RpcValue cp1{map};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toIMap() == cp2.toIMap());
			}
			{
				RpcValue cp1{{
						{127, RpcValue::List{11,12,13}},
						{128, 2},
						{129, 3},
							 }};
				std::stringstream out;
				int len = ChainPackProtocol::write(out, cp1);
				RpcValue cp2 = ChainPackProtocol::read(out);
				qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toIMap() == cp2.toIMap());
			}
		}
		{
			qDebug() << "------------- Meta1";
			RpcValue cp1{RpcValue::String{"META1"}};
			cp1.setMetaValue(meta::Tag::MetaTypeId, 11);
			std::stringstream out;
			int len = ChainPackProtocol::write(out, cp1);
			RpcValue cp2 = ChainPackProtocol::read(out);
			std::ostream::pos_type consumed = out.tellg();
			qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " consumed: " << consumed << " dump: " << binary_dump(out.str());
			qDebug() << "hex:" << hex_dump(out.str());
			QVERIFY(len == (int)consumed);
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.metaData() == cp2.metaData());
		}
		{
			qDebug() << "------------- Meta2";
			RpcValue cp1{RpcValue::List{"META2", 17, 18, 19}};
			cp1.setMetaValue(meta::Tag::MetaTypeNameSpaceId, 12);
			cp1.setMetaValue(meta::Tag::MetaTypeId, 2);
			cp1.setMetaValue(meta::Tag::USER, "foo");
			cp1.setMetaValue(meta::Tag::USER+1, RpcValue::List{1,2,3});
			cp1.setMetaValue("bar", 3);
			std::stringstream out;
			int len = ChainPackProtocol::write(out, cp1);
			RpcValue cp2 = ChainPackProtocol::read(out);
			std::ostream::pos_type consumed = out.tellg();
			qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " consumed: " << consumed << " dump: " << binary_dump(out.str());
			qDebug() << "hex:" << hex_dump(out.str());
			QVERIFY(len == (int)consumed);
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.metaData() == cp2.metaData());
		}
	}

private slots:
	void initTestCase()
	{
		//qDebug("called before everything else");
		shv::chainpack::Exception::setAbortOnException(true);
	}
	void testProtocol()
	{
		textTest();
		binaryTest();
	}

	void cleanupTestCase()
	{
		//qDebug("called after firstTest and secondTest");
	}
};

QTEST_MAIN(TestRpcValue)
#include "tst_chainpack_rpcvalue.moc"
