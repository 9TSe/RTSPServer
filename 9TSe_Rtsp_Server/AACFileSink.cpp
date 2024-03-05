#include "AACFileSink.h"
#include <string>
#include <stdio.h>
#include <string.h>
#include "Log.h"

AACFileSink::AACFileSink(UsageEnvironment* env, MediaSource* mediasource, int payloadtype)
	:Sink(env, mediasource, payloadtype),
	m_sampleRate(44100),
	m_channels(2),
	m_fps(mediasource->Get_Fps())
{
	LOGI("AACFileSink()");
	m_marker = 1;
	Run_Every(1000 / m_fps);
}

AACFileSink::~AACFileSink()
{
	LOGI("~AACFileSink()");
}

AACFileSink* AACFileSink::Create_New(UsageEnvironment* env, MediaSource* mediasource)
{
	return new AACFileSink(env, mediasource, RTP_PAYLOAD_TYPE_AAC);
}

std::string AACFileSink::Get_MediaDescription(uint16_t port)
{
	char buf[100] = { 0 };
	sprintf(buf, "m=audio %hu RTP/AVP %d", port, m_payloadType); //unsigned short
	return std::string(buf);
}

static uint32_t AACSampleRate[16] =
{
	97000, 88200, 64000, 48000,
	44100, 32000, 24000, 22050,
	16000, 12000, 11025, 8000,
	7350, 0, 0, 0 
};

std::string AACFileSink::Get_Attribute()
{
	char buf[500] = { 0 };
	sprintf(buf, "a=rtpmap:97 mpeg4-generic/%u/%u\r\n", m_sampleRate, m_channels);

	uint8_t index = 0;
	for (index = 0; index < 16; ++index)
	{
		if (AACSampleRate[index] == m_sampleRate)
			break;
	}
	if (index == 16)
		return "";

	uint8_t profile = 1;
	char configstr[10] = { 0 };
	sprintf(configstr, "%02x%02x", (uint8_t)((profile + 1) << 3) | (index >> 1),
		(uint8_t)((index << 7) | (m_channels << 3)));

	sprintf(buf + strlen(buf),
		"a=fmtp:%d profile-level-id=1;"
		"mode=AAC-hbr;"
		"sizelength=13;indexlength=3;indexdeltalength=3;"
		"config=%04u",
		m_payloadType, atoi(configstr));
	return std::string(buf);
}

void AACFileSink::Send_Frame(MediaFrame* frame)
{
	RtpHeader* rtpheader = m_rtpPacket.m_rtpHeader;
	int framesize = frame->m_size - 7; //remove aac_header

	rtpheader->payload[0] = 0x00;
	rtpheader->payload[1] = 0x10;
	rtpheader->payload[2] = (framesize & 0x1fe0) >> 5;//高8位
	rtpheader->payload[3] = (framesize & 0x1f) << 3; //低5位

	memcpy(rtpheader->payload + 4, frame->m_buf + 7, framesize);
	m_rtpPacket.m_size = RTP_HEADER_SIZE + 4 + framesize;

	Send_RtpPacket(&m_rtpPacket);
	m_seq++;

	//	1000 / m_fps -> 一帧多少毫秒
	m_timeStamp += m_sampleRate * (1000 / m_fps) / 1000;
}