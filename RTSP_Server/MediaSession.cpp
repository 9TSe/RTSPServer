#include "MediaSession.h"
#include "Log.h"
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <algorithm>
#include <assert.h>


MediaSession* MediaSession::Create_New(std::string sessionname)
{
	return new MediaSession(sessionname);
}

MediaSession::MediaSession(const std::string& sessionname)
	:m_sessionName(sessionname),
	m_isStart_Multicast(false)
{
	LOGI("MediaSession() name = %s", sessionname.data());
	m_tracks[0].m_trackId = TrackId0;
	m_tracks[0].m_isAlive = false;
	m_tracks[1].m_trackId = TrackId1;
	m_tracks[1].m_isAlive = false;

	for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
	{
		m_multicast_rtpInstances[i] = nullptr;
		m_multicast_rtcpInstances[i] = nullptr;
	}
}

MediaSession::~MediaSession()
{
	LOGI("~MediaSession()");
	for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
	{
		if (m_multicast_rtpInstances[i])
		{
			this->Remove_RtpInstance(m_multicast_rtpInstances[i]);
			delete m_multicast_rtpInstances[i];
		}
		if (m_multicast_rtcpInstances[i])
		{
			delete m_multicast_rtcpInstances[i];
		}
	}

	for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
	{
		if (m_tracks[i].m_isAlive)
		{
			Sink* sink = m_tracks[i].m_sink;
			delete sink;
		}
	}
}

std::string MediaSession::Generate_SdpDescription()
{
	if (!m_sdp.empty())
		return m_sdp;
	std::string ip = "0.0.0.0";
	char buf[2048] = { 0 };
	snprintf(buf, sizeof(buf),
		"v=0\r\n"
		"o=- 9%ld 1 IN IP4 %s\r\n"
		"t=0 0\r\n"
		"a=control:*\r\n"
		"a=type:broadcast\r\n",
		(long)time(NULL), ip.c_str());

	if (Is_StartMulticast())
	{
		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			"a=rtcp-unicast: reflection\r\n");
	}

	for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
	{
		uint16_t port = 0;
		if (m_tracks[i].m_isAlive != true)
			continue;
		if (Is_StartMulticast())
			port = GetMuliticast_DestRtpPort((TrackId)i);

		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			"%s\r\n", m_tracks[i].m_sink->Get_MediaDescription(port).c_str());

		if (Is_StartMulticast())
		{
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				"c=IN IP4 %s/255\r\n", GetMuliticast_DestAddr().c_str());
		}
		else
		{
			snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
				"c=IN IP4 0.0.0.0\r\n");
		}

		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			"%s\r\n", m_tracks[i].m_sink->Get_Attribute().c_str());

		snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
			"a=control:track%d\r\n", m_tracks[i].m_trackId);
	}
	m_sdp = buf;
	return m_sdp;
}

MediaSession::Track* MediaSession::Get_Track(MediaSession::TrackId trackid)
{
	for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
	{
		if (m_tracks[i].m_trackId == trackid)
			return &m_tracks[i];
	}
	return nullptr;
}

bool MediaSession::Add_Sink(MediaSession::TrackId trackid, Sink* sink)
{
	Track* track = Get_Track(trackid);
	if (!track)
		return false;
	track->m_sink = sink;
	track->m_isAlive = true;

	sink->Set_SessionCallback(MediaSession::SendPacket_Callback, this, track);
	return true;
}

bool MediaSession::Add_RtpInstance(MediaSession::TrackId trackid, RtpInstance* rtpInstance)
{
	Track* track = Get_Track(trackid);
	if (!track || track->m_isAlive != true)
		return false;

	track->m_rtpInstances.push_back(rtpInstance);
	return true;
}

bool MediaSession::Remove_RtpInstance(RtpInstance* rtpinstance)
{
	for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
	{
		if (m_tracks[i].m_isAlive == false)
			continue;

		std::list<RtpInstance*>::iterator it = std::find(m_tracks[i].m_rtpInstances.begin(),
			m_tracks[i].m_rtpInstances.end(), rtpinstance);

		if (it == m_tracks[i].m_rtpInstances.end())
			continue;

		m_tracks[i].m_rtpInstances.erase(it);
		return true;
	}
	return false;
}

void MediaSession::SendPacket_Callback(void* arg1, void* arg2, void* packet, Sink::PacketType packettype)
{
	RtpPacket* rtppacket = (RtpPacket*)packet;
	MediaSession* session = (MediaSession*)arg1;
	MediaSession::Track* track = (MediaSession::Track*)arg2;

	session->HandleSend_RtpPacket(track, rtppacket);
}

void MediaSession::HandleSend_RtpPacket(MediaSession::Track* track, RtpPacket* rtppacket)
{
	std::list<RtpInstance*>::iterator it;
	for (it = track->m_rtpInstances.begin(); it != track->m_rtpInstances.end(); ++it)
	{
		RtpInstance* rtpinstance = *it;
		if (rtpinstance->Alive())
			rtpinstance->Send(rtppacket);
	}
}

bool MediaSession::Start_Multicast()
{
	//随机生成多播地址
	sockaddr_in addr = { 0 };
	uint32_t range = 0xe8ffffff - 0xe8000100;
	addr.sin_addr.s_addr = htonl(0xe8000100 + (rand()) % range);
	m_multicastAddr = inet_ntoa(addr.sin_addr);

	int rtpsockfd1, rtcpsockfd1;
	int rtpsockfd2, rtcpsockfd2;
	uint16_t rtpport1, rtcpport1;
	uint16_t rtpport2, rtcpport2;
	bool ret;

	rtpsockfd1 = sockets::Create_UdpSocket();
	assert(rtpsockfd1 > 0);

	rtpsockfd2 = sockets::Create_UdpSocket();
	assert(rtpsockfd2 > 0);

	rtcpsockfd1 = sockets::Create_UdpSocket();
	assert(rtcpsockfd1 > 0);

	rtcpsockfd2 = sockets::Create_UdpSocket();
	assert(rtcpsockfd2 > 0);

	uint16_t port = rand() & 0xfffe;
	if (port < 10000)
		port += 10000;

	rtpport1 = port;
	rtcpport1 = port + 1;
	rtpport2 = rtcpport1 + 1;
	rtcpport2 = rtpport2 + 1;

	m_multicast_rtpInstances[TrackId0] = RtpInstance::CreateNew_OverUdp(rtpsockfd1, 0, m_multicastAddr, rtpport1);
	m_multicast_rtpInstances[TrackId1] = RtpInstance::CreateNew_OverUdp(rtpsockfd2, 0, m_multicastAddr, rtpport2);
	m_multicast_rtcpInstances[TrackId0] = RtcpInstance::Create_New(rtcpsockfd1, 0, m_multicastAddr, rtcpport1);
	m_multicast_rtcpInstances[TrackId1] = RtcpInstance::Create_New(rtcpsockfd2, 0, m_multicastAddr, rtcpport2);

	this->Add_RtpInstance(TrackId0, m_multicast_rtpInstances[TrackId0]);
	this->Add_RtpInstance(TrackId1, m_multicast_rtpInstances[TrackId1]);
	m_multicast_rtpInstances[TrackId0]->Set_Alive(true);
	m_multicast_rtpInstances[TrackId1]->Set_Alive(true);

	m_isStart_Multicast = true;

	return true;
}

bool MediaSession::Is_StartMulticast()
{
	return m_isStart_Multicast;
}

uint16_t MediaSession::GetMuliticast_DestRtpPort(TrackId trackid)
{
	if (trackid > TrackId1 || !m_multicast_rtpInstances[trackid])
		return -1;
	return m_multicast_rtpInstances[trackid]->Get_PeerPort();
}