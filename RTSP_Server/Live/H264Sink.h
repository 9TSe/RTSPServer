#pragma once
#include "Sink.h"
#include <string>

class H264Sink : public Sink
{
public:
	static H264Sink* createNew(std::shared_ptr<UsageEnvironment> env, MediaSource* source);
	H264Sink(std::shared_ptr<UsageEnvironment> env, MediaSource* source);
	virtual ~H264Sink();

	virtual std::string getMediaDescription(uint16_t port);
	virtual std::string getAtrribute();
	virtual void sendFrame(MediaFrame* frame);

private:
	RtpPacket m_rtppacket;
	int m_clockRate;
	int m_fps;
};