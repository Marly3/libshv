#pragma once

#include "../shviotqtglobal.h"

#include <QObject>
#include <QAbstractSocket>

class QTcpSocket;

namespace shv {
namespace iotqt {
namespace rpc {

/// wrapper class for QTcpSocket and QWebSocket

class SHVIOTQT_DECL_EXPORT Socket : public QObject
{
	Q_OBJECT
public:
	explicit Socket(QObject *parent = nullptr);

	virtual void connectToHost(const QString &host_name, quint16 port) = 0;

	virtual void close() = 0;
	virtual void abort() = 0;

	virtual QAbstractSocket::SocketState state() const = 0;
	virtual QString errorString() const = 0;

	virtual QHostAddress  peerAddress() const = 0;
	virtual quint16  peerPort() const = 0;

	virtual QByteArray readAll() = 0;
	virtual qint64 write(const char *data, qint64 max_size) = 0;
	//virtual bool flush() = 0;
	virtual void writeMessageBegin() = 0;
	virtual void writeMessageEnd() = 0;

	Q_SIGNAL void connected();
	Q_SIGNAL void disconnected();
	Q_SIGNAL void readyRead();
	//Q_SIGNAL void readyWrite();
	Q_SIGNAL void  bytesWritten(qint64 bytes);

	Q_SIGNAL void  stateChanged(QAbstractSocket::SocketState state);
	Q_SIGNAL void error(QAbstractSocket::SocketError socket_error);
};

class SHVIOTQT_DECL_EXPORT TcpSocket : public Socket
{
	Q_OBJECT

	using Super = Socket;
public:
	TcpSocket(QTcpSocket *socket, QObject *parent = nullptr);

	void connectToHost(const QString &host_name, quint16 port) override;
	void close() override;
	void abort() override;
	QAbstractSocket::SocketState state() const override;
	QString errorString() const override;
	QHostAddress peerAddress() const override;
	quint16 peerPort() const override;
	QByteArray readAll() override;
	qint64 write(const char *data, qint64 max_size) override;
	//bool flush() override;
	void writeMessageBegin() override {}
	void writeMessageEnd() override;
private:
	QTcpSocket *m_socket = nullptr;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv
