#pragma once
#include <string>
#include <stdint.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#include "InetAddress.h"
#include "SocketsOps.h"
#include "Rtp.h"

class RtpInstance
{
public:
	enum RtpType
	{
		RTP_OVER_UDP,
		RTP_OVER_TCP
	};

	static RtpInstance* CreateNew_OverUdp(int localsockfd, uint16_t localport,
										std::string destip, uint16_t destport)
	{
		return new RtpInstance(localsockfd, localport, destip, destport);
	}

	static RtpInstance* CreateNew_OverTcp(int sockfd, uint8_t rtpchannel)
	{
		return new RtpInstance(sockfd, rtpchannel);
	}

	RtpInstance(int localsockfd, uint16_t localport, const std::string& destip, uint16_t destport)
		:m_rtpType(RTP_OVER_UDP),
		m_sockFd(localsockfd), m_localPort(localport), m_destAddr(destip,destport),
		m_isAlive(false),
		m_sessionId(0),
		m_rtpChannel(0)
	{}

	RtpInstance(int sockfd, uint8_t rtpchannel)
		:m_rtpType(RTP_OVER_TCP),
		m_sockFd(sockfd), m_localPort(0),
		m_isAlive(false),
		m_sessionId(0),
		m_rtpChannel(rtpchannel)
	{}

	~RtpInstance()
	{
		sockets::close(m_sockFd);
	}

	uint16_t Get_LocalPort() const { return m_localPort; }
	uint16_t Get_PeerPort() { return m_destAddr.Get_Port(); }

	int Send(RtpPacket* rtppacket)
	{
		switch (m_rtpType)
		{
		case RtpInstance::RTP_OVER_UDP:
		{
			return Send_OverUdp(rtppacket->m_buf4, rtppacket->m_size);
			break;
		}
		case RtpInstance::RTP_OVER_TCP:
		{
			rtppacket->m_buf[0] = '$';
			rtppacket->m_buf[1] = (uint8_t)m_rtpChannel;
			rtppacket->m_buf[2] = (uint8_t)(((rtppacket->m_size) & 0xff00) >> 8);
			rtppacket->m_buf[3] = (uint8_t)((rtppacket->m_size) & 0xff);
			return Send_OverTcp(rtppacket->m_buf, 4 + rtppacket->m_size);
			break;
		}
		default: {
			return -1;
			break;
			}
		}
	}

	bool Alive() const { return m_isAlive; }
	int Set_Alive(bool alive) { m_isAlive = alive; return 0; }
	void Set_SessionId(uint16_t sessionid) { m_sessionId = sessionid; }
	uint16_t SessionId() const { return m_sessionId; }

private:
	int Send_OverUdp(void* buf, int size)
	{
		return sockets::sendto(m_sockFd, buf, size, m_destAddr.Get_Addr());
	}

	int Send_OverTcp(void* buf, int size)
	{
		return sockets::write(m_sockFd, buf, size);
	}

private:
	RtpType m_rtpType;
	int m_sockFd;
	uint16_t m_localPort;
	Ipv4Address m_destAddr;
	bool m_isAlive;
	uint16_t m_sessionId;
	uint8_t m_rtpChannel;
};

class RtcpInstance
{
public:
	static RtcpInstance* Create_New(int localsockfd, uint16_t localport,
		std::string destip, uint16_t destport)
	{
		return new RtcpInstance(localsockfd, localport, destip, destport);
	}

	RtcpInstance(int localsockfd, uint16_t localport,
		std::string destip, uint16_t destport)
		:m_localSockFd(localsockfd), m_localPort(localport), m_destAddr(destip, destport),
		m_isAlive(false),
		m_sessionId(0)
	{}

	~RtcpInstance()
	{
		sockets::close(m_localSockFd);
	}

	int Send(void* buf, int size)
	{
		return sockets::sendto(m_localSockFd, buf, size, m_destAddr.Get_Addr());
	}

	int Recv(void* buf, int size, Ipv4Address* addr)
	{
		return 0;
	}

	uint16_t Get_LocalPort() const { return m_localPort; }
	int Alive() const { return m_isAlive; }
	int Set_Alive(bool alive) { m_isAlive = alive; return 0; }
	void Set_SessionId(uint16_t sessionid) { m_sessionId = sessionid; }
	uint16_t SessionId() const { return m_sessionId; }

private:
	int m_localSockFd;
	uint16_t m_localPort;
	Ipv4Address m_destAddr;
	bool m_isAlive;
	uint16_t m_sessionId;
};