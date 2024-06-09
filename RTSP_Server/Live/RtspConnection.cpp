#include "RtspConnection.h"
#include "../Scheduler/Log.h"
#include "RtspServer.h"
#include "MediaSessionManager.h"

static void getPeerName(int fd, std::string& ip)
{
	sockaddr_in addr;
	socklen_t socklen = sizeof(sockaddr_in);
	getpeername(fd, (sockaddr*)&addr, &socklen);
	ip = inet_ntoa(addr.sin_addr);
}

RtspConnection* RtspConnection::createNew(RtspServer* rtspserver, int clientfd)
{
	return new RtspConnection(rtspserver, clientfd);
}

RtspConnection::RtspConnection(RtspServer* rtspserver, int clientfd)
	:TcpConnection(rtspserver->getEnv(), clientfd)
	,m_rtspServer(rtspserver)
	,m_method(Method::NONE)
	,m_trackid(MediaSession::TrackId::TRACK_IDNONE)
	,m_sessionid(rand())
	,m_streamPrefix("track")
	,m_isRtpOverTcp(false)

{
	LOGI("RtspConnection() m_fd = %d", m_fd);
	for (int i = 0; i < MAX_TRACK_NUM; ++i)
	{
		m_rtpinstances[i] = nullptr;
		m_rtcpinstances[i] = nullptr;
	}
	getPeerName(m_fd, m_peerip);
}

RtspConnection::~RtspConnection()
{
	LOGI("~RtspConnection() m_fd = %d", m_fd);
	for (int i = 0; i < MAX_TRACK_NUM; ++i)
	{
		if (m_rtpinstances[i])
		{
			MediaSession* mediasession = m_rtspServer->m_ssmgr->getSession(m_suffix);
			if (!mediasession)
				mediasession->removeRtpInstance(m_rtpinstances[i]);
			delete m_rtpinstances[i];
		}
		if (m_rtcpinstances[i])
			delete m_rtcpinstances[i];

	}
}


void RtspConnection::handleRtpOverTcp()
{
	int num = 0;
	while (true)
	{
		num++;
		uint8_t* buf = reinterpret_cast<uint8_t*>(m_inputbuf.peek());
		uint8_t channel = buf[1];
		uint16_t rtpsize = (buf[2] << 8) | buf[3];
		uint16_t bufsize = 4 + rtpsize;
		if (m_inputbuf.readable() < bufsize) return;
		else
		{
			if (channel == 0 || channel == 2) //rtp
			{
				Rtp rtp;
				parseRtpHeader(buf + 4, &rtp);
				LOGI("num = %d, rtpsize = %d", num, rtpsize);
			}
			else if (channel == 1 || channel == 3)
			{
				RtcpHeader rtcpheader;
				parseRtcpHeader(buf + 4, &rtcpheader);
				LOGI("num = %d, rtcptype = %d, rtpsize = %d", num, rtcpheader.payloadtype, rtpsize);
			}
			m_inputbuf.retrieve(bufsize);
		}

	}
}

bool RtspConnection::parseRequest1(const char* begin, const char* end)
{
	std::string message(begin, end);
	char method[64] = { 0 };
	char url[512] = { 0 };
	char version[64] = { 0 };
	if (sscanf(message.c_str(), "%s %s %s", method, url, version) != 3) return false;

	if (!strcmp(method, "OPTIONS")) m_method = OPTIONS;
	else if (!strcmp(method, "DESCRIBE")) m_method = DESCRIBE;
	else if (!strcmp(method, "SETUP")) m_method = SETUP;
	else if (!strcmp(method, "PLAY")) m_method = PLAY;
	else if (!strcmp(method, "TEARDOWN")) m_method = TEARDOWN;
	else
	{
		m_method = NONE;
		return false;
	}

	if (strncmp(url, "rtsp://", 7) != 0) return false;
	uint16_t port = 0;
	char ip[64] = { 0 };
	char suffix[64] = { 0 };
	//url : rtsp://127.0.0.1:554/live/test/streamid=1 RTSP/1.0\r\n
	if (sscanf(url + 7, "%[^:]:%hu/%s", ip, &port, suffix) == 3) {}
	// rtsp://127.0.0.1/live...
	else if (sscanf(url + 7, "%[^/]/%s", ip, suffix) == 2) port = 554; //rtsp request default port
	else return false;
	m_url = url;
	m_suffix = suffix;
	return true;
}

