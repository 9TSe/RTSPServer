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

	bool Connect_Server();// ��һ�� ���ӷ�����
	void Start_Cmd();// �ڶ��� ����RTSP��������������ɹ������������ݵ�ѭ��

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
	std::string m_rtspUrl; // rtsp�������url ��ʽ��rtsp://ip:port/app/name
	std::string m_ip;  // rtsp�������url��ȡ��ip
	uint16_t    m_port;// rtsp�������url��ȡ��port
	/*
	rtsp����url��ȡ��mediaRoute����ͬ��rtsp��������mediaRoute��ʽ�ǲ�ͬ��
	���磺
	rtsp://127.0.0.1:554/live/test     ��ȡ��mediaRoute live/test
	rtsp://127.0.0.1:554/v9URTioEFB87  ��ȡ��mediaRoute v9URTioEFB87
	*/
	std::string m_mediaRoute;
	int  m_fd = -1;
	char m_bufSnd[1000] = { 0 };
	std::string m_sessionID; // rtsp������󣬷���˷���Ψһ��ʶ
	Sdp* m_sdp;
};