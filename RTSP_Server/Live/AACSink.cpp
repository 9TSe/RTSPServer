#include "AACSink.h"

AACSink* AACSink::createNew(UsageEnvironment* env, MediaSource* source)
{
	return new AACSink(env, source);
}

AACSink::AACSink(UsageEnvironment* env, MediaSource* source)
	:Sink(env, source, RTP_PAYLOAD_TYPE_AAC)
	,m_sampleRate(44100)
	, m_channel(2)
	,m_fps(source->getFps())
{
	LOGI("AACSink()");
	m_marker = 1;
	runEvery(1000 / m_fps);
}

AACSink::~AACSink()
{
	LOGI("~AACSink()");
}

std::string AACSink::getMediaDescription(uint16_t port)
{
	char buf[100] = { 0 };
	sprintf(buf, "m=audio %hu RTP/AVP %d", port, m_payloadType);
	return buf;
}

std::string AACSink::getAtrribute()
{
	char buf[500] = { 0 };
	sprintf(buf, "a=rtpmap:97 mpeg4-generic/44100/2\r\n");

	uint8_t index = 4; //sampling_frequency_index[4] = 44100
	uint8_t profile = 1; //aac
	char configstr[10] = { 0 };
	sprintf(configstr, "%02x%02x", (uint8_t)((profile + 1) << 3) | (index >> 1), //0001 0010 0x12
		(uint8_t)((index << 7) | (m_channel << 3))); //0100 ??out -> 0001 0000 0x10
	sprintf(buf + strlen(buf),
		"a=fmtp:%d profile-level-id=1;"
		"mode=AAC-hbr;"
		"sizelength=13;indexlength=3;indexdeltalength=3;" //i think indexlength is 4 instead of 3
		"config=%04u",
		m_payloadType, atoi(configstr));
	return buf;
}

void AACSink::sendFrame(MediaFrame* frame)
{
	int framesize = frame->m_size - 7;
	m_rtpPacket.m_rtp->payload[0] = 0x00;
	m_rtpPacket.m_rtp->payload[1] = 0x10;
	m_rtpPacket.m_rtp->payload[2] = (framesize & 0x1fe0) >> 5;	//aacframelength = 13bit
	m_rtpPacket.m_rtp->payload[3] = (framesize & 0x1f) << 3;

	memcpy(m_rtpPacket.m_rtp->payload + 4, frame->m_buf + 7, framesize);
	m_rtpPacket.m_size = RTP_HEADER_SIZE + 4 + framesize;
	sendRtpPacket(&m_rtpPacket);
	m_seq++;
	m_timestamp += m_sampleRate * (1000 / m_fps) / 1000;  //1025
	//m_timestamp += (m_sampleRate / 1024) * (1000 / m_fps); //every second time 1001
}