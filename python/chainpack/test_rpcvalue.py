#!/usr/bin/env python3.6
# -*- coding: utf-8 -*-

try:
	import better_exceptions
except:
	pass
import hypothesis
from hypothesis import given
from hypothesis.strategies import *

from rpcvalue import *

def test1():
	print("============= chainpack binary test ============")

def test2():
	for t in range(TypeInfo.TERMINATION, TypeInfo.TRUE+1):
		print(t, ChainPackProtocol(t), TypeInfo(t))

def test3():
	for t in range(TypeInfo.Null, TypeInfo.MetaIMap+1):
		print(t, ChainPackProtocol(t), TypeInfo(t))

def testNull():
	print("------------- NULL")

def testTinyUint():
	print("------------- tiny uint")
	for n in range(64):
		cp1 = RpcValue(n);
		out = ChainPackProtocol()
		len = out.write(cp1)
		if(n < 10):
			print(n, " ", cp1, " len: ", len, " dump: ", out)
		assert len == 1
		cp2 = out.read()
		assert cp1.type == cp2.type
		assert cp1.value == cp2.value

def testUint():
	print("------------- uint")
	n_max = (1<<18*8)-1
	n = 0
	while n < (n_max/3 - 1):
		cp1 = RpcValue(n, Type.UInt);
		out = ChainPackProtocol()
		l = out.write(cp1)
		print(n, " ", cp1, " len: ", l, " dump: ", out)
		cp2 = out.read()
		cp1.assertEquals(cp2)
		print(n, " ", cp1, " " ,cp2, " len: " ,l)# ," dump: " ,binary_dump(out.str()).c_str();
		assert(cp1.toInt() == cp2.toInt());
		n *= 3
		n += 1

def testInt():
	print("------------- int")
	n_max = (1<<18*8)-1
	n = 0
	neg = True
	while n < (n_max/3 - 1):
		neg = not neg
		if neg:
			ni = -n
		else:
			ni = n
		cp1 = RpcValue(ni, Type.Int);
		out = ChainPackProtocol()
		l = out.write(cp1)
		print(ni, " ", cp1, " len: ", l, " dump: ", out)
		cp2 = out.read()
		print(ni, " ", cp1, " " ,cp2, " len: " ,l)# ," dump: " ,binary_dump(out.str()).c_str();
		cp1.assertEquals(cp2)
		assert(cp1.toInt() == cp2.toInt());
		n *= 3
		n += 1

def testIMap():
	print("------------- IMap")
	map = RpcValue(dict([
		(1, "foo "),
		(2, "bar"),
		(3, "baz")]), Type.IMap)
	cp1 = RpcValue(map)
	out = ChainPackProtocol()
	len = out.write(cp1)
	out2 = ChainPackProtocol(out)
	cp2 = out.read()
	print(cp1, cp2, " len: ", len, " dump: ", out2)
	assert cp1 == cp2

def testIMap2():
	cp1 = RpcValue(dict([
		(127, RpcValue([11,12,13])),
		(128, 2),
		(129, 3)]), Type.IMap)
	out = ChainPackProtocol()
	l: int = out.write(cp1)
	out2 = ChainPackProtocol(out)
	cp2: RpcValue = out.read()
	print(cp1 ,cp2," len: " ,l ," dump: " ,out2);
	cp1.assertEquals(cp2)

def testMeta():
	print("------------- Meta")
	cp1 = RpcValue([17, 18, 19])
	cp1.setMetaValue(meta.Tag.MetaTypeNameSpaceId, RpcValue(1, Type.UInt))
	cp1.setMetaValue(meta.Tag.MetaTypeId, RpcValue(2, Type.UInt))
	cp1.setMetaValue(meta.Tag.USER, "foo")
	cp1.setMetaValue(meta.Tag.USER+1, RpcValue([1,2,3]))
	out = ChainPackProtocol()
	l = out.write(cp1)
	orig_len = len(out)
	orig_out = out
	cp2 = out.read();
	consumed = orig_len - len(out)
	print(cp1, cp2, " len: ", l, " consumed: ", consumed , " dump: " , orig_out)
	assert l == consumed
	cp1.assertEquals(cp2)


