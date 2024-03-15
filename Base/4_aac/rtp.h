#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <stdint.h>
#define RTP_VESION              2
#define RTP_PAYLOAD_TYPE_H264   96
#define RTP_PAYLOAD_TYPE_AAC    97
#define RTP_HEADER_SIZE         12
#define RTP_MAX_PKT_SIZE        1400
struct RtpHeader
{
    uint8_t csrcLen : 4;
    uint8_t extension : 1;
    uint8_t padding : 1;
    uint8_t version : 2;
    uint8_t payloadType : 7;
    uint8_t marker : 1; //音频中标记会话的开始
    uint16_t seq;
    uint32_t timestamp;
    uint32_t ssrc;
};

struct RtpPacket
{
    RtpHeader rtpHeader;
    uint8_t payload[0];
};

void rtpHeaderInit(RtpPacket* rtpPacket, uint8_t csrcLen, uint8_t extension,
    uint8_t padding, uint8_t version, uint8_t payloadType, uint8_t marker,
    uint16_t seq, uint32_t timestamp, uint32_t ssrc);

int rtpSendPacketOverTcp(int clientSockfd, RtpPacket* rtpPacket, uint32_t dataSize);
int rtpSendPacketOverUdp(int serverRtpSockfd, const char* ip, int16_t port, RtpPacket* rtpPacket, uint32_t dataSize);


