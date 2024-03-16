#include "RtspClient.h"
#include <WS2tcpip.h>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include "Sdp.h"
#include "Log.h"
#include <assert.h>
#include <rtp-payload.h>
#include <rtp-profile.h>

static void* rtp_packet_alloc(void* /*param*/, int bytes)
{
	static uint8_t buffer[2 * 1024 * 1024 + 4] = { 0, 0, 0, 1, };
	assert(bytes <= sizeof(buffer) - 4);
	return buffer + 4;
}

static void rtp_packet_free(void* /*param*/, void* /*packet*/)
{}
static int rtp_packet_decode_packet(void* param, const void* packet, int bytes, uint32_t timestamp, int flags)
{
	LOGI("bytes=%d,timestamp=%u,flags=%d", bytes, timestamp, flags);
	static uint8_t buffer[8 * 1024 * 1024];

	assert(bytes + 4 < sizeof(buffer));
	assert(0 == flags);

	static const uint8_t start_code[4] = { 0x00,0x00, 0x00, 0x01 };
	struct RtpContext* ctx = (struct RtpContext*)param;

	size_t buffer_size = 0;

	if (0 == strcmp("H264", ctx->encoding) || 0 == strcmp("H265", ctx->encoding))
	{
		memcpy(buffer, start_code, sizeof(start_code));
		buffer_size += sizeof(start_code);
	}

	memcpy(buffer + buffer_size, packet, bytes);
	buffer_size += bytes;

	uint8_t naluType = buffer[4];

	for (int i = 0; i < 10; i++)
	{
		if (i == 0)
			printf("naluType=%d [%d", naluType, buffer[i]);

		else if (i == 9)
			printf(",%d]\n", buffer[i]);

		else
			printf(",%d", buffer[i]);
	}
	if (0x65 == naluType)
	{
		//针对于当前视频, 特意赋值sps和pps, 若拉流其他视频大概率不行
		uint8_t sps[34] = { 0x00,0x00,0x00,0x01,0x67,0x64,0x00,0x28,0xac,0xd9,0x40,0x78,0x02,0x27,0xe5,0xc0,0x5a,0x80,0x80,0x80 ,0xa0 ,0x00,0x00,0x03,0x00,0x00,0x00,0x00,0x06,0x41,0xe3,0x06,0x32,0xc0 };
		uint8_t pps[10] = { 0x00,0x00,0x00,0x01,0x68,0xeb,0xe3,0xcb,0x22,0xc0 };
		fwrite(sps, 1, sizeof(sps), ctx->out_f);
		fwrite(pps, 1, sizeof(pps), ctx->out_f);
	}
	// TODO:
	// check media file
	fwrite(buffer, 1, buffer_size, ctx->out_f);
	return 0;
}

RtspClient::RtspClient(const char* transport, const char* url) 
	:m_userAgent("BXC_RtspClient"),
	m_transport(transport),
	m_rtspUrl(url) 
{
	char ip[40] = { 0 };
	uint16_t port = 0;
	char mediaRoute[100] = { 0 };

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		LOGE("WSAStartup error");

	if (strncmp(url, "rtsp://", 7) != 0)
		LOGE("parse Url protocol error");

	if (sscanf(url + 7, "%[^:]:%hu/%s", ip, &port, mediaRoute) == 3) {}

	else if (sscanf(url + 7, "%[^/]/%s", ip, mediaRoute) == 2) {}

	else {}
		LOGE("parse Url compontent error");

	m_ip = ip;
	m_port = port;
	m_mediaRoute = mediaRoute;
	m_sdp = new Sdp;
	//解析RTP
	int payload = 96; //目前只解视频流,所以写死了
	const char* encoding = "H264";

	m_rtpCtx = new RtpContext;
	m_rtpCtx->payload = payload;
	m_rtpCtx->encoding = encoding;

	m_rtpCtx->out_f = fopen("out.h264", "wb");

	rtp_packet_setsize(1456); // 1456(live555)

	struct rtp_payload_t handler1;
	handler1.alloc = rtp_packet_alloc;
	handler1.free = rtp_packet_free;
	handler1.packet = rtp_packet_decode_packet;
	m_rtpCtx->decoder = rtp_payload_decode_create(payload, encoding, &handler1, m_rtpCtx);
}

bool RtspClient::Connect_Server()
{
	LOGI("RtspClient::connectServer：rtspUrl=%s", m_rtspUrl.data());

	m_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_fd < 0)
	{
		LOGE("create socket error");
		return false;
	}

	int on = 1;
	setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

	sockaddr_in client_addr;
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.S_un.S_addr = INADDR_ANY;

	if (bind(m_fd, (LPSOCKADDR)&client_addr, sizeof(SOCKADDR)) == -1)
	{
		LOGE("bind error");
		return false;
	}
	struct sockaddr_in m_server_addr;
	m_server_addr.sin_family = AF_INET;
	m_server_addr.sin_port = htons(m_port);
	inet_pton(AF_INET, m_ip.c_str(), &m_server_addr.sin_addr);


	if (connect(m_fd, (struct sockaddr*)&m_server_addr, sizeof(sockaddr_in)) == -1)
	{
		LOGE("connect error %d", WSAGetLastError());
		return false;
	}

	return true;
}


