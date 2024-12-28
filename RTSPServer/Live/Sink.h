#pragma once
#include "Rtp.h"
#include "../Scheduler/Timer.h"
#include "../Scheduler/UsageEnvironment.h"
#include "../Scheduler/Event.h"
#include "MediaSource.h"
#include <string>
#include <functional>


class Sink
{
public:
	enum PacketType
	{
		UNKNOWN = -1,
		RTPPACKET = 0
	};
	using SessionSendPacketCallback = std::function<void(void* arg1, void* arg2, void* packet, PacketType type)>;

	explicit Sink(std::shared_ptr<UsageEnvironment> env, MediaSource* source, int payloadtype);
	virtual ~Sink();
	void setSessionCallback(SessionSendPacketCallback callback, void* arg1, void* arg2);

	virtual std::string getMediaDescription(uint16_t port) = 0;
	virtual std::string getAtrribute() = 0;

protected:
	virtual void sendFrame(MediaFrame* frame) = 0;
	void sendRtpPacket(RtpPacket* packet);
	void runEvery(int interval);
private:
	static void timeoutCallback(void* arg);
	void handleTimeout();

protected:
	std::shared_ptr<UsageEnvironment> m_env;
	MediaSource* m_mediaSource;

	uint8_t m_csrcLen : 4;
	uint8_t m_extension : 1;
	uint8_t m_padding : 1;
	uint8_t m_version : 2;
	uint8_t m_payloadType : 7;
	uint8_t m_marker : 1;
	uint16_t m_seq;
	uint32_t m_timestamp;
	uint32_t m_ssrc;
	SessionSendPacketCallback m_sessionsendCallback;
	void* m_arg1;
	void* m_arg2;

private:
	Timer::TimerId m_timeId;
	TimerEvent* m_timeEvent;
};