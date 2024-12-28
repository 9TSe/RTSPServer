#pragma once
#include "Sink.h"

class AACSink : public Sink
{
public:
	static AACSink* createNew(std::shared_ptr<UsageEnvironment> env, MediaSource* source);
	AACSink(std::shared_ptr<UsageEnvironment> env, MediaSource* source);
	virtual ~AACSink();

	virtual std::string getMediaDescription(uint16_t port);
	virtual std::string getAtrribute();

protected:
	virtual void sendFrame(MediaFrame* frame);

private:
	RtpPacket m_rtpPacket;
	uint32_t m_sampleRate;
	uint32_t m_channel;
	int m_fps;
};