def testBlob():
	print("------------- Blob")
	cp1 = RpcValue(b"blob containing\0zero character")
	out = ChainPackProtocol()
	l = out.write(cp1)
	cp2 = out.read()
	print(cp1, " ", cp1, " ", cp2, " len: ", " dump: ", out)#binary_dump(out.str()).c_str();
	cp1.assertEquals(cp2);

def testDateTime():
	print("------------- DateTime")
	for dt in [
				#UtcAndTz(datetime(2243, 1, 1, 0, 0, 0, 1000, tzinfo=timezone.utc),0),#https://bugs.python.org/issue23517
				UtcAndTz(datetime(1978, 7, 6, 4, 51, 44, 30000, tzinfo=timezone.utc),52),
				UtcAndTz(datetime(2018,2,2, 0,0,0,    0,timezone.utc), -4),
				UtcAndTz(datetime(2018,2,2, 0,0,0,    1000,timezone.utc), 0),
				UtcAndTz(datetime(2017,5,3, 5,52,3,   0,timezone.utc), 0),
				UtcAndTz(datetime(2017,5,3, 15,52,3,  923000,timezone.utc)),
				UtcAndTz(datetime(2017,5,3, 15,52,31, 0,timezone.utc), 10),
				UtcAndTz(datetime(2017,5,3, 15,52,3,  0,timezone.utc)),
				UtcAndTz(datetime(2017,5,3, 15,52,3,  0,timezone.utc), 1),
				UtcAndTz(datetime(2017,5,3, 15,52,3,  0,timezone.utc), -1),
				UtcAndTz(datetime(2017,5,3, 15,52,3,  0,timezone.utc), 63),
				UtcAndTz(datetime(2017,5,3, 15,52,3,  0,timezone.utc), -64),
				UtcAndTz(datetime(2017,5,3, 15,52,3,  923000,timezone.utc)),
				]:
					cp1 = RpcValue(dt)
					out = ChainPackProtocol()
					len = out.write(cp1)
					out2 = ChainPackProtocol(out[:])
					cp2 = out2.read()
					print(dt, cp1, cp2, " len: " ,len ," dump: " ,out);
					cp1.assertEquals(cp2)

def testArray():
	print("------------- Array")
	cp1 = RpcValueArray(Type.Int)
	cp1._value.append(RpcValue(11));
	cp1._value.append(RpcValue(12));
	cp1._value.append(RpcValue(13));
	cp1._value.append(RpcValue(14));
	out = ChainPackProtocol()
	l: int = out.write(cp1);
	out2 = ChainPackProtocol(out)
	cp2 = out2.read();
	print(cp1, cp2, " len: ", l, " dump: " , out);
	cp1.assertEquals(cp2);
	assert(cp1.toPython() == cp2.toPython());


def round_trip(x):
	print("encoding:",x)
	encoded = ChainPackProtocol(x)
	print("encoded:",encoded)
	decoded = encoded.read()
	print("decoded:",decoded)
	return decoded


@composite
def utc_and_tz(draw, dt=datetimes(min_value=datetime(1970,1,1,0,0), max_value=datetime(2242, 1,1,0,0)), tz=integers(-64, 63)):
	dt = draw(dt).replace(tzinfo=timezone.utc)
	return UtcAndTz(dt.replace(microsecond=trunc(dt.microsecond/1000)*1000), draw(tz))

#hypothesis_test_integers = integers()
hypothesis_test_integers = integers(-2147483647, 2147483647)
test_data = recursive(none() | booleans() | floats(allow_nan=False, allow_infinity=False) | hypothesis_test_integers | text() | utc_and_tz(),
lambda children: lists(children) | dictionaries(text(), children))

with hypothesis.settings(verbosity=hypothesis.Verbosity.verbose):
	@given(test_data)
	def test_decode_inverts_encode(s):
		print("------------- Hypothesis")
		s = RpcValue(s)
		assert round_trip(s) == s

	@given(dictionaries(integers(min_value=1), test_data), test_data)
	def test_decode_inverts_encode2(md, s):
		print("------------- Hypothesis with meta")
		s = RpcValue(s)
		for k,v in md.items():
			if k in [meta.Tag.MetaTypeId, meta.Tag.MetaTypeNameSpaceId]:
				if (type(v) != int) or (v < 0):
					should_fail = True
			s.setMetaValue(k, v)
		try:
			assert round_trip(s) == s
		except ChainpackException as e:
			if not should_fail:
				raise