bool RtspConnection::parseCSeq(std::string& message)
{
	std::size_t pos = message.find("CSeq");
	if (pos != std::string::npos)
	{
		uint32_t cseq = 0;
		sscanf(message.c_str() + pos, "%*[^:]: %u", &cseq);
		m_cseq = cseq;
		return true;
	}
	return false;
}

bool RtspConnection::parseDescribe(std::string& message)
{
	if (message.find("Accept") == std::string::npos || message.find("sdp") == std::string::npos) return false;
	return true;
}

bool RtspConnection::parseSetup(std::string& message)
{
	m_trackid = MediaSession::TRACK_IDNONE;
	for (int i = 0; i < MAX_TRACK_NUM; ++i)
	{
		if (m_url.find(m_streamPrefix + std::to_string(i)) != std::string::npos)
		{
			if (i == 0) m_trackid = MediaSession::TRACK_ID0;
			else if (i == 1) m_trackid = MediaSession::TRACK_ID1;
		}
	}
	if (m_trackid == MediaSession::TRACK_IDNONE) return false;

	std::size_t pos;
	if (message.find("Transport") != std::string::npos)
	{
		if ((pos = message.find("RTP/AVP/TCP")) != std::string::npos)
		{
			m_isRtpOverTcp = true;
			uint8_t rtpchannel, rtcpchannel;
			if (sscanf(message.c_str() + pos, "%*[^;];%*[^;];%*[^=]=%hhu-%hhu", &rtpchannel, &rtcpchannel) != 2) return false;
			m_rtpChannel = rtpchannel;
			return true;
		}
		else if ((pos = message.find("RTP/AVP")) != std::string::npos)
		{
			uint16_t rtpport, rtcpport;
			if (message.find("unicast", pos) != std::string::npos)
			{
				if (sscanf(message.c_str() + pos, "%*[^;];%*[^;];%*[^=]=%hu-%hu", &rtpport, &rtcpport) != 2) 
					return false;
			}
			else if ((message.find("multicast", pos)) != std::string::npos) return true; //multicast
			else return false;
			m_peerRtpPort = rtpport;
			m_peerRtcpPort = rtcpport;
		}
		else return false;
		return true;
	}
	return false;
}

bool RtspConnection::parsePlay(std::string& message)
{
	if (message.find("Session:") != std::string::npos) //dont know Session*:...(or none sessionid),cant parse
		return true;
	return false;
}


bool RtspConnection::parseRequest2(const char* begin, const char* end)
{
	std::string message(begin, end);
	if (!parseCSeq(message)) return false;
	else if (m_method == OPTIONS) return true;
	else if (m_method == DESCRIBE) return parseDescribe(message);
	else if (m_method == SETUP) return parseSetup(message);
	else if (m_method == PLAY) return parsePlay(message);
	else if (m_method == TEARDOWN) return true;
	else return false;
}

bool RtspConnection::parseRequest()
{
	const char* crlf = m_inputbuf.findCRLF();
	if (!crlf) goto end;
	//m_method, m_url, m_suffix
	if (parseRequest1(m_inputbuf.peek(), crlf)) m_inputbuf.retrieveUntil(crlf + 2); //readindex+
	else goto end;

	crlf = m_inputbuf.findLastCRLF();
	if (!crlf) goto end;
	//parse method
	if (parseRequest2(m_inputbuf.peek(), crlf))
	{
		m_inputbuf.retrieveUntil(crlf + 2);
		return true;
	}
	else goto end;

end:
	m_inputbuf.retrieveAll();
	return false;
}


int RtspConnection::sendMessage(void* buf, int size)
{
	LOGI("%s", (char*)buf);
	m_outputbuf.append(buf, size);
	int ret = m_outputbuf.write(m_fd);
	m_outputbuf.retrieveAll();
	return ret;
}

bool RtspConnection::handleOption()
{
	snprintf(m_buffer, sizeof(m_buffer),
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %u\r\n"
		//"Public: DESCRIBE, ANNOUNCE, SETUP, PLAY, RECORD, PAUSE, GET_PARAMETER, TEARDOWN\r\n"
		"Public: DESCRIBE, SETUP, PLAY\r\n"
		"Server: %s\r\n"
		"\r\n", m_cseq, RTSPVERSION);
	if (sendMessage(m_buffer, strlen(m_buffer)) < 0) return false;
	return true;
}

bool RtspConnection::handleDescribe()
{
	MediaSession* session = m_rtspServer->m_ssmgr->getSession(m_suffix);
	if (!session)
	{
		LOGE("in describe, cant find session: %s", m_suffix);
		return false;
	}
	std::string sdp = session->generateSdpDscription();
	memset(m_buffer, 0, sizeof(m_buffer));
	snprintf(m_buffer, sizeof(m_buffer),
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %u\r\n"
		"Content-Length: %u\r\n"
		"Content-Type: application/sdp\r\n"
		"\r\n"
		"%s", m_cseq, sdp.size(), sdp.c_str());
	if (sendMessage(m_buffer, strlen(m_buffer)) < 0) return false;
	return true;
}

bool RtspConnection::createRtpOverTcp(MediaSession::TrackId trackid, int sockfd, uint8_t rtpchannel)
{
	m_rtpinstances[trackid] = RtpInstance::createNewOverTcp(sockfd, rtpchannel);
	return true;
}

bool RtspConnection::createRtpRtcpOverUdp(MediaSession::TrackId trackid, std::string peerip,
	uint16_t peerrtpport, uint16_t peerrtcpport)
{
	if (m_rtpinstances[trackid] || m_rtcpinstances[trackid]) return false;

	uint16_t rtpport, rtcpport;
	int rtpfd, rtcpfd;
	int i;
	for (int i = 0; i < 10; ++i) //retry 10
	{
		if ((rtpfd = sockets::udpSocket()) < 0) return false;
		if ((rtcpfd = sockets::udpSocket()) < 0)
		{
			sockets::close(rtpfd);
			return false;
		}
		uint16_t port = rand() & 0xfffe;
		if (port < 10000) port += 10000;

		rtpport = port;
		rtcpport = port + 1;
		if (!sockets::bind(rtpfd, "0.0.0.0", rtpport))
		{
			sockets::close(rtpfd);
			sockets::close(rtcpfd);
			continue;
		}
		if (!sockets::bind(rtcpfd, "0.0.0.0", rtcpport))
		{
			sockets::close(rtpfd);
			sockets::close(rtcpfd);
			continue;
		}
		break;
	}
	if (i == 10) return false;
	
	m_rtpinstances[trackid] = RtpInstance::createNewOverUdp(rtpfd, rtpport, peerip, peerrtpport);
	m_rtcpinstances[trackid] = RtcpInstance::createNew(rtcpfd, rtcpport, peerip, peerrtcpport);
	return true;
}

bool RtspConnection::handleSetup()
{
	//url : rtsp://127.0.0.1:554/live/test/streamid=1 RTSP/1.0\r\n
	char sessionname[100] = { 0 };
	if (sscanf(m_suffix.c_str(), "%[^/]/", sessionname) != 1) return false; // test/str...
	MediaSession* session = m_rtspServer->m_ssmgr->getSession(sessionname);
	if (!session)
	{
		LOGE("in setup, cant find session: %s", sessionname);
		return false;
	}

	if (m_trackid >= MAX_TRACK_NUM || m_rtpinstances[m_trackid] || m_rtcpinstances[m_trackid]) return false;
	if (session->isStartMulticast())
	{
		snprintf(m_buffer, sizeof(m_buffer),
			"RTSP/1.0 200 OK\r\n"
			"CSeq: %d\r\n"
			"Transport: RTP/AVP;multicast;"
			"destination=%s;source=%s;port=%d-%d;ttl=255\r\n"
			"Session: %08x\r\n"
			"\r\n", m_cseq, session->getMulticastDestAddr().c_str(),
			sockets::getLocalIp().c_str(),
			session->getMulticastDestRtpPort(m_trackid), session->getMulticastDestRtpPort(m_trackid) + 1,
			m_sessionid);
	}
	else
	{
		if (m_isRtpOverTcp)
		{
			createRtpOverTcp(m_trackid, m_fd, m_rtpChannel);
			m_rtpinstances[m_trackid]->setSessionId(m_sessionid);
			session->addRtpInstance(m_trackid, m_rtpinstances[m_trackid]);

			snprintf(m_buffer, sizeof(m_buffer),
				"RTSP/1.0 200 OK\r\n"
				"CSeq: %d\r\n"
				"Server: %s\r\n"
				"Transport: RTP/AVP/TCP;unicast;interleaved=%hhu-%hhu\r\n"
				"Session: %08x\r\n"
				"\r\n", m_cseq, RTSPVERSION, m_rtpChannel, m_rtpChannel + 1, m_sessionid);
		}
		else
		{
			if (!createRtpRtcpOverUdp(m_trackid, m_peerip, m_peerRtpPort, m_peerRtcpPort))
			{
				LOGE("createRtpRtcpOverUdp error");
				return false;
			}
			m_rtpinstances[m_trackid]->setSessionId(m_sessionid);
			m_rtcpinstances[m_trackid]->setSessionId(m_sessionid);
			session->addRtpInstance(m_trackid, m_rtpinstances[m_trackid]);
			snprintf(m_buffer, sizeof(m_buffer),
				"RTSP/1.0 200 OK\r\n"
				"CSeq: %d\r\n"
				"Server: %s\r\n"
				"Transport: RTP/AVP;unicast;client_port=%hu-%hu;server_port=%hu-%hu\r\n"
				"Session: %08x\r\n"
				"\r\n", m_cseq, RTSPVERSION, m_peerRtpPort, m_peerRtcpPort,
				m_rtpinstances[m_trackid]->getlocalport(), m_rtcpinstances[m_trackid]->getlocalport(),
				m_sessionid);
		}
	}
	if (sendMessage(m_buffer, strlen(m_buffer)) < 0) return false;
	return true;
}

bool RtspConnection::handlePlay()
{
	snprintf(m_buffer, sizeof(m_buffer),
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
		"Server: %s\r\n"
		"Range: ntp=0.000-\r\n"
		"Session: %08x; timeout=60\r\n"
		"\r\n", m_cseq, RTSPVERSION, m_sessionid);
	if (sendMessage(m_buffer, strlen(m_buffer)) < 0) return false;
	for (int i = 0; i < MAX_TRACK_NUM; ++i)
	{
		if (m_rtpinstances[i]) m_rtpinstances[i]->setAlive(true);
		if (m_rtcpinstances[i]) m_rtcpinstances[i]->setAlive(true);
	}
	return true;
}

bool RtspConnection::handleTeardown()
{
	LOGI("Tear Down been trigge");
	snprintf(m_buffer, sizeof(m_buffer),
		"RTSP/1.0 200 OK\r\n"
		"CSeq: %d\r\n"
		"Server: %s\r\n"
		"\r\n", m_cseq, RTSPVERSION);
	if (sendMessage(m_buffer, strlen(m_buffer)) < 0) return false;
	return true;
}

void RtspConnection::handleReadBytes()
{
	if (m_isRtpOverTcp)
	{
		if (m_inputbuf.peek()[0] == '$')
		{
			handleRtpOverTcp();
			return;
		}
	}
	if (!parseRequest()) 
	{
		LOGE("parseRequest error");
		goto disconnect;
	}

	switch (m_method)
	{
	case OPTIONS:
		if (!handleOption())
			goto disconnect;
		break;
	case DESCRIBE:
		if (!handleDescribe())
			goto disconnect;
		break;
	case SETUP:
		if (!handleSetup())
			goto disconnect;
		break;
	case PLAY:
		if (!handlePlay())
			goto disconnect;
		break;
	case TEARDOWN:
		if (!handleTeardown())
			goto disconnect;
		break;
	default:
		goto disconnect;
	}
	return;
disconnect:
	handleDisconnect();
}