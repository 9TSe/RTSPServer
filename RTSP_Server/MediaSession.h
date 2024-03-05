#pragma once
#include <string>
#include <list>
#include "RtpInstance.h"
#include "Sink.h"

#define MEDIA_MAX_TRACK_NUM 2

class MediaSession
{
public:
	enum TrackId
	{
		TrackIdNone = -1,
		TrackId0	= 0,
		TrackId1	= 1
	};

	static MediaSession* Create_New(std::string sessionname);
	explicit MediaSession(const std::string& sessionname);
	~MediaSession();

	std::string Name() const { return m_sessionName; }
	std::string Generate_SdpDescription();
	bool Add_Sink(MediaSession::TrackId trackid, Sink* sink); // 生产者
	bool Add_RtpInstance(MediaSession::TrackId trackid, RtpInstance* rtpinstance); // 消费者
	bool Remove_RtpInstance(RtpInstance* rtpinstance);

	bool Start_Multicast();
	bool Is_StartMulticast();
	std::string GetMuliticast_DestAddr() const { return m_multicastAddr; }
	uint16_t GetMuliticast_DestRtpPort(TrackId trackid);

private:
	class Track
	{
	public:
		Sink* m_sink;
		int m_trackId;
		bool m_isAlive;
		std::list<RtpInstance*> m_rtpInstances;
	};

	Track* Get_Track(MediaSession::TrackId trackid);
	static void SendPacket_Callback(void* arg1, void* arg2, void* packet, Sink::PacketType packettype);
	void HandleSend_RtpPacket(MediaSession::Track* track, RtpPacket* rtppacket);


private:
	std::string m_sessionName;
	std::string m_sdp;
	Track m_tracks[MEDIA_MAX_TRACK_NUM];
	bool m_isStart_Multicast;
	std::string m_multicastAddr;
	RtpInstance* m_multicast_rtpInstances[MEDIA_MAX_TRACK_NUM];
	RtcpInstance* m_multicast_rtcpInstances[MEDIA_MAX_TRACK_NUM];
};