#include "socketrpcdriver.h"

#include <necrolog.h>

#include <cassert>
#include <string.h>

#ifdef FREE_RTOS
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#elif defined __GNUC__
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

namespace cp = shv::chainpack;

namespace shv {
namespace chainpack {

SocketRpcDriver::SocketRpcDriver()
{
}

SocketRpcDriver::~SocketRpcDriver()
{
	closeConnection();
}

void SocketRpcDriver::closeConnection()
{
	if(isOpen())
		::close(m_socket);
	m_socket = -1;
}

bool SocketRpcDriver::isOpen()
{
	return m_socket >= 0;
}

int64_t SocketRpcDriver::writeBytes(const char *bytes, size_t length)
{
	if(!isOpen()) {
		nInfo() << "Write to closed socket";
		return 0;
	}
	flush();
	size_t bytes_to_write_len = (m_writeBuffer.size() + length > m_maxWriteBufferLength)? m_maxWriteBufferLength - m_writeBuffer.size(): length;
	if(bytes_to_write_len > 0)
		m_writeBuffer += std::string(bytes, bytes_to_write_len);
	flush();
	return bytes_to_write_len;
}

bool SocketRpcDriver::flush()
{
	if(m_writeBuffer.empty()) {
		nDebug() << "write buffer is empty";
		return false;
	}
	nDebug() << "Flushing write buffer, buffer len:" << m_writeBuffer.size() << "...";
	nDebug() << "writing to socket:" << Utils::toHex(m_writeBuffer);
	int64_t n = ::write(m_socket, m_writeBuffer.data(), m_writeBuffer.length());
	nDebug() << "\t" << n << "bytes written";
	if(n > 0)
		m_writeBuffer = m_writeBuffer.substr(n);
	return (n > 0);
}

bool SocketRpcDriver::connectToHost(const std::string &host, int port)
{
	closeConnection();
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket < 0) {
		 nError() << "ERROR opening socket";
		 return false;
	}
	{
		struct sockaddr_in serv_addr;
		struct hostent *server;
		server = gethostbyname(host.c_str());

		if (server == NULL) {
			nError() << "ERROR, no such host" << host;
			return false;
		}

		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
		serv_addr.sin_port = htons(port);

		/* Now connect to the server */
		nInfo().nospace() << "connecting to " << host << ":" << port;
		if (::connect(m_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
			nError() << "ERROR, connecting host" << host;
			return false;
		}
	}


	{
		//set_socket_nonblock
		int flags;
		flags = fcntl(m_socket, F_GETFL, 0);
		assert(flags != -1);
		fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
	}

	nInfo() << "... connected";

	return true;
}

void SocketRpcDriver::exec()
{
	//int pfd[2];
	//if (pipe(pfd) == -1)
	//		 return;
	fd_set read_flags,write_flags; // the flag sets to be used
	struct timeval waitd;

	static constexpr size_t BUFF_LEN = 255;
	char in[BUFF_LEN];
	char out[BUFF_LEN];
	memset(&in, 0, BUFF_LEN);
	memset(&out, 0, BUFF_LEN);

	while(1) {
		waitd.tv_sec = 5;
		waitd.tv_usec = 0;

		FD_ZERO(&read_flags);
		FD_ZERO(&write_flags);
		FD_SET(m_socket, &read_flags);
		if(!m_writeBuffer.empty())
			FD_SET(m_socket, &write_flags);
		//FD_SET(STDIN_FILENO, &read_flags);
		//FD_SET(STDIN_FILENO, &write_flags);

		int sel = select(FD_SETSIZE, &read_flags, &write_flags, (fd_set*)0, &waitd);

		//ESP_LOGI(__FILE__, "select returned, number of active file descriptors: %d", sel);
		//if an error with select
		if(sel < 0) {
			nError() << "select failed errno:" << errno;
			return;
		}
		if(sel == 0) {
			nInfo() << "\t timeout";
			idleTaskOnSelectTimeout();
			continue;
		}

		//socket ready for reading
		if(FD_ISSET(m_socket, &read_flags)) {
			nInfo() << "\t read fd is set";
			//clear set
			FD_CLR(m_socket, &read_flags);

			memset(&in, 0, BUFF_LEN);

			auto n = read(m_socket, in, BUFF_LEN);
			nInfo() << "\t " << n << "bytes read";
			if(n <= 0) {
				nError() << "Closing socket";
				closeConnection();
				return;
			}
			onBytesRead(std::string(in, n));
		}

		//socket ready for writing
		if(FD_ISSET(m_socket, &write_flags)) {
			nInfo() << "\t write fd is set";
			FD_CLR(m_socket, &write_flags);
			enqueueDataToSend(Chunk());
		}
	}
}

void SocketRpcDriver::sendResponse(int request_id, const cp::RpcValue &result)
{
	cp::RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setResult(result);
	nInfo() << "sending response:" << resp.toCpon();
	sendRpcValue(resp.value());
}

void SocketRpcDriver::sendNotify(std::string &&method, const cp::RpcValue &result)
{
	nInfo() << "sending notify:" << method;
	cp::RpcNotify ntf;
	ntf.setMethod(std::move(method));
	ntf.setParams(result);
	sendRpcValue(ntf.value());
}

}}
