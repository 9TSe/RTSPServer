#include <stdio.h>
#include <string.h>
#include "H264FileSink.h"
#include "Log.h"

H264FileSink* H264FileSink::Create_New(UsageEnvironment* env, MediaSource* mediasource)
{
    if (!mediasource)
        return nullptr;
    return new H264FileSink(env, mediasource);
}

H264FileSink::H264FileSink(UsageEnvironment* env, MediaSource* mediasource)
    :Sink(env, mediasource, RTP_PAYLOAD_TYPE_H264),
    m_clockRate(90000),
    m_fps(mediasource->Get_Fps())
{
    LOGI("H264FileSink()");
    Run_Every(1000 / m_fps);
}

H264FileSink::~H264FileSink()
{
    LOGI("~H264FileSink()");
}

std::string H264FileSink::Get_MediaDescription(uint16_t port)
{
    char buf[100] = { 0 };
    sprintf(buf, "m=video %hu RTP/AVP %d", port, m_payloadType);
    return std::string(buf);
}

std::string H264FileSink::Get_Attribute()
{
    char buf[100];
    sprintf(buf, "a=rtpmap:%d H264/%d\r\n", m_payloadType, m_clockRate);
    sprintf(buf + strlen(buf), "a=framerate:%d", m_fps);
    return std::string(buf);
}

void H264FileSink::Send_Frame(MediaFrame* frame)
{
    RtpHeader* rtpHeader = m_rtpPacket.m_rtpHeader;
    uint8_t naluType = frame->m_buf[0];

    if (frame->m_size <= RTP_MAX_PKT_SIZE)
    {
        memcpy(rtpHeader->payload, frame->m_buf, frame->m_size);
        m_rtpPacket.m_size = RTP_HEADER_SIZE + frame->m_size;
        Send_RtpPacket(&m_rtpPacket);
        m_seq++;

        if ((naluType & 0x1F) == 7 || (naluType & 0x1F) == 8) // 如果是SPS、PPS就不需要加时间戳
            return;
    }
    else
    {
        int pktnum = frame->m_size / RTP_MAX_PKT_SIZE;       // 有几个完整的包
        int remainpkt_size = frame->m_size % RTP_MAX_PKT_SIZE; // 剩余不完整包的大小
        int pos = 1;

        /* 发送完整的包 */
        for (int i = 0; i < pktnum; i++)
        {
            /*
            *     FU Indicator
            *    0 1 2 3 4 5 6 7
            *   +-+-+-+-+-+-+-+-+
            *   |F|NRI|  Type   |
            *   +---------------+
            * */
            rtpHeader->payload[0] = (naluType & 0x60) | 28; //(naluType & 0x60)表示nalu的重要性，28表示为分片

            /*
            *      FU Header
            *    0 1 2 3 4 5 6 7
            *   +-+-+-+-+-+-+-+-+
            *   |S|E|R|  Type   |
            *   +---------------+
            * */
            rtpHeader->payload[1] = naluType & 0x1F;

            if (i == 0) //第一包数据
                rtpHeader->payload[1] |= 0x80; // start
            else if (remainpkt_size == 0 && i == pktnum - 1) //最后一包数据
                rtpHeader->payload[1] |= 0x40; // end

            memcpy(rtpHeader->payload + 2, frame->m_buf + pos, RTP_MAX_PKT_SIZE);
            m_rtpPacket.m_size = RTP_HEADER_SIZE + 2 + RTP_MAX_PKT_SIZE;
            Send_RtpPacket(&m_rtpPacket);

            m_seq++;
            pos += RTP_MAX_PKT_SIZE;
        }

        /* 发送剩余的数据 */
        if (remainpkt_size > 0)
        {
            rtpHeader->payload[0] = (naluType & 0x60) | 28;
            rtpHeader->payload[1] = naluType & 0x1F;
            rtpHeader->payload[1] |= 0x40; //end

            memcpy(rtpHeader->payload + 2, frame->m_buf + pos, remainpkt_size);
            m_rtpPacket.m_size = RTP_HEADER_SIZE + 2 + remainpkt_size;
            Send_RtpPacket(&m_rtpPacket);
            m_seq++;
        }
    }

    m_timeStamp += m_clockRate / m_fps;



}
