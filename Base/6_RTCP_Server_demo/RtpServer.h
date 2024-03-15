#pragma once
#include <stdint.h>

class RtcpContext;

class RtpServer
{
public:
	RtpServer();
	~RtpServer();

	int Start(const char* ip, uint16_t port);
	void Parse_RecvData(char* recvBuf, int recvBufSize);

private:
	RtcpContext* m_rtcpContext_forRecv;
	uint8_t* m_recvCache = nullptr;
	uint64_t m_recvCache_size = 0;
	uint32_t m_rtcpSSRC = 0x09;
	uint32_t m_rtpSSRC = 0x08;
};