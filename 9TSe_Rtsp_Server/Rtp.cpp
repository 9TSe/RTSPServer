#include "Rtp.h"
#include <string.h>

RtpPacket::RtpPacket()
	//m_buf(new uint8_t[4+RTP_HEADER_SIZE + RTP_MAX_PKT_SIZE + 100]),
	:m_buf((uint8_t*)malloc(4+RTP_HEADER_SIZE + RTP_MAX_PKT_SIZE + 100)),
	m_buf4(m_buf+4),
	m_rtpHeader((RtpHeader*)m_buf4),
	m_size(0)
{}

RtpPacket::~RtpPacket()
{	//delete []m_buf;
	free(m_buf);
	m_buf = nullptr;
}

void Parse_RtpHeader(uint8_t* buf, RtpHeader* rtpheader)
{
	memset(rtpheader, 0, sizeof(*rtpheader));

	rtpheader->version = (buf[0] & 0xc0) >> 6;
	rtpheader->padding = (buf[0] & 0x20) >> 5;
	rtpheader->extension = (buf[0] & 0x10) >> 4;
	rtpheader->csrcLen = (buf[0] & 0x0f);
	rtpheader->marker = (buf[1] & 0x80) >> 7;
	rtpheader->payloadType = (buf[1] & 0x7f);
	rtpheader->seq = ((buf[2] & 0xff) << 8) | (buf[3] & 0xff);
	rtpheader->timestamp = ((buf[4] & 0xff) << 24) | ((buf[5] & 0xff) << 16) |
							((buf[6] & 0xff) << 8) | ((buf[7] & 0xff));
	rtpheader->ssrc = ((buf[8] & 0xff) << 24) | ((buf[9] & 0xff) << 16) |
						((buf[10] & 0xff) << 8) | ((buf[11] & 0xff));
}

void Parse_RtcpHeader(uint8_t* buf, RtcpHeader* rtcpheader)
{
	memset(rtcpheader, 0, sizeof(*rtcpheader));

	rtcpheader->version = (buf[0] & 0xC0) >> 6;
	rtcpheader->padding = (buf[0] & 0x20) >> 5;
	rtcpheader->rc = (buf[0] & 0x1F);

	rtcpheader->packetType = (buf[1] & 0xFF);

	rtcpheader->length = ((buf[2] & 0xFF) << 8) | (buf[3] & 0xFF);
}