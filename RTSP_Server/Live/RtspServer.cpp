#include "RtspServer.h"
#include "InetAddress.h"
#include "../Scheduler/UsageEnvironment.h"
#include "../Scheduler/Log.h"
#include "../Scheduler/EventScheduler.h"
#include "../Scheduler/Event.h"
#include "SocketOps.h"
#include "RtspConnection.h"

RtspServer* RtspServer::createNew(UsageEnvironment* env, MediaSessionManager* ssmgr, IPV4Address& addr)
{
	return new RtspServer(env, ssmgr, addr);
}

RtspServer::RtspServer(UsageEnvironment* env, MediaSessionManager* ssmgr, IPV4Address& addr)
	:m_env(env)
	,m_ssmgr(ssmgr)
	,m_addr(addr)
	,m_listen(false)
{
	m_fd = sockets::tcpSocket();
	sockets::setAddreuse(m_fd, 1);
	if (!sockets::bind(m_fd, addr.getIp(), addr.getPort()))
		return;
	LOGI("rtsp://%s:%d fd=%d", addr.getIp().data(), addr.getPort(), m_fd);

	m_acceptIOEvent = IOEvent::createNew(m_fd, this);
	m_acceptIOEvent->setReadCallback(readCallback); 
	m_acceptIOEvent->enableReadEvent();

	m_closeTriggerEvent = TriggerEvent::createNew(this);
	m_closeTriggerEvent->setTriggerCallback(closeConnectCallback);
}

RtspServer::~RtspServer()
{
	LOGI("~RtspServer()");
	if (m_listen)
		m_env->eventScheduler()->removeIOEvent(m_acceptIOEvent);
	delete m_acceptIOEvent;
	delete m_closeTriggerEvent;
	sockets::close(m_fd);
}

void RtspServer::start()
{
	LOGI("RtspServer start()");
	sockets::listen(m_fd, 60);
	m_listen = true;
	m_env->eventScheduler()->addIOEvent(m_acceptIOEvent);
}

void RtspServer::readCallback(void* arg)
{
	RtspServer* rtsp = (RtspServer*)arg;
	rtsp->handleRead();
}

void RtspServer::handleRead()
{
	int clientfd = sockets::accept(m_fd);
	if (clientfd < 0)
	{
		LOGE("accept error");
		return;
	}
	RtspConnection* conn = RtspConnection::createNew(this, clientfd);
	conn->setDisConnectCallback(disConnectCallbcak, this);
	m_connMap.insert(std::make_pair(clientfd, conn));
}


void RtspServer::disConnectCallbcak(void* arg, int fd)
{
	RtspServer* rtspserver = (RtspServer*)arg;
	rtspserver->handleDisconnect(fd);
}

void RtspServer::handleDisconnect(int fd)
{
	std::lock_guard<std::mutex> mutex(m_mutex);
	m_disConnlist.push_back(fd);
	m_env->eventScheduler()->addTriggerEvent(m_closeTriggerEvent);
}

void RtspServer::closeConnectCallback(void* arg)
{
	RtspServer* rtsp = (RtspServer*)arg;
	rtsp->handleCloseConnect();
}

void RtspServer::handleCloseConnect()
{
	std::lock_guard<std::mutex> mutex(m_mutex);
	for (auto& it : m_disConnlist)
	{
		auto _it = m_connMap.find(it);
		if (_it != m_connMap.end())
		{
			delete _it->second;
			m_connMap.erase(_it);
		}
		else
		{
			LOGE("close connect map not match with list");
			//return; //to ensure all connect will be cleared
		}
	}
	m_disConnlist.clear();
}