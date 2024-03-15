#pragma once
#include <stdint.h>

class RtcpContext;
class RtpClient
{
public:
	RtpClient();
	~RtpClient();
	int Start(const char* serverIp, uint16_t serverPort);
	void Parse_RecvData(char* recvBuf, int recvBuf_size);

private:
	int m_connFd = -1;
	RtcpContext* m_rtcpContext_forSend = nullptr;
	RtcpContext* m_rtcpContext_forRecv = nullptr;
	uint8_t* m_recvCache = nullptr;
	uint64_t m_recvCache_size = 0;
	uint32_t m_rtpSsrc = 0x08;
};