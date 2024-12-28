#include "H264Sink.h"

H264Sink* H264Sink::createNew(std::shared_ptr<UsageEnvironment> env, MediaSource* source)
{
	if (!source) return nullptr;
	return new H264Sink(env, source);
}

H264Sink::H264Sink(std::shared_ptr<UsageEnvironment> env, MediaSource* source)
	:Sink(env, source, RTP_PAYLOAD_TYPE_H264)
	, m_clockRate(90000)
	, m_fps(source->getFps())
{
	LOG_CORE_INFO("H264Sink()");
	runEvery(1000 / m_fps);
}

H264Sink::~H264Sink()
{
	LOG_CORE_INFO("~H264Sink()");
}

std::string H264Sink::getMediaDescription(uint16_t port)
{
	char buf[100] = { 0 };
	sprintf(buf, "m=video %hu RTP/AVP %d", port, m_payloadType);
	return buf;
}

std::string H264Sink::getAtrribute()
{
	char buf[100] = { 0 };
	/*sprintf(buf, "a=rtpmap:%d H264/%d\r\n"
				"a=framerate:%d", m_payloadType, m_clockRate, m_fps);*/
	sprintf(buf, "a=rtpmap:%d H264/%d\r\n", m_payloadType, m_clockRate);
	sprintf(buf + strlen(buf), "a=framerate:%d", m_fps);
	return buf;
}

void H264Sink::sendFrame(MediaFrame* frame)
{
	uint8_t nalu = frame->m_buf[0];
	if (frame->m_size <= RTPPKT_MAX_SIZE) //one nalu packet
	{
		memcpy(m_rtppacket.m_rtp->payload, frame->m_buf, frame->m_size);
		m_rtppacket.m_size = RTP_HEADER_SIZE + frame->m_size;
		sendRtpPacket(&m_rtppacket);
		m_seq++;
		//if ((nalu & 0x1f) == 7 || (nalu & 0x1f) == 8) return; //sps pps dont need add timestamp
		if ((nalu & 0x1f) == 6 || (nalu & 0x1f) == 7 || (nalu & 0x1f) == 8) return; //sei sps pps dont need add timestamp
	}
	else // more rtp load one nalu
	{
		int pktnum = frame->m_size / RTPPKT_MAX_SIZE;
		int remainsize = frame->m_size % RTPPKT_MAX_SIZE;

		int pos = 1; //avoid fist nalu bytes;
		for (int i = 0; i < pktnum; ++i) //send complete pkt
		{
			m_rtppacket.m_rtp->payload[0] = (nalu & 0x60) | 28;
			//if (nalu & 0x80) LOGE("---------------nalu error---------------");

			m_rtppacket.m_rtp->payload[1] = nalu & 0x1f;
			if (i == 0) m_rtppacket.m_rtp->payload[1] |= 0x80; //first nalu
			else if(remainsize == 0 && i == pktnum - 1) m_rtppacket.m_rtp->payload[1] |= 0x40; //last nalu

			memcpy(m_rtppacket.m_rtp->payload + 2, frame->m_buf + pos, RTPPKT_MAX_SIZE);
			m_rtppacket.m_size = RTP_HEADER_SIZE + 2 + RTPPKT_MAX_SIZE;
			sendRtpPacket(&m_rtppacket);

			m_seq++;
			pos += RTPPKT_MAX_SIZE;
		}

		if (remainsize > 0) // send remain nalu
		{
			m_rtppacket.m_rtp->payload[0] = (nalu & 0x60) | 28;
			m_rtppacket.m_rtp->payload[1] = nalu & 0x1f;
			m_rtppacket.m_rtp->payload[1] |= 0x40; //end

			memcpy(m_rtppacket.m_rtp->payload + 2, frame->m_buf + pos, remainsize);
			m_rtppacket.m_size = RTP_HEADER_SIZE + 2 + remainsize;
			sendRtpPacket(&m_rtppacket);
			m_seq++;
		}
		if ((nalu & 0x1f) == 6 || (nalu & 0x1f) == 7 || (nalu & 0x1f) == 8) return;
	}
	m_timestamp += m_clockRate / m_fps;
}