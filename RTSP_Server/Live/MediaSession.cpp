#include "MediaSession.h"
#include "../Scheduler/Log.h"
#include <algorithm>
#include <assert.h>

std::shared_ptr<MediaSession> MediaSession::createNew(const std::string name)
{
	return std::make_shared<MediaSession>(name);
}

MediaSession::MediaSession(const std::string& name)
	:m_sessionname(name)
	,m_isStart_multicast(false)
{
	LOGI("MediaSession() name = %s", name.data());
	m_tracks[0].m_trackid = TRACK_ID0;
	m_tracks[0].m_isalive = false;
	m_tracks[1].m_trackid = TRACK_ID1;
	m_tracks[1].m_isalive = false;

	for (int i = 0; i < MAX_TRACK_NUM; ++i)
	{
		m_multicast_rtpinstances[i] = nullptr;
		m_multicast_rtcpinstances[i] = nullptr;
	}
}

MediaSession::~MediaSession()
{
	LOGI("~MediaSession()");
	for (int i = 0; i < MAX_TRACK_NUM; ++i)
	{
		if (m_multicast_rtpinstances[i])
		{
			this->removeRtpInstance(m_multicast_rtpinstances[i]);
			delete m_multicast_rtpinstances[i];
		}
		if (m_multicast_rtcpinstances[i])
			delete m_multicast_rtcpinstances[i];
	}
	for (int i = 0; i < MAX_TRACK_NUM; ++i)
		if (m_tracks[i].m_isalive)
			delete m_tracks[i].m_sink;
}

bool MediaSession::removeRtpInstance(RtpInstance* rtpinstance)
{
	for (int i = 0; i < MAX_TRACK_NUM; ++i)
	{
		if (!m_tracks[i].m_isalive)
			continue;
		auto it = std::find(m_tracks[i].m_rtpinstancelist.begin(), m_tracks[i].m_rtpinstancelist.end(), rtpinstance);
		if (it == m_tracks[i].m_rtpinstancelist.end()) continue;
		m_tracks[i].m_rtpinstancelist.erase(it);
		return true;
	}
	return false;
}

MediaSession::Track* MediaSession::getTrack(MediaSession::TrackId trackid)
{
	for (int i = 0; i < MAX_TRACK_NUM; ++i)
	{
		if (m_tracks[i].m_trackid == trackid)
			return &m_tracks[i];
	}
	return nullptr;
}

bool MediaSession::addSink(MediaSession::TrackId trackid, Sink* sink)
{
	Track* track = getTrack(trackid);
	if (!track) return false;
	track->m_sink = sink;
	track->m_isalive = true;
	sink->setSessionCallback(sendPacketCallback, this, track);
	return true;
}

void MediaSession::sendPacketCallback(void* arg1, void* arg2, void* packet, Sink::PacketType packettype)
{
	MediaSession* mediasession = (MediaSession*)arg1;
	Track* track = (Track*)arg2;
	RtpPacket* rtppacket = (RtpPacket*)packet;
	mediasession->handleSendPacket(track, rtppacket);
}

void MediaSession::handleSendPacket(MediaSession::Track* track, RtpPacket* rtppacket)
{
	for (auto& it : track->m_rtpinstancelist)
		if (it->getalive())
			it->Send(rtppacket);
}

bool MediaSession::addRtpInstance(MediaSession::TrackId trackid, RtpInstance* rtpinstance)
{
	Track* track = getTrack(trackid);
	if (!track || !track->m_isalive) return false;
	track->m_rtpinstancelist.push_back(rtpinstance);
	return true;
}

uint16_t MediaSession::getMulticastDestRtpPort(MediaSession::TrackId trackid)
{
	if (trackid > TRACK_ID1 || !m_multicast_rtpinstances[trackid])
		return -1;
	return m_multicast_rtpinstances[trackid]->getpeerport();
}

std::string MediaSession::generateSdpDscription()
{
	if (!m_sdp.empty())
		return m_sdp;
	char buf[2048] = { 0 };
	std::string ip = "0.0.0.0";
	snprintf(buf, sizeof(buf),
		"v=0\r\n"
		"o=- 9%ld 1 IN IP4 %s\r\n"
		"t=0 0\r\n"
		"a=control:*\r\n"
		"a=type:broadcast\r\n", (long)time(NULL), ip.c_str());

	if (isStartMulticast())
	{
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			"a=rtcp-unicast: reflection\r\n");
	}

	for (int i = 0; i < MAX_TRACK_NUM; ++i)
	{
		if (!m_tracks[i].m_isalive) continue;
		uint16_t port = 0;
		if (isStartMulticast())
			port = getMulticastDestRtpPort((TrackId)i);

		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s\r\n",
			m_tracks[i].m_sink->getMediaDescription(port).c_str()); //virtual

		if (isStartMulticast())
		{
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), 
				"c=IN IP4 %s/255\r\n", getMulticastDestAddr().c_str());
		}
		else
		{
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				"c=IN IP4 0.0.0.0\r\n");
		}
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			"%s\r\n", m_tracks[i].m_sink->getAtrribute().c_str());
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			"a=control:track%d\r\n", m_tracks[i].m_trackid);
	}
	m_sdp = buf;
	return m_sdp;
}

bool MediaSession::startMulticast()
{
	sockaddr_in sockaddr = { 0 };
	uint32_t range = 0xe8ffffff - 0xe8000100; //232.255.255.255 - 232.0.1.0(avoid bottom ip)
	sockaddr.sin_addr.s_addr = htonl(0xe8000100 + (rand() % range));
	m_multicastAddr = inet_ntoa(sockaddr.sin_addr);

	int rtpsocketfd1 = sockets::udpSocket();
	int rtpsocketfd2 = sockets::udpSocket();
	int rtcpsocketfd1 = sockets::udpSocket();
	int rtcpsocketfd2 = sockets::udpSocket();
	assert(rtpsocketfd1 * rtpsocketfd2 * rtcpsocketfd1 * rtcpsocketfd2 > 0);

	uint16_t port = rand() & 0xfffe;
	if (port < 10000) port += 10000; //avoid conflict

	uint16_t rtpport1 = port;
	uint16_t rtcpport1 = port + 1;
	uint16_t rtpport2 = port + 2;
	uint16_t rtcpport2 = port + 3;

	m_multicast_rtpinstances[TRACK_ID0] = RtpInstance::createNewOverUdp(rtpsocketfd1, 0, m_multicastAddr, rtpport1);
	m_multicast_rtpinstances[TRACK_ID1] = RtpInstance::createNewOverUdp(rtpsocketfd2, 0, m_multicastAddr, rtpport2);
	m_multicast_rtcpinstances[TRACK_ID0] = RtcpInstance::createNew(rtcpsocketfd1, 0, m_multicastAddr, rtcpport1);
	m_multicast_rtcpinstances[TRACK_ID1] = RtcpInstance::createNew(rtcpsocketfd2, 0, m_multicastAddr, rtcpport2);

	this->addRtpInstance(TRACK_ID0, m_multicast_rtpinstances[TRACK_ID0]);
	this->addRtpInstance(TRACK_ID1, m_multicast_rtpinstances[TRACK_ID1]);
	m_multicast_rtcpinstances[TRACK_ID0]->setAlive(true);
	m_multicast_rtcpinstances[TRACK_ID1]->setAlive(true);

	m_isStart_multicast = true;
	return true;
}