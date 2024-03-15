#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <thread>
#include "Rtcp.h"
#include "Rtp.h"
#include "RtcpContext.h"
#include "Log.h"
#include "Utils.h"
#include "RtpClient.h"
#pragma comment(lib, "ws2_32.lib")

#define CACHE_MAX_SIZE 20000

RtpClient::RtpClient()
{
	m_rtcpContext_forSend = new RtcpContextForSend;
	m_recvCache = (uint8_t*)malloc(CACHE_MAX_SIZE);
}

RtpClient::~RtpClient()
{
	delete m_rtcpContext_forSend;
	m_rtcpContext_forSend = nullptr;
	free(m_recvCache);
	m_recvCache = nullptr;
}

void RtpClient::Parse_RecvData(char* recvBuf, int recvBuf_size)
{
	if ((m_recvCache_size + recvBuf_size) > CACHE_MAX_SIZE)
		LOGE("read data exceed buffer limit, m_recvCache=%d, recvBuf_size=%d", m_recvCache_size, recvBuf_size);
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
			int16_t pktSize;
			uint32_t index = 0;
			uint8_t magic;
			uint8_t channel;
			for (index = 0; index < (m_recvCache_size - RTP_HEADER_SIZE); ++index)
			{
				magic = m_recvCache[index];
				//if (magic == 0x24)
				if(0x24 == magic)
				{
					channel = m_recvCache[index + 1];
					pktSize = ntohs(*(int16_t*)(m_recvCache + index + 2)); // +2 ?
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
				//if (channel == 0x00)
				if(0x00 == channel)
				{
					//RTP
				}
				//else if (channel == 0x01)
				else if( 0x01 == channel)
				{
					//RTCP
					printf("RTCP magic=%d,channel=%d,mRecvCacheSize=%d,pktSize=%d\n", magic, channel, m_recvCache_size, pktSize);
					//std::vector<RtcpHeader*> rtcpArr = RtcpHeader::Load_FromBytes(pktBuf, pktSize);
					auto rtcpArr = RtcpHeader::Load_FromBytes(pktBuf, pktSize);
					for (auto& rtcp : rtcpArr)
						m_rtcpContext_forSend->On_Rtcp(rtcp);

					RtcpSR* rtcp = m_rtcpContext_forSend->Create_RtcpSR(m_rtpSsrc);
					printf("send reciver rtcp sr start\n");
					std::cout << "ssrc:" << ntohl(rtcp->ssrc) << "\n";
					std::cout << "ntpmsw:" << ntohl(rtcp->ntpmsw) << "\n";
					std::cout << "ntplsw:" << ntohl(rtcp->ntplsw) << "\n";
					std::cout << "rtpts:" << ntohl(rtcp->rtpts) << "\n";
					std::cout << "packet_count:" << ntohl(rtcp->packet_count) << "\n";
					std::cout << "octet_count:" << ntohl(rtcp->octet_count) << "\n";
					printf("send reciver rtcp sr end\n");

					uint32_t rtcpSize = rtcp->Get_Size();
					char* rtcpBuf = (char*)malloc(4 + rtcpSize);
					rtcpBuf[0] = 0x24;//$
					rtcpBuf[1] = 0x01;// 0x00;
					rtcpBuf[2] = (uint8_t)(((rtcpSize) & 0xFF00) >> 8);
					rtcpBuf[3] = (uint8_t)((rtcpSize) & 0xFF);
					memcpy(rtcpBuf + 4, (char*)rtcp, rtcpSize);

					int sendRtcp_bufSize = ::send(m_connFd, rtcpBuf, 4 + rtcpSize, 0);
					if (sendRtcp_bufSize <= 0)
					{
						LOGE("::send sr error : mConnFd=%d,sendRtcpBufSize=%d", m_connFd, sendRtcp_bufSize);
						break;
					}
					free(rtcpBuf);
					rtcpBuf = nullptr;
					delete rtcp;
					rtcp = nullptr;
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

int RtpClient::Start(const char* serverIp, uint16_t serverPort)
{
	LOGI("rtpClient start,serverIp=%s,serverPort=%d", serverIp, serverPort);
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		LOGE("WSAStartup error");
		return -1;
	}

	m_connFd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_connFd == -1)
	{
		LOGE("create socket error");
		WSACleanup();
		return -1;
	}

	int on = 1;
	setsockopt(m_connFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	//serverAddr.sin_addr.s_addr = inet_addr(serverIp);
	inet_pton(AF_INET, serverIp, &serverAddr.sin_addr);

	if (connect(m_connFd, (sockaddr*)&serverAddr, sizeof(sockaddr_in)) == -1)
	{
		LOGE("socket connect error");
		return -1;
	}
	LOGI("m_connFd=%d connect success", m_connFd);

	//send thread
	std::thread t1([&]()
		{
			RtpPacket* rtpPacket = (RtpPacket*)malloc(500000);
			RtpHeader_Init(rtpPacket, 0, 0, 0, RTP_VERSION, RTP_PAYLOAD_TYPE_H264, 0, 0, 0, m_rtpSsrc);
			char frame[4000];
			int frameSize = sizeof(frame);
			memset(frame, 0, frameSize);

			while (true)
			{
				rtpPacket->rtpHeader.seq++;
				rtpPacket->rtpHeader.timestamp += 90000 / 25;
				memcpy(rtpPacket->payload, frame, frameSize);
				uint32_t rtpSize = RTP_HEADER_SIZE + frameSize;
				//before Rtp send
				rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
				rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
				rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);

				char* tempBuf = (char*)malloc(4 + rtpSize);
				tempBuf[0] = 0x24;
				tempBuf[1] = 0x00;// 0x00;
				tempBuf[2] = (uint8_t)(((rtpSize) & 0xFF00) >> 8);
				tempBuf[3] = (uint8_t)((rtpSize) & 0xFF);
				memcpy(tempBuf + 4, (char*)rtpPacket, rtpSize);

				int sendRtp_size = ::send(m_connFd, tempBuf, 4 + rtpSize, 0);
				free(tempBuf);
				tempBuf = nullptr;

				if (sendRtp_size <= 0)
				{
					LOGE("::send rtp error: m_connFd = %d, sendRtp_size = %d", m_connFd, sendRtp_size);
					break;
				}
				rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
				rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
				rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);

				//set rtcp sr
				uint64_t ntpStampMs = Get_CurrentMillisecond();
				m_rtcpContext_forSend->On_Rtp(rtpPacket->rtpHeader.seq,
					rtpPacket->rtpHeader.timestamp, ntpStampMs, 90000, rtpSize);

				Sleep(40);
			}
		});

	//recv thread
	std::thread t2([&]()
		{
			char recvBuf[1000];
			int recvBuf_size;
			while (true)
			{
				recvBuf_size = recv(m_connFd, recvBuf, sizeof(recvBuf), 0);
				if (recvBuf_size <= 0)
				{
					LOGE("::recv error: m_connFd = %d, recvBuf_size = %d", m_connFd, recvBuf_size);
					break;
				}
				Parse_RecvData(recvBuf, recvBuf_size);
			}
		});
	t1.join();
	t2.join();
	closesocket(m_connFd);
	m_connFd;
	return 0;
}
