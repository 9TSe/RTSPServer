#pragma once

#include <stdint.h>
#include <string.h>

constexpr int VERSION = 2;
constexpr int RTP_PAYLOAD_TYPE_H264 = 96;
constexpr int RTP_PAYLOAD_TYPE_AAC = 97;
constexpr int RTP_HEADER_SIZE = 12;
constexpr int RTPPKT_MAX_SIZE = 1400;


struct Rtp
{
	uint8_t csrc : 4;
	uint8_t extension : 1;
	uint8_t padding : 1;
	uint8_t version : 2;

	uint8_t payloadtype : 7;
	uint8_t marker : 1;

	uint16_t seq;

	uint32_t timestamp;

	uint32_t ssrc;

	uint8_t payload[];
};

struct RtcpHeader
{
	uint8_t rc : 5;
	uint8_t padding : 1;
	uint8_t version : 2;

	uint8_t payloadtype;

	uint16_t length;
};

class RtpPacket
{
public:
	RtpPacket()
		:m_buf(new uint8_t[4 + RTPPKT_MAX_SIZE + RTP_HEADER_SIZE + 100])
		,m_buf4(m_buf + 4)
		,m_size(0)
		,m_rtp((Rtp*)m_buf4)
	{}
	~RtpPacket()
	{
		delete[] m_buf;
	}

	uint8_t* m_buf;
	uint8_t* m_buf4;
	int m_size;
	Rtp* const m_rtp;
};

void parseRtpHeader(uint8_t* buf, Rtp* rtp);
void parseRtcpHeader(uint8_t* buf, RtcpHeader* rtcp);