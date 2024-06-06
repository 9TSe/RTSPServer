#include "TcpConnection.h"
#include "../Scheduler/UsageEnvironment.h"
#include "../Scheduler/Event.h"
#include "../Scheduler/EventScheduler.h"
#include "SocketOps.h"

TcpConnection::TcpConnection(UsageEnvironment* env, int fd)
	:m_env(env)
	,m_fd(fd)
{
	m_clientIOEvent = IOEvent::createNew(m_fd, this);
	m_clientIOEvent->setReadCallback(readCallback);
	m_clientIOEvent->enableReadEvent();
	m_env->eventScheduler()->addIOEvent(m_clientIOEvent);
}

TcpConnection::~TcpConnection()
{
	m_env->eventScheduler()->removeIOEvent(m_clientIOEvent);
	delete m_clientIOEvent;
	sockets::close(m_fd);
}

void TcpConnection::setDisConnectCallback(DisConnectCallback callback, void* arg)
{
	m_disconCallback = callback;
	m_arg = arg;
}

void TcpConnection::handleDisconnect()
{
	if (m_disconCallback)
		m_disconCallback(m_arg, m_fd);
}

void TcpConnection::readCallback(void* arg)
{
	TcpConnection* tcpcon = (TcpConnection*)arg;
	tcpcon->handleRead();
}

void TcpConnection::handleRead()
{
	int ret = m_inputbuf.read(m_fd);
	if (ret < 0)
	{
		LOGE("tcpconnection read error m_fd = %d", m_fd);
		handleDisconnect();
		return;
	}
	handleReadBytes();
}

void TcpConnection::handleReadBytes()
{
	LOGI("handleReadBytes");
	m_inputbuf.retrieveAll();
}