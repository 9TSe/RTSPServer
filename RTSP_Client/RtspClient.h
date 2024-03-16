#pragma once
#include <string>

struct SdpTrack;
class Sdp;

struct RtpContext
{
	int payload;//96,97,,,
	const char* encoding;//H264,H265,,,

	FILE* out_f;
	void* decoder;

	size_t size;
	uint8_t packet[64 * 1024];
};


class RtspClient
{
public:
	explicit RtspClient(const char* transport, const char* url);
	~RtspClient();

	bool Connect_Server();// 第一步 连接服务器
	void Start_Cmd();// 第二步 进行RTSP的信令交互，交互成功则进入接收数据的循环

private:
	void Parse_Data();
	bool Parse_Packet(char channel, char* packet, int size);

	bool SendCmd_Options(int cseq);
	bool SendCmd_Describe(int cseq);
	bool SendCmd_Setup(int cseq, SdpTrack* track);
	bool SendCmd_Play(int cseq);
	bool SendCmd_OverTcp(char* buf, size_t size);

private:
	RtpContext* m_rtpCtx;

	std::string m_userAgent;
	std::string m_transport;// tcp or udp
	std::string m_rtspUrl; // rtsp请求发起的url 格式：rtsp://ip:port/app/name
	std::string m_ip;  // rtsp请求发起的url提取的ip
	uint16_t    m_port;// rtsp请求发起的url提取的port
	/*
	rtsp请求url提取的mediaRoute，不同的rtsp服务器，mediaRoute格式是不同的
	例如：
	rtsp://127.0.0.1:554/live/test     提取的mediaRoute live/test
	rtsp://127.0.0.1:554/v9URTioEFB87  提取的mediaRoute v9URTioEFB87
	*/
	std::string m_mediaRoute;
	int  m_fd = -1;
	char m_bufSnd[1000] = { 0 };
	std::string m_sessionID; // rtsp请求发起后，服务端返回唯一标识
	Sdp* m_sdp;
};