#pragma once
#include <string>
#include <map>
#include "MediaSession.h"
#include "TcpConnection.h"

class RtspServer;
class RtspConnection : public TcpConnection
{
public:
	enum Method
	{
		OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN, NONE
	};

	static RtspConnection* Create_New(RtspServer* rtspserver, int clientfd);
	RtspConnection(RtspServer* rtspserver, int clientfd);
	virtual ~RtspConnection();

protected:
	virtual void Handle_ReadBytes();

private:
    bool Parse_Request();
    bool Parse_Request1(const char* begin, const char* end);
    bool Parse_Request2(const char* begin, const char* end);

    bool Parse_CSeq(std::string& message);
    bool Parse_Describe(std::string& message);
    bool Parse_Setup(std::string& message);
    bool Parse_Play(std::string& message);

    bool HandleCmd_Option();
    bool HandleCmd_Describe();
    bool HandleCmd_Setup();
    bool HandleCmd_Play();
    bool HandleCmd_Teardown();

    int Send_Message(void* buf, int size);
    int Send_Message();

    bool CreateRtpRtcp_OverUdp(MediaSession::TrackId trackid, std::string peerip,
        uint16_t peer_rtpport, uint16_t peer_rtcpport);
    bool CreateRtp_OverTcp(MediaSession::TrackId trackid, int sockfd, uint8_t rtpchannel);

    void HandleRtp_OverTcp();

private:
    RtspServer* m_rtspServer;
    std::string m_peerIp;
    Method m_method;
    std::string m_url;
    std::string m_suffix;
    uint32_t m_cseq;
    std::string m_streamPrefix;// 数据流名称（作为拉流服务默认是track）


    uint16_t m_peerRtp_port;
    uint16_t m_peerRtcp_port;

    MediaSession::TrackId m_trackId;// 拉流setup请求时，当前的trackId
    RtpInstance* m_rtpInstances[MEDIA_MAX_TRACK_NUM];
    RtcpInstance* m_rtcpInstances[MEDIA_MAX_TRACK_NUM];

    int m_sessionId;
    bool m_isRtp_overTcp;
    uint8_t m_rtpChannel;

};