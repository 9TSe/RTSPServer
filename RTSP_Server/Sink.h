#pragma once
#include <string>
#include <stdint.h>
#include "Rtp.h"
#include "MediaSource.h"
#include "Event.h"
#include "UsageEnvironment.h"

class Sink
{
public:
	enum PacketType
	{
		UNKNOWN = -1,
		RTPPACKET = 0,
	};

	typedef void (*SessionSendPacketCallback)(void* arg1, void* arg2, void* packet, PacketType packetType);
	//using SessionSendPacketCallback = void (*)(void* arg1, void*arg2, void* packet, PacketType packetType);

	Sink(UsageEnvironment* env, MediaSource* mediasource, int payloadtype);
	virtual ~Sink();

	void Stop_TimeEvent();
	virtual std::string Get_MediaDescription(uint16_t port) = 0;
	virtual std::string Get_Attribute() = 0;
	void Set_SessionCallback(SessionSendPacketCallback callback, void* arg1, void* arg2);

protected:
	virtual void Send_Frame(MediaFrame* frame) = 0;
	void Send_RtpPacket(RtpPacket* packet);
	void Run_Every(int interval);

private:
	static void Callback_Timeout(void* arg);
	void Handle_Timeout();

protected:
	UsageEnvironment* m_env;
	MediaSource* m_mediaSource;
	SessionSendPacketCallback m_sessionSendpacket_callback;
	void* m_arg1;
	void* m_arg2;
	uint8_t m_csrcLen;
	uint8_t m_extension;
	uint8_t m_padding;
	uint8_t m_version;
	uint8_t m_payloadType;
	uint8_t m_marker;
	uint16_t m_seq;
	uint32_t m_timeStamp;
	uint32_t m_ssrc;

private:
	TimerEvent* m_timerEvent;
	Timer::TimerId m_timerId; //get after Run_Every()
};