void RtspClient::Start_Cmd()
{
	bool isSendPlay = false;
	int sendCseq = 0; // 与RTSP服务交互过程中Seq的累计值
	int sendTrackNum = 0;

	// RTSP协议交互每次返回的参数解析 start
	int  CSeq = 0;
	char sessionID[50] = { 0 };//从Rtsp响应中获取Session
	int  ResponseStateCode = 0;
	int  ContentLength = 0;
	char ContentBase[100] = { 0 };

	char bufCmdRecv[1000] = { 0 };
	char bufCmdRecvCopy[1000] = { 0 };
	int  bufCmdRecvSize = 0;
	// RTSP协议交互每次返回的参数解析 end

	++sendCseq;

	if (!SendCmd_Options(sendCseq))
		return;

	while (true)
	{
		ResponseStateCode = 0;
		CSeq = 0;
		ContentLength = 0;

		bufCmdRecvSize = recv(m_fd, bufCmdRecv, sizeof(bufCmdRecv), 0);
		if (bufCmdRecvSize <= 0) 
		{
			LOGE("bufCmdRecv error: %d", WSAGetLastError());
			goto FINISH;
		}

		memcpy(bufCmdRecvCopy, bufCmdRecv, bufCmdRecvSize);
		bufCmdRecv[bufCmdRecvSize] = '\0';
		bufCmdRecvCopy[bufCmdRecvSize] = '\0';

		LOGI("bufCmdRecvSize=%d,bufCmdRecv=%s", bufCmdRecvSize, bufCmdRecv);

		const char* sep = "\n";
		char* line = strtok(bufCmdRecv, sep);
		while (line) 
		{
			if (strstr(line, "RTSP/1.0")) 
			{
				if (sscanf(line, "RTSP/1.0 %d OK\r\n", &ResponseStateCode) != 1) 
				{
					LOGE("parse RTSP/1.0 error");
					goto FINISH;
				}
			}
			else if (strstr(line, "Session")) 
			{
				//memset(sessionID, 0, sizeof(sessionID) / sizeof(char));
				if (sscanf(line, "Session: %s\r\n", &sessionID) != 1) 
				{
					LOGE("parse Session error");
					goto FINISH;
				}
				else 
				{
					m_sessionID = sessionID;
				}
			}
			else if (strstr(line, "CSeq")) 
			{
				if (sscanf(line, "CSeq: %d\r\n", &CSeq) != 1) 
				{
					LOGE("parse CSeq error");
					goto FINISH;
				}
			}
			else if (strstr(line, "Content-Base")) 
			{
				if (sscanf(line, "Content-Base: %s\r\n", &ContentBase) != 1) 
				{
					LOGE("parse Content-Base error");
					goto FINISH;
				}

			}
			else if (strstr(line, "Content-length")) 
			{
				if (sscanf(line, "Content-length: %d\r\n", &ContentLength) != 1) 
				{
					LOGE("parse Content-length error");
					goto FINISH;
				}

			}
			else if (strstr(line, "Content-Length")) 
			{
				if (sscanf(line, "Content-Length: %d\r\n", &ContentLength) != 1) 
				{
					LOGE("parse Content-Length error");
					goto FINISH;
				}
			}
			line = strtok(NULL, sep);
		}
		if (200 == ResponseStateCode) 
		{
			if (1 == CSeq) 
			{
				// 解析Options，发送Describe
				++sendCseq;
				if (!SendCmd_Describe(sendCseq))
					goto FINISH;
			}
			else if (2 == CSeq)
			{
				// 解析Sdp
				m_sdp->Parse(bufCmdRecvCopy, bufCmdRecvSize);

				// 发送Setup请求
				SdpTrack* track = m_sdp->Pop_Track();
				if (track) 
				{
					++sendCseq;
					++sendTrackNum;
					SendCmd_Setup(sendCseq, track);
				}
			}
			else if (sendCseq == CSeq && !isSendPlay) 
			{
				// 继续发送Setup请求
				SdpTrack* track = m_sdp->Pop_Track();
				if (track) 
				{
					++sendCseq;
					++sendTrackNum;
					SendCmd_Setup(sendCseq, track);
				}
				else 
				{
					LOGI("发送(%d次)track完成，开始发送Play请求", sendTrackNum);
					++sendCseq;
					if (!SendCmd_Play(sendCseq))
						goto FINISH;
					isSendPlay = true;
				}
			}
			else if (sendCseq == CSeq && isSendPlay) 
			{
				LOGI("接收到Play请求的响应");
				Parse_Data();
				goto FINISH;
			}
			else 
			{
				LOGE("CSeq=%d is error", CSeq);
				goto FINISH;
			}
		}
		else 
		{
			LOGE("Rtsp ResponseStateCode=%d is error", ResponseStateCode);
			goto FINISH;
		}
	}

FINISH:
	LOGI("FINISH");
	return;
}

bool RtspClient::SendCmd_Options(int cseq)
{
	sprintf(m_bufSnd, "OPTIONS rtsp://%s:%d/%s RTSP/1.0\r\n"
		"CSeq: %d\r\n"
		"User-Agent: %s\r\n"
		"\r\n",
		m_ip.data(), m_port, m_mediaRoute.data(),
		cseq,
		m_userAgent.data());

	return SendCmd_OverTcp(m_bufSnd, strlen(m_bufSnd));
}

bool RtspClient::SendCmd_OverTcp(char* buf, size_t size)
{
	LOGI("size=%lld,buf=%s", size, buf);
	int ret = send(m_fd, buf, size, 0);
	if (ret < 0) 
	{
		LOGE("send error: %d", WSAGetLastError());
		return false;
	}
	return true;
}

bool RtspClient::SendCmd_Describe(int cseq)
{
	sprintf(m_bufSnd, "DESCRIBE rtsp://%s:%d/%s RTSP/1.0\r\n"
		"Accept: application/sdp\r\n"
		"CSeq: %d\r\n"
		"User-Agent: %s\r\n"
		"\r\n",
		m_ip.data(), m_port, m_mediaRoute.data(),
		cseq,
		m_userAgent.data());

	return SendCmd_OverTcp(m_bufSnd, strlen(m_bufSnd));
}

bool RtspClient::SendCmd_Setup(int cseq, SdpTrack* track)
{
	int interleaved1 = track->control_id * 2;
	int interleaved2 = interleaved1 + 1;


	sprintf(m_bufSnd, "SETUP rtsp://%s:%d/%s/%s RTSP/1.0\r\n"
		"Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n"
		"CSeq: %d\r\n"
		"User-Agent: %s\r\n"
		"Session: %s\r\n"
		"\r\n",
		m_ip.data(), m_port, m_mediaRoute.data(), track->control,
		interleaved1, interleaved2,
		cseq,
		m_userAgent.data(),
		m_sessionID.data());

	return SendCmd_OverTcp(m_bufSnd, strlen(m_bufSnd));
}

bool RtspClient::SendCmd_Play(int cseq) 
{
	sprintf(m_bufSnd, "PLAY rtsp://%s:%d/%s RTSP/1.0\r\n"
		"Range: npt=0.000-\r\n"
		"CSeq: %d\r\n"
		"User-Agent: %s\r\n"
		"Session: %s\r\n"
		"\r\n",
		m_ip.data(), m_port, m_mediaRoute.data(),
		cseq,
		m_userAgent.data(),
		m_sessionID.data());

	return SendCmd_OverTcp(m_bufSnd, strlen(m_bufSnd));
}

void RtspClient::Parse_Data() {

	char* bufRecv = (char*)malloc(2000);
	int   bufRecvSize = 0;

	while (true)
	{
		bufRecvSize = recv(m_fd, bufRecv, sizeof(bufRecv), 0);
		if (bufRecvSize <= 0) 
		{
			LOGE("bufRecv(1) error: %d", WSAGetLastError());
			return;
		}
		else 
		{
			char* p = bufRecv;
			int size = bufRecvSize;

			while (size != 0)
			{
				if (*p == '$')
				{
					if (size < 4)
					{
						int need = 4 - size;
						while (need)
						{
							bufRecvSize = recv(m_fd, p + size, need, 0);
							if (bufRecvSize <= 0)
							{
								LOGE("recv(2) error: %d", WSAGetLastError());
								return;
							}
							size += bufRecvSize;
							need -= bufRecvSize;
						}
					}
					++p;
					int8_t channel = *p;
					++p;
					int16_t len = ntohs(*(int16_t*)(p));
					p += 2;
					size -= 4;
					if (size >= len)
					{
						this->Parse_Packet(channel, p, len);
						p += len;
						size -= len;
					}
					else
					{
						int need = len - size;
						while (need)
						{
							bufRecvSize = recv(m_fd, p + size, need, 0);

							if (bufRecvSize <= 0)
							{
								LOGE("recv(3) error: %d", WSAGetLastError());
								return;
							}
							size += bufRecvSize;
							need -= bufRecvSize;
						}
						this->Parse_Packet(channel, p, len);
						p += len;
						size = 0;
					}
				}
				else 
					++p;
			}
		}
	}
	free(bufRecv);
	bufRecv = nullptr;
}

bool RtspClient::Parse_Packet(char channel, char* packet, int size) 
{
	//LOGI("channel=%d,size=%d", channel,size);
	if (0 == channel) 
	{
		m_rtpCtx->size = size;
		memcpy(m_rtpCtx->packet, packet, m_rtpCtx->size);
		rtp_payload_decode_input(m_rtpCtx->decoder, m_rtpCtx->packet, m_rtpCtx->size);

	}
	else 
		LOGI("channel=%d,size=%d", channel, size);
	return true;
}

RtspClient::~RtspClient()
{
	closesocket(m_fd);
	WSACleanup();
	if (m_sdp) 
	{
		delete m_sdp;
		m_sdp = nullptr;
	}
	if (m_rtpCtx) 
	{
		//fclose(ctx.out_f);
		rtp_payload_decode_destroy(m_rtpCtx->decoder);
		delete m_rtpCtx;
		m_rtpCtx = nullptr;
	}
}