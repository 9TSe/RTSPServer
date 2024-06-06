#pragma once
#include <string>
#include <list>
#include "Sink.h"
#include "RtpInstance.h"

constexpr int MAX_TRACK_NUM = 2;
class MediaSession
{
public:
	enum TrackId
	{
		TRACK_IDNONE = -1,
		TRACK_ID0 = 0,
		TRACK_ID1 = 1
	};
	static MediaSession* createNew(std::string name);
	MediaSession(const std::string& name);
	~MediaSession();

	bool removeRtpInstance(RtpInstance* rtpinstance);
	bool addRtpInstance(MediaSession::TrackId trackid, RtpInstance* rtpinstance);
	bool addSink(MediaSession::TrackId trackid, Sink* sink);

	bool startMulticast();
	uint16_t getMulticastDestRtpPort(MediaSession::TrackId trackid);
	std::string generateSdpDscription();

	std::string getMulticastDestAddr() { return m_multicastAddr; }
	std::string sessionName() const { return m_sessionname; }
	std::string MulticastAddr() { return m_multicastAddr; }
	bool isStartMulticast() { return m_isStart_multicast; }

private:
	class Track
	{
	public:
		Sink* m_sink;
		int m_trackid;
		bool m_isalive;
		std::list<RtpInstance*> m_rtpinstancelist;
	};

	Track* getTrack(MediaSession::TrackId trackid);
	static void sendPacketCallback(void* arg1, void* arg2, void* packet, Sink::PacketType packettype);
	void handleSendPacket(MediaSession::Track* track, RtpPacket* packet);

private:
	std::string m_sessionname;
	bool m_isStart_multicast;
	Track m_tracks[MAX_TRACK_NUM];
	RtpInstance* m_multicast_rtpinstances[MAX_TRACK_NUM];
	RtcpInstance* m_multicast_rtcpinstances[MAX_TRACK_NUM];
	std::string m_multicastAddr;
	std::string m_sdp;
};