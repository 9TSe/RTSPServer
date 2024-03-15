#include "RtpServer.h"
#include "Rtp.h"
#include "Rtcp.h"
#include "RtcpContext.h"
#include "Log.h"
#include "Utils.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")
#define CACHE_MAX_SIZE 20000

RtpServer::RtpServer()
{
	m_rtcpContext_forRecv = new RtcpContextForRecv;
	m_recvCache = (uint8_t*)malloc(CACHE_MAX_SIZE);
}

RtpServer::~RtpServer()
{
	delete m_rtcpContext_forRecv;
	m_rtcpContext_forRecv = nullptr;
	free(m_recvCache);
	m_recvCache = nullptr;
}

void RtpServer::Parse_RecvData(char* recvBuf, int recvBuf_size)
{
	if ((m_recvCache_size + recvBuf_size) > CACHE_MAX_SIZE)
	{
		LOGE("Exceed the buffer capacity limit, ignore. m_recvCache_size=%d, recvBuf_size=%d"
			, m_recvCache_size, recvBuf_size);
	}
	else
	{
		memcpy(m_recvCache + m_recvCache_size, recvBuf, recvBuf_size);
		m_recvCache_size += recvBuf_size;
	}
	while (true)
	{
		if (m_recvCache_size > RTP_HEADER_SIZE)
		{
			bool success = false;
			int16_t pktSize; //rtp or rtcp size
			uint32_t index = 0;
			uint8_t magic;
			uint8_t channel;
			for (index = 0; index < (m_recvCache_size - RTP_HEADER_SIZE); ++index)
			{
				magic = m_recvCache[index];
				if (magic == 0x24)
				{
					channel = m_recvCache[index + 1];
					pktSize = ntohs(*(int16_t*)(m_recvCache + index + 2));
					if ((m_recvCache_size - 4) >= pktSize)
						success = true;
					break;
				}
			}
			if (success)
			{
				m_recvCache_size -= 4;
				m_recvCache_size -= pktSize;
				char* pktBuf = (char*)malloc(pktSize);
				memcpy(pktBuf, m_recvCache + index + 4, pktSize);
				memmove(m_recvCache, m_recvCache + index + 4 + pktSize, m_recvCache_size);
				if (channel == 0x00)
				{
					RtpHeader rtpHeader;
					Parse_RtpHeader((uint8_t*)pktBuf, &rtpHeader);
					printf("seq = %d, timestamp = %d, ssrc = %d \n",
						rtpHeader.seq, rtpHeader.timestamp, rtpHeader.ssrc);
					m_rtcpContext_forRecv->On_Rtp(rtpHeader.seq, rtpHeader.timestamp, 0, 90000, pktSize);
				}
				else if (channel == 0x01)
				{
					printf("RTCP magic = %d, channel = %d, m_recvCache_size = %d, pktSize = %d\n",
						magic, channel, m_recvCache_size, pktSize);
					std::vector<RtcpHeader*> rtcps = RtcpHeader::Load_FromBytes(pktBuf, pktSize);
					for (auto& rtcp : rtcps)
					{
						if ((RtcpType)rtcp->pt == RtcpType::RTCP_SR)
						{
							RtcpSR* rtcpSR = (RtcpSR*)rtcp;
							printf("recive rtcp sr start \n");
							std::cout << "ssrc:" << rtcpSR->ssrc << "\n";
							std::cout << "ntpmsw:" << rtcpSR->ntpmsw << "\n";
							std::cout << "ntplsw:" << rtcpSR->ntplsw << "\n";
							std::cout << "rtpts:" << rtcpSR->rtpts << "\n";
							std::cout << "packet_count:" << rtcpSR->packet_count << "\n";
							std::cout << "octet_count:" << rtcpSR->octet_count << "\n";
							printf("recive rtcp sr end\n");
						}
						else
							LOGE("recived undefined type of RTCP");
						m_rtcpContext_forRecv->On_Rtcp(rtcp);
					}
				}
				free(pktBuf);
				pktBuf = nullptr;
			}
			else
				break;
		}
		else
			break;
	}
}

int RtpServer::Start(const char* ip, uint16_t port)
{
	LOGI("rtpserver rtp : %s:%d", ip, port);
	SOCKET serverFd = -1;
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		LOGE("WSAStartup error");
		return -1;
	}
	SOCKADDR_IN serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);

	serverFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (bind(serverFd, (SOCKADDR*)&serverAddr, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		LOGE("socket bind error");
		return -1;
	}

	if (listen(serverFd, SOMAXCONN) < 0)
	{
		LOGE("socket listen error");
		return -1;
	}

	while (true)
	{
		LOGI("block listen new conncet");
		int len = sizeof(SOCKADDR);
		SOCKADDR_IN acceptAddr;
		int clientFd = accept(serverFd, (SOCKADDR*)&acceptAddr, &len);
		if (clientFd == SOCKET_ERROR)
		{
			LOGE("accept connection error");
			break;
		}
		LOGI("new connect, clientFd = %d", clientFd);

		char recvBuf[10000];
		int recvBuf_size;
		uint64_t speedTotal_size = 0;
		time_t t1 = time(nullptr);
		time_t t2 = 0;
		uint64_t lastSend_rrStart = Get_CurTime();
		while (true)
		{
			recvBuf_size = recv(clientFd, recvBuf, sizeof(recvBuf), 0);
			if (recvBuf_size <= 0)
			{
				LOGE("::recv error, clientFd = %d, recvBuf_size = %d", clientFd, recvBuf_size);
				break;
			}
			time_t cur = Get_CurTime();
			if ((cur - lastSend_rrStart) > 1000) //every 1000ms client send once RtcpRR
			{
				lastSend_rrStart = cur;
				RtcpRR* rtcp = m_rtcpContext_forRecv->Create_RtcpRR(m_rtcpSSRC, m_rtpSSRC);
				printf("send to pusher rtcpRR start\n");
				std::cout << "rtcp->Get_Size():" << rtcp->Get_Size() << "\n";
				std::cout << "ssrc:" << ntohl(rtcp->ssrc) << "\n";
				printf("send to pusher rtcpRR end\n");

				uint32_t rtcpSize = rtcp->Get_Size();
				char* rtcpBuf = (char*)malloc(4 + rtcpSize);
				rtcpBuf[0] = 0x24;//$
				rtcpBuf[1] = 0x01;// 0x00;
				rtcpBuf[2] = (uint8_t)(((rtcpSize) & 0xFF00) >> 8);
				rtcpBuf[3] = (uint8_t)((rtcpSize) & 0xFF);
				memcpy(rtcpBuf + 4, (char*)rtcp, rtcpSize);

				int sendRtcp_bufSize = ::send(clientFd, rtcpBuf, 4 + rtcpSize, 0);
				if (sendRtcp_bufSize <= 0)
				{
					LOGE("::send RR error, clientFd = %d, sendRtcp_bufSize = %d", clientFd, sendRtcp_bufSize);
					break;
				}
				free(rtcpBuf);
				rtcpBuf = nullptr;
				delete rtcp;
				rtcp = nullptr;
			}
			speedTotal_size += recvBuf_size;
			if (speedTotal_size > 2097152) // 2 * 1024 * 1024 = 2MB
			{
				t2 = time(nullptr);
				if (t2 - t1 > 0)
				{
					uint64_t speed = speedTotal_size / 1024 / (t2 - t1);
					printf("clientFd = %d, speedTotal_size = %llu, speed = %llu kps\n",
						clientFd, speedTotal_size, speed);
					speedTotal_size = 0;
					t1 = time(nullptr);
				}
			}
			Parse_RecvData(recvBuf, recvBuf_size);
		}
		closesocket(clientFd);
		LOGI("close connect clientFd = %d", clientFd);
	}
	return 0;
}