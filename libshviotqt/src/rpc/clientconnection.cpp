#include "clientconnection.h"

#include "clientappclioptions.h"
#include "rpc.h"
#include "socket.h"
#include "socketrpcconnection.h"

#include <shv/coreqt/log.h>

#include <shv/core/exception.h>

#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/rpcmessage.h>

#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QCryptographicHash>
#include <QThread>

#include <fstream>

#define logRpcMsg() shvCDebug("RpcMsg")
//#define logRpcSyncCalls() shvCDebug("RpcSyncCalls")

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

ClientConnection::ClientConnection(QObject *parent)
	: Super(parent)
	, m_loginType(IRpcConnection::LoginType::Sha1)
{
	//setConnectionType(cp::Rpc::TYPE_CLIENT);

	connect(this, &SocketRpcConnection::socketConnectedChanged, this, &ClientConnection::onSocketConnectedChanged);

	m_checkConnectedTimer = new QTimer(this);
	m_checkConnectedTimer->setInterval(10*1000);
	connect(m_checkConnectedTimer, &QTimer::timeout, this, &ClientConnection::checkBrokerConnected);
}

ClientConnection::~ClientConnection()
{
	shvDebug() << __FUNCTION__;
	abort();
}

void ClientConnection::setCliOptions(const ClientAppCliOptions *cli_opts)
{
	if(!cli_opts)
		return;

	setCheckBrokerConnectedInterval(cli_opts->reconnectInterval() * 1000);

	//if(cli_opts->isMetaTypeExplicit_isset())
	//	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());
	if(cli_opts->defaultRpcTimeout_isset()) {
		cp::RpcDriver::setDefaultRpcTimeoutMsec(cli_opts->defaultRpcTimeout() * 1000);
		shvInfo() << "Default RPC timeout set to:" << cp::RpcDriver::defaultRpcTimeoutMsec() << "msec.";
	}

	const std::string pv = cli_opts->protocolType();
	if(pv == "cpon")
		setProtocolType(shv::chainpack::Rpc::ProtocolType::Cpon);
	else if(pv == "jsonrpc")
		setProtocolType(shv::chainpack::Rpc::ProtocolType::JsonRpc);
	else
		setProtocolType(shv::chainpack::Rpc::ProtocolType::ChainPack);

	setHost(cli_opts->serverHost());
	setPort(cli_opts->serverPort());
	setUser(cli_opts->user());
	setPassword(cli_opts->password());
	if(password().empty() && !cli_opts->passwordFile().empty()) {
		std::ifstream is(cli_opts->passwordFile(), std::ios::binary);
		if(is) {
			std::string pwd;
			is >> pwd;
			setPassword(pwd);
		}
		else {
			shvError() << "Cannot open password file";
		}
	}
	shvDebug() << cli_opts->loginType() << "-->" << (int)shv::chainpack::IRpcConnection::loginTypeFromString(cli_opts->loginType());
	setLoginType(shv::chainpack::IRpcConnection::loginTypeFromString(cli_opts->loginType()));

	m_heartbeatInterval = cli_opts->heartbeatInterval();
	{
		cp::RpcValue::Map opts;
		opts[cp::Rpc::OPT_IDLE_WD_TIMEOUT] = 3 * m_heartbeatInterval;
		setConnectionOptions(opts);
	}
}

void ClientConnection::setTunnelOptions(const chainpack::RpcValue &opts)
{
	shv::chainpack::RpcValue::Map conn_opts = connectionOptions().toMap();
	conn_opts[cp::Rpc::KEY_TUNNEL] = opts;
	setConnectionOptions(conn_opts);
}

void ClientConnection::open()
{
	if(!hasSocket()) {
		TcpSocket *socket = new TcpSocket(new QTcpSocket());
		setSocket(socket);
	}
	checkBrokerConnected();
	if(m_checkBrokerConnectedInterval > 0)
		m_checkConnectedTimer->start(m_checkBrokerConnectedInterval);
}

void ClientConnection::close()
{
	m_checkConnectedTimer->stop();
	closeConnection();
}

void ClientConnection::abort()
{
	m_checkConnectedTimer->stop();
	abortConnection();
}

void ClientConnection::setCheckBrokerConnectedInterval(int ms)
{
	m_checkBrokerConnectedInterval = ms;
	if(ms == 0)
		m_checkConnectedTimer->stop();
	else
		m_checkConnectedTimer->setInterval(ms);
}

void ClientConnection::resetConnection()
{
	abortConnection();
}

void ClientConnection::sendMessage(const cp::RpcMessage &rpc_msg)
{
	logRpcMsg() << cp::RpcDriver::SND_LOG_ARROW << rpc_msg.toCpon();
	sendRpcValue(rpc_msg.value());
}

void ClientConnection::onRpcMessageReceived(const chainpack::RpcMessage &msg)
{
	logRpcMsg() << cp::RpcDriver::RCV_LOG_ARROW << msg.toCpon();
	if(isInitPhase()) {
		processInitPhase(msg);
		return;
	}
	if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		if(rp.requestId() == m_connectionState.pingRqId) {
			m_connectionState.pingRqId = 0;
			return;
		}
	}
	emit rpcMessageReceived(msg);
}

void ClientConnection::onRpcValueReceived(const chainpack::RpcValue &rpc_val)
{
	cp::RpcMessage msg(rpc_val);
	onRpcMessageReceived(msg);
}

void ClientConnection::sendHello()
{
	setBrokerConnected(false);
	m_connectionState.helloRequestId = callMethod(cp::Rpc::METH_HELLO);
}

void ClientConnection::sendLogin(const shv::chainpack::RpcValue &server_hello)
{
	m_connectionState.loginRequestId = callMethod(cp::Rpc::METH_LOGIN, createLoginParams(server_hello));
}

void ClientConnection::checkBrokerConnected()
{
	//shvWarning() << "check: " << isSocketConnected();
	if(!isBrokerConnected()) {
		abortConnection();
		shvInfo().nospace() << "connecting to: " << user() << "@" << host() << ":" << port();
		m_connectionState = ConnectionState();
		connectToHost(QString::fromStdString(host()), port());
	}
}

void ClientConnection::setBrokerConnected(bool b)
{
	if(b != m_connectionState.isBrokerConnected) {
		m_connectionState.isBrokerConnected = b;
		if(b) {
			shvInfo() << "Connected to broker";// << "client id:" << brokerClientId();// << "mount point:" << brokerMountPoint();
			if(m_heartbeatInterval > 0) {
				if(!m_pingTimer) {
					shvInfo() << "Creating heart-beat timer, interval:" << m_heartbeatInterval << "sec.";
					m_pingTimer = new QTimer(this);
					m_pingTimer->setInterval(m_heartbeatInterval * 1000);
					connect(m_pingTimer, &QTimer::timeout, this, [this]() {
						if(m_connectionState.pingRqId > 0) {
							shvError() << "PING response not received within" << (m_pingTimer->interval() / 1000) << "seconds, restarting conection to broker.";
							resetConnection();
						}
						else {
							m_connectionState.pingRqId = callShvMethod(".broker/app", cp::Rpc::METH_PING);
						}
					});
				}
				m_pingTimer->start();
			}
		}
		else {
			if(m_pingTimer)
				m_pingTimer->stop();
		}
		emit brokerConnectedChanged(b);
	}
}

void ClientConnection::emitInitPhaseError(const std::string &err)
{
	Q_EMIT brokerLoginError(err);
}

void ClientConnection::onSocketConnectedChanged(bool is_connected)
{
	setBrokerConnected(false);
	if(is_connected) {
		shvInfo() << "Socket connected to RPC server";
		//sendKnockKnock(cp::RpcDriver::ChainPack);
		//RpcResponse resp = callMethodSync("echo", "ahoj babi");
		//shvInfo() << "+++" << resp.toStdString();
		clearBuffers();
		sendHello();
	}
	else {
		shvInfo() << "Socket disconnected from RPC server";
	}
}

static std::string sha1_hex(const std::string &s)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(s.data(), s.length());
	return std::string(hash.result().toHex().constData());
}

chainpack::RpcValue ClientConnection::createLoginParams(const chainpack::RpcValue &server_hello)
{
	shvDebug() << server_hello.toCpon() << "login type:" << (int)loginType();
	std::string pass;
	if(loginType() == chainpack::IRpcConnection::LoginType::Sha1) {
		std::string server_nonce = server_hello.toMap().value("nonce").toString();
		std::string pwd = password();
		if(pwd.size() > 0 && pwd.size() < 40)
			pwd = sha1_hex(pwd); /// SHA1 password must be 40 chars long, it is considered to be plain if shorter
		std::string pn = server_nonce + pwd;
		QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
		hash.addData(pn.data(), pn.length());
		QByteArray sha1 = hash.result().toHex();
		pass = std::string(sha1.constData(), sha1.size());
	}
	else if(loginType() == chainpack::IRpcConnection::LoginType::Plain) {
		pass = password();
		shvDebug() << "plain password:" << pass;
	}
	else {
		shvError() << "Login type:" << chainpack::IRpcConnection::loginTypeToString(loginType()) << "not supported";
	}
	//shvWarning() << password << sha1;
	return cp::RpcValue::Map {
		{"login", cp::RpcValue::Map {
			{"user", user()},
			{"password", pass},
			//{"passwordFormat", chainpack::AbstractRpcConnection::passwordFormatToString(password_format)},
			{"type", chainpack::IRpcConnection::loginTypeToString(loginType())},
		 },
		},
		{"options", connectionOptions()},
	};
}

void ClientConnection::processInitPhase(const chainpack::RpcMessage &msg)
{
	do {
		if(!msg.isResponse())
			break;
		cp::RpcResponse resp(msg);
		//shvInfo() << "Handshake response received:" << resp.toCpon();
		if(resp.isError()) {
			emitInitPhaseError(resp.error().message());
			break;
		}
		int id = resp.requestId().toInt();
		if(id == 0)
			break;
		if(m_connectionState.helloRequestId == id) {
			sendLogin(resp.result());
			return;
		}
		else if(m_connectionState.loginRequestId == id) {
			m_connectionState.loginResult = resp.result();
			setBrokerConnected(true);
			return;
		}
	} while(false);
	shvError() << "Invalid handshake message! Dropping connection." << msg.toCpon();
	resetConnection();
}

}}}


