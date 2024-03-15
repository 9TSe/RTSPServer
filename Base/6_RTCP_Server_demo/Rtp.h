#pragma once 
#include <stdint.h>

#define RTP_VERSION				2
#define RTP_PAYLOAD_TYPE_MESH	90
#define RTP_PAYLOAD_TYPE_H264	96
#define RTP_PAYLOAD_TYPE_AAC	97
#define RTP_HEADER_SIZE			12
#define RTP_MAX_SIZE			1400

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
};

struct RtpPacket
{
	RtpHeader rtpHeader;
	uint8_t payload[0];
};

void RtpHeader_Init(RtpPacket* rtpPacket, uint8_t csrcLen, uint8_t extension, uint8_t padding,
	uint8_t version, uint8_t payloadType, uint8_t marker, uint16_t seq, uint32_t timestamp, uint32_t ssrc);

int Parse_RtpHeader(uint8_t* headerBuf, RtpHeader* rtpHeader);