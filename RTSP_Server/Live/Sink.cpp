#include "Sink.h"
#include "../Scheduler/Log.h"
#include "../Scheduler/EventScheduler.h"
#include <arpa/inet.h>

Sink::Sink(UsageEnvironment* env, MediaSource* source, int payloadType)
	:m_env(env)
	, m_mediaSource(source)
	, m_csrcLen(0)
	, m_extension(0)
	, m_padding(0)
	, m_version(VERSION)
	, m_payloadType(payloadType)
	, m_marker(0)
	, m_seq(0)
	, m_timestamp(0)
	, m_ssrc(rand())
	, m_sessionsendCallback(nullptr)
	, m_arg1(nullptr)
	, m_arg2(nullptr)
	, m_timeId(0)
{
	LOGI("Sink()");
	m_timeEvent = TimerEvent::createNew(this);
	m_timeEvent->setTimerCallback(timeoutCallback);
}

Sink::~Sink()
{
	LOGI("~Sink()");
	delete m_timeEvent;
	delete m_mediaSource; //why delete it
}

void Sink::timeoutCallback(void* arg)
{
	Sink* sink = (Sink*)arg;
	sink->handleTimeout();
}

void Sink::handleTimeout()
{
	MediaFrame* frame = m_mediaSource->getFrameFromQueue();
	if (!frame) return;
	this->sendFrame(frame);
	m_mediaSource->putFrameToQueue(frame); //start a thread to read file into frame???
}

void Sink::setSessionCallback(SessionSendPacketCallback callback, void* arg1, void* arg2)
{
	m_sessionsendCallback = callback;
	m_arg1 = arg1;
	m_arg2 = arg2;
}

void Sink::sendRtpPacket(RtpPacket* packet)
{
	Rtp* rtp = packet->m_rtp;
	rtp->csrc = m_csrcLen;
	rtp->extension = m_extension;
	rtp->padding = m_padding;
	rtp->version = m_version;
	rtp->payloadtype = m_payloadType;
	rtp->marker = m_marker;
	rtp->seq = htons(m_seq);
	rtp->timestamp = htonl(m_timestamp);
	rtp->ssrc = htonl(m_ssrc);

	if (m_sessionsendCallback)
		m_sessionsendCallback(m_arg1, m_arg2, packet, PacketType::RTPPACKET);
}

void Sink::runEvery(int interval)
{
	m_timeId = m_env->eventScheduler()->addTimeEventRunEvery(m_timeEvent, interval);
}