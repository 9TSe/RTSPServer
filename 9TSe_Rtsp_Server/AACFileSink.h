#pragma once
#include "UsageEnvironment.h"
#include "Sink.h"
#include "MediaSource.h"

class AACFileSink : public Sink
{
public:
	AACFileSink(UsageEnvironment* env, MediaSource* mediasource, int payloadtype);
	virtual ~AACFileSink();
	static AACFileSink* Create_New(UsageEnvironment* env, MediaSource* mediasource);

	virtual std::string Get_MediaDescription(uint16_t port);
	virtual std::string Get_Attribute();

protected:
	virtual void Send_Frame(MediaFrame* frame);

private:
	RtpPacket m_rtpPacket;
	uint32_t m_sampleRate;
	uint32_t m_channels;
	int m_fps;

};