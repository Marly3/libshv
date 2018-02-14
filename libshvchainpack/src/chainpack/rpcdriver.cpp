#include "rpcdriver.h"
#include "metatypes.h"
#include "chainpack.h"
#include "exception.h"
#include "cponwriter.h"
#include "cponreader.h"
#include "chainpacktokenizer.h"
#include "chainpackreader.h"

#include <necrolog.h>

#include <sstream>
#include <iostream>

#define logRpcMsg() nCDebug("RpcMsg")
#define logRpcData() nCDebug("RpcData")
//#define logRpcSyncCalls() nCDebug("RpcSyncCalls")

namespace shv {
namespace chainpack {

const char * RpcDriver::SND_LOG_ARROW = "<==";
const char * RpcDriver::RCV_LOG_ARROW = "==>";

int RpcDriver::s_defaultRpcTimeout = 5000;

RpcDriver::RpcDriver()
{
}

RpcDriver::~RpcDriver()
{
}

void RpcDriver::sendMessage(const RpcValue &msg)
{
	using namespace std;
	//shvLogFuncFrame() << msg.toStdString();
	logRpcMsg() << SND_LOG_ARROW << msg.toStdString();
	std::string packed_data = codeRpcValue(protocolVersion(), msg);
	logRpcData() << "send message:" << (packed_data.size() > 250? "<... long data ...>" :
				(protocolVersion() == Rpc::ProtocolVersion::Cpon? packed_data: Utils::toHex(packed_data)));

	enqueueDataToSend(Chunk{std::move(packed_data)});
}

void RpcDriver::sendRawData(std::string &&data)
{
	logRpcMsg() << "send raw data: " << (data.size() > 250? "<... long data ...>" : Utils::toHex(data));
	enqueueDataToSend(Chunk{std::move(data)});
}

void RpcDriver::sendRawData(RpcValue::MetaData &&meta_data, std::string &&data)
{
	logRpcMsg() << "send raw meta + data: " << meta_data.toStdString() << (data.size() > 250? "<... long data ...>" : Utils::toHex(data));
	using namespace std;
	//shvLogFuncFrame() << msg.toStdString();
	std::ostringstream os_packed_meta_data;
	switch (protocolVersion()) {
	case Rpc::ProtocolVersion::Cpon: {
		CponWriter wr(os_packed_meta_data);
		wr << meta_data;
		break;
	}
	case Rpc::ProtocolVersion::ChainPack: {
		ChainPackWriter wr(os_packed_meta_data);
		wr << meta_data;
		break;
	}
	default:
		SHVCHP_EXCEPTION("Cannot serialize data without protocol version specified.")
	}
	Rpc::ProtocolVersion packed_data_ver = RpcMessage::protocolVersion(meta_data);
	if(packed_data_ver == protocolVersion()) {
		enqueueDataToSend(Chunk(os_packed_meta_data.str(), std::move(data)));
	}
	else if(packed_data_ver != Rpc::ProtocolVersion::Invalid) {
		// recode data;
		RpcValue val = decodeData(packed_data_ver, data, 0);
		enqueueDataToSend(Chunk(os_packed_meta_data.str(), codeRpcValue(protocolVersion(), val)));
	}
	else {
		SHVCHP_EXCEPTION("Cannot decode data without protocol version specified.")
	}
}

RpcMessage RpcDriver::composeRpcMessage(RpcValue::MetaData &&meta_data, const std::string &data, bool throw_exc)
{
	Rpc::ProtocolVersion packed_data_ver = RpcMessage::protocolVersion(meta_data);
	RpcValue val = decodeData(packed_data_ver, data, 0);
	if(!val.isValid()) {
		const char * msg = "Compose RPC message error.";
		if(throw_exc) {
			SHVCHP_EXCEPTION(msg);
		}
		else {
			nError() << msg;
			return RpcMessage();
		}
	}
	val.setMetaData(std::move(meta_data));
	return RpcMessage(val);
}

void RpcDriver::enqueueDataToSend(RpcDriver::Chunk &&chunk_to_enqueue)
{
	/// LOCK_FOR_SEND lock mutex here in the multithreaded environment
	lockSendQueue();
	if(!chunk_to_enqueue.empty())
		m_chunkQueue.push_back(std::move(chunk_to_enqueue));
	if(!isOpen()) {
		nError() << "write data error, socket is not open!";
		return;
	}
	flush();
	writeQueue();
	/// UNLOCK_FOR_SEND unlock mutex here in the multithreaded environment
	unlockSendQueue();
}

void RpcDriver::writeQueue()
{
	if(m_chunkQueue.empty())
		return;
	logRpcData() << "writePendingData(), queue len:" << m_chunkQueue.size();
	//static int hi_cnt = 0;
	const Chunk &chunk = m_chunkQueue[0];

	if(!m_topChunkHeaderWritten) {
		std::string protocol_version_data;
		{
			std::ostringstream os;
			ChainPackWriter wr(os);
			wr.writeUIntData((unsigned)protocolVersion());
			protocol_version_data = os.str();
		}
		{
			std::ostringstream os;
			ChainPackWriter wr(os);
			wr.writeUIntData(chunk.size() + protocol_version_data.length());
			std::string packet_len_data = os.str();
			auto len = writeBytes(packet_len_data.data(), packet_len_data.length());
			if(len < 0)
				SHVCHP_EXCEPTION("Write socket error!");
			if(len < (int)packet_len_data.length())
				SHVCHP_EXCEPTION("Design error! Chunk length shall be always written at once to the socket");
		}
		{
			auto len = writeBytes(protocol_version_data.data(), protocol_version_data.length());
			if(len < 0)
				SHVCHP_EXCEPTION("Write socket error!");
			if(len != 1)
				SHVCHP_EXCEPTION("Design error! Protocol version shall be always written at once to the socket");
		}
		m_topChunkHeaderWritten = true;
	}
	if(m_topChunkBytesWrittenSoFar < chunk.metaData.size()) {
		m_topChunkBytesWrittenSoFar += writeBytes_helper(chunk.metaData, m_topChunkBytesWrittenSoFar, chunk.metaData.size() - m_topChunkBytesWrittenSoFar);
	}
	if(m_topChunkBytesWrittenSoFar >= chunk.metaData.size()) {
		m_topChunkBytesWrittenSoFar += writeBytes_helper(chunk.data
														 , m_topChunkBytesWrittenSoFar - chunk.metaData.size()
														 , chunk.data.size() - (m_topChunkBytesWrittenSoFar - chunk.metaData.size()));
		//logRpc() << "writeQueue - data len:" << chunk.length() << "start index:" << m_topChunkBytesWrittenSoFar << "bytes written:" << len << "remaining:" << (chunk.length() - m_topChunkBytesWrittenSoFar - len);
	}
	if(m_topChunkBytesWrittenSoFar == chunk.size()) {
		m_topChunkHeaderWritten = false;
		m_topChunkBytesWrittenSoFar = 0;
		m_chunkQueue.pop_front();
	}
}

int64_t RpcDriver::writeBytes_helper(const std::string &str, size_t from, size_t length)
{
	auto len = writeBytes(str.data() + from, length);
	if(len < 0)
		SHVCHP_EXCEPTION("Write socket error!");
	if(len == 0)
		SHVCHP_EXCEPTION("Design error! At least 1 byte of data shall be always written to the socket");
	return len;
}

void RpcDriver::onBytesRead(std::string &&bytes)
{
	logRpcData() << bytes.length() << "bytes of data read";
	m_readData += std::string(std::move(bytes));
	while(true) {
		int len = processReadData(m_readData);
		//shvInfo() << len << "bytes of" << m_readData.size() << "processed";
		if(len > 0)
			m_readData = m_readData.substr(len);
		else
			break;
	}
}

int RpcDriver::processReadData(const std::string &read_data)
{
	logRpcData() << __FUNCTION__ << "data len:" << read_data.length();

	using namespace shv::chainpack;

	std::istringstream in(read_data);

	bool ok;
	int64_t chunk_len = ChainPackTokenizer::readUIntData(in, &ok);
	if(!ok || in.tellg() < 0)
		return 0;

	size_t message_data_len = (size_t)in.tellg() + chunk_len;

	Rpc::ProtocolVersion protocol_version = (Rpc::ProtocolVersion)ChainPackTokenizer::readUIntData(in, &ok);
	if(!ok)
		return 0;

	logRpcData() << "\t chunk len:" << chunk_len << "read_len:" << message_data_len << "stream pos:" << in.tellg();
	if(message_data_len > read_data.length())
		return 0;

	//		 << "reading bytes:" << (protocolVersion() == Cpon? read_data: shv::core::Utils::toHex(read_data));

	RpcValue::MetaData meta_data;
	size_t meta_data_end_pos;
	switch (protocol_version) {
	/*
	case Json: {
		msg = CponProtocol::read(read_data, (size_t)in.tellg());
		if(msg.isMap()) {
			nError() << "JSON message cannot be translated to ChainPack";
		}
		else {
			const RpcValue::Map &map = msg.toMap();
			unsigned id = map.value("id").toUInt();
			RpcValue::String method = map.value("method").toString();
			RpcValue result = map.value("result");
			RpcValue error = map.value("error");
			RpcValue shv_path = map.value("shvPath");
			if(method.empty()) {
				// response
				RpcResponse resp;
				resp.setShvPath(shv_path);
				resp.setId(id);
				if(result.isValid())
					resp.setResult(result);
				else if(error.isValid())
					resp.setError(error.toIMap());
				else
					nError() << "JSON RPC response must contain response or error field";
				msg = resp.value();
			}
			else {
				// request
				RpcRequest rq;
				rq.setShvPath(shv_path);
				rq.setMethod(std::move(method));
				rq.setParams(map.value("params"));
				if(id > 0)
					rq.setId(id);
				msg = rq.value();
			}
		}
		break;
	}
	*/
	case Rpc::ProtocolVersion::Cpon: {
		CponReader rd(in);
		rd >> meta_data;
		meta_data_end_pos = in.eof()? read_data.size(): (size_t)in.tellg();
		break;
	}
	case Rpc::ProtocolVersion::ChainPack: {
		ChainPackReader rd(in);
		rd >> meta_data;
		meta_data_end_pos = in.eof()? read_data.size(): (size_t)in.tellg();
		break;
	}
	default:
		meta_data_end_pos = message_data_len;
		nError() << "Throwing away message with unknown protocol version:" << (unsigned)protocol_version;
		break;
	}
	if(m_protocolVersion == Rpc::ProtocolVersion::Invalid && protocol_version != Rpc::ProtocolVersion::Invalid) {
		// if protocol version is not explicitly specified,
		// it is set from first received message (should be knockknock)
		m_protocolVersion = protocol_version;
	}
	if(meta_data_end_pos < message_data_len)
		onRpcDataReceived(protocol_version, std::move(meta_data), read_data, meta_data_end_pos, message_data_len - meta_data_end_pos);
	return message_data_len;
}

RpcValue RpcDriver::decodeData(Rpc::ProtocolVersion protocol_version, const std::string &data, size_t start_pos)
{
	RpcValue ret;
	try {
		switch (protocol_version) {
		case Rpc::ProtocolVersion::Cpon: {
			std::istringstream in(data);
			in.seekg(start_pos);
			CponReader rd(in);
			rd >> ret;
			break;
		}
		case Rpc::ProtocolVersion::ChainPack: {
			std::istringstream in(data);
			in.seekg(start_pos);
			ChainPackReader rd(in);
			rd >> ret;
			break;
		}
		default:
			nError() << "Don't know how to decode message with unknown protocol version:" << (unsigned)protocol_version;
			break;
		}
	}
	catch(CponReader::ParseException &e) {
		nError() << e.mesage();
	}
	return ret;
}

std::string RpcDriver::codeRpcValue(Rpc::ProtocolVersion protocol_version, const RpcValue &val)
{
	std::ostringstream os_packed_data;
	switch (protocol_version) {
	/*
	case Json: {
		RpcValue::Map json_msg;
		RpcMessage rpc_msg(msg);
		RpcValue shv_path = rpc_msg.shvPath();
		json_msg["shvPath"] = shv_path;
		if(rpc_msg.isResponse()) {
			// response
			json_msg["id"] = rpc_msg.id();
			RpcResponse resp(rpc_msg);
			if(resp.isError())
				json_msg["error"] = resp.error().message();
			else
				json_msg["result"] = resp.result();
		}
		else {
			// request
			RpcRequest rq(rpc_msg);
			json_msg["id"] = rq.id();
			json_msg["method"] = rq.method();
			json_msg["params"] = rq.params();
			if(rpc_msg.isRequest())
				json_msg["id"] = rpc_msg.id();
		}
		shv::chainpack::CponProtocol::write(os_packed_data, json_msg);
		break;
	}
	*/
	case Rpc::ProtocolVersion::Cpon: {
		CponWriter wr(os_packed_data);
		wr << val;
		break;
	}
	case Rpc::ProtocolVersion::ChainPack: {
		ChainPackWriter wr(os_packed_data);
		wr << val;
		break;
	}
	default:
		SHVCHP_EXCEPTION("Cannot serialize data without protocol version specified.")
	}
	return os_packed_data.str();
}

void RpcDriver::onRpcDataReceived(Rpc::ProtocolVersion protocol_version, RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len)
{
	(void)data_len;
	RpcValue msg = decodeData(protocol_version, data, start_pos);
	if(msg.isValid()) {
		msg.setMetaData(std::move(md));
		logRpcMsg() << RCV_LOG_ARROW << msg.toStdString();
		onRpcValueReceived(msg);
	}
	else {
		nError() << "Throwing away message with unknown protocol version:" << (unsigned)protocol_version;
	}
}

void RpcDriver::onRpcValueReceived(const RpcValue &msg)
{
	logRpcData() << "\t message received:" << msg.toCpon();
	//logLongFiles() << "\t emitting message received:" << msg.dumpText();
	if(m_messageReceivedCallback)
		m_messageReceivedCallback(msg);
}

} // namespace chainpack
} // namespace shv
