#pragma once
#include <string>
#include "InetAddress.h"
#include "SocketOps.h"
#include "Rtp.h"
class RtpInstance
{
public:
	enum RtpType
	{
		RTP_OVERUDP,
		RTP_OVERTCP
	};
	static RtpInstance* createNewOverUdp(int fd, uint16_t localport, std::string destip, uint16_t destport)
	{
		return new RtpInstance(fd, localport, destip, destport);
	}
	static RtpInstance* createNewOverTcp(int fd, uint8_t channel)
	{
		return new RtpInstance(fd, channel);
	}
	RtpInstance(int fd, uint16_t localport, const std::string& destip, uint16_t destport)
		:m_rtptype(RTP_OVERUDP)
		,m_localport(localport)
		,m_fd(fd)
		,m_destaddr(destip, destport)
		,m_isAlive(false)
		,m_rtpchannel(0)
		,m_sessionId(0)
	{}
	RtpInstance(int fd, uint8_t channel)
		:m_rtptype(RTP_OVERTCP)
		,m_localport(0)
		,m_fd(fd)
		,m_isAlive(false)
		,m_rtpchannel(channel)
		,m_sessionId(0)
	{}

	~RtpInstance()
	{
		sockets::close(m_fd);
	}

	uint16_t getlocalport() const { return m_localport; }
	uint16_t getpeerport()  { return m_destaddr.getPort(); }
	uint16_t getsessionId() const { return m_sessionId; }
	bool getalive() const { return m_isAlive; }

	void setAlive(bool alive) { m_isAlive = alive; }
	void setSessionId(uint16_t sessionid) { m_sessionId = sessionid; }

	int Send(RtpPacket* rtppacket)
	{
		switch (m_rtptype)
		{
		case RtpInstance::RTP_OVERUDP:
			return sendOverUdp(rtppacket->m_buf4, rtppacket->m_size);
		case RtpInstance::RTP_OVERTCP:

			rtppacket->m_buf[0] = '$';
			rtppacket->m_buf[1] = m_rtpchannel;
			rtppacket->m_buf[2] = (uint8_t)(((rtppacket->m_size) & 0xff00) >> 8);
			rtppacket->m_buf[3] = (uint8_t)((rtppacket->m_size) & 0xff);
			return sendOverTcp(rtppacket->m_buf, 4 + rtppacket->m_size);

		default:
			return -1;
		}
	}

private:
	int sendOverTcp(void* data, int size)
	{
		return sockets::write(m_fd, data, size);
	}

	int sendOverUdp(void* data, int size)
	{
		return sockets::sendto(m_fd, data, size, m_destaddr.getSockAddr());
	}

private:
	RtpType m_rtptype;
	uint16_t m_localport;
	int m_fd;
	IPV4Address m_destaddr;
	bool m_isAlive;
	uint8_t m_rtpchannel;
	uint16_t m_sessionId;
};


class RtcpInstance
{
public:
	static RtcpInstance* createNew(int fd, uint16_t localport, std::string destip, uint16_t destport)
	{
		return new RtcpInstance(fd, localport, destip, destport);
	}
	RtcpInstance(int fd, uint16_t localport, std::string destip, uint16_t destport)
		:m_fd(fd)
		,m_localport(localport)
		,m_destaddr(destip, destport)
		,m_sessionId(0)
		,m_isAlive(false)
	{}

	~RtcpInstance()
	{
		sockets::close(m_fd);
	}

	int Send(void* data, int size)
	{
		return sockets::sendto(m_fd, data, size, m_destaddr.getSockAddr());
	}

	uint16_t getlocalport() { return m_localport; }
	bool getalive() { return m_isAlive; }
	uint16_t getsessionId() { return m_sessionId; }
	void setSessionId(uint16_t sessionid) { m_sessionId = sessionid; }
	void setAlive(bool alive) { m_isAlive = alive; }

private:
	int m_fd;
	uint16_t m_localport;
	IPV4Address m_destaddr;
	uint16_t m_sessionId;
	bool m_isAlive;
};