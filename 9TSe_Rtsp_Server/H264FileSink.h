#pragma once
#include <stdint.h>
#include "Sink.h"

class H264FileSink : public Sink
{
public:
    H264FileSink(UsageEnvironment* env, MediaSource* mediasource);
    virtual ~H264FileSink();
    static H264FileSink* Create_New(UsageEnvironment* env, MediaSource* mediasource);
    virtual std::string Get_MediaDescription(uint16_t port);
    virtual std::string Get_Attribute();
    virtual void Send_Frame(MediaFrame* frame);

private:
    RtpPacket m_rtpPacket;
    int m_clockRate;
    int m_fps;

};