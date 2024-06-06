#include "Rtp.h"

void parseRtpHeader(uint8_t* buf, Rtp* rtp)
{
	memset(rtp, 0, sizeof(*rtp));
	rtp->version = (buf[0] & 0xc0) >> 6;
	rtp->padding = (buf[0] & 0x20) >> 5;
	rtp->extension = (buf[0] & 0x10) >> 4;
	rtp->csrc = (buf[0] & 0x0f);

	rtp->marker = (buf[1] & 0x80) >> 7;
	rtp->payloadtype = (buf[1] & 0x7f);

	rtp->seq = ((buf[2] & 0xff) << 8) | (buf[3] & 0xff);

	rtp->timestamp = ((buf[4] & 0xff) << 24) | ((buf[5] & 0xff) << 16) | ((buf[6] & 0xff) << 8) | (buf[7] & 0xff);

	rtp->ssrc = ((buf[8] & 0xff) << 24) | ((buf[9] & 0xff) << 16) | ((buf[10] & 0xff) << 8) | (buf[11] & 0xff);
}

void parseRtcpHeader(uint8_t* buf, RtcpHeader* rtcp)
{
	memset(rtcp, 0, sizeof(*rtcp));
	rtcp->version = (buf[0] & 0xc0) >> 6;
	rtcp->padding = (buf[0] & 0x20) >> 5;
	rtcp->rc = (buf[0] & 0x1f);

	rtcp->payloadtype = (buf[1] & 0xff);

	rtcp->length = ((buf[2] & 0xff) << 8) | (buf[3] & 0xff);
}