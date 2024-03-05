#pragma once
#include <stdint.h>
#include <stdlib.h>

#define RTP_VERSION				2
#define RTP_PAYLOAD_TYPE_H264	96
#define RTP_PAYLOAD_TYPE_AAC	97
#define RTP_HEADER_SIZE			12
#define RTP_MAX_PKT_SIZE		1400

struct RtpHeader
{
	uint8_t csrcLen		: 4;
	uint8_t extension	: 1;
	uint8_t padding		: 1;
	uint8_t version		: 2;

	uint8_t payloadType : 7;
	uint8_t marker		: 1;

	uint16_t seq;

	uint32_t timestamp;

	uint32_t ssrc;

	uint8_t payload[0];
};

struct RtcpHeader
{
	uint8_t rc		: 5;
	uint8_t padding : 1;
	uint8_t version : 2;

	uint8_t packetType;

	uint16_t length;
};

class RtpPacket
{
public:
	RtpPacket();
	~RtpPacket();

	uint8_t* m_buf;		//4+rtpHeader+rtpBody
	uint8_t* m_buf4;	// rtpHeader+rtpBody
	RtpHeader* const m_rtpHeader;
	int m_size; //rtpHeader+rtpBody
};

void Parse_RtpHeader(uint8_t* buf, RtpHeader* rtpheader);
void Parse_RtcpHeader(uint8_t* buf, RtcpHeader* rtcpheader);