#pragma once
#include "TcpConnection.h"
#include "MediaSession.h"

constexpr char* RTSPVERSION = "NineTSe_RTSPServer";

class RtspServer;
class RtspConnection : public TcpConnection
{
public:
	enum Method
	{
		OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN, NONE
	};
	RtspConnection(RtspServer* rtspserver, int clientfd);
	virtual ~RtspConnection();
	static RtspConnection* createNew(RtspServer* rtspserver, int clientfd);

protected:
	virtual void handleReadBytes();

private:
	void handleRtpOverTcp();
	bool parseRequest();
	bool parseRequest1(const char* begin, const char* end);
	bool parseRequest2(const char* begin, const char* end);
	bool parseCSeq(std::string& message);
	bool parseDescribe(std::string& message);
	bool parseSetup(std::string& message);
	bool parsePlay(std::string& message);

	bool handleOption();
	bool handleDescribe();
	bool handleSetup();
	bool handlePlay();
	bool handleTeardown();

	int sendMessage(void* buf, int size);

	bool createRtpRtcpOverUdp(MediaSession::TrackId trackid, std::string peerip,
		uint16_t peerrtpport, uint16_t peerrtcpport);
	bool createRtpOverTcp(MediaSession::TrackId trackid, int sockfd, uint8_t rtpchannel);

private:
	RtspServer* m_rtspServer;
	Method m_method;
	MediaSession::TrackId m_trackid;
	int m_sessionid;
	std::string m_streamPrefix;
	bool m_isRtpOverTcp;
	RtpInstance* m_rtpinstances[MAX_TRACK_NUM];
	RtcpInstance* m_rtcpinstances[MAX_TRACK_NUM];
	std::string m_peerip;
	std::string m_suffix; //what
	std::string m_url;
	uint32_t m_cseq;
	uint8_t m_rtpChannel;
	uint16_t m_peerRtpPort;
	uint16_t m_peerRtcpPort;
};