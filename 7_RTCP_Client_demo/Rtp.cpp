#include "Rtp.h"
#include <stdio.h>
#include <string.h>

void RtpHeader_Init(RtpPacket* rtpPacket, uint8_t csrcLen, uint8_t extension, uint8_t padding,
    uint8_t version, uint8_t payloadType, uint8_t marker, uint16_t seq, uint32_t timestamp, uint32_t ssrc)
{
    rtpPacket->rtpHeader.csrcLen = csrcLen;
    rtpPacket->rtpHeader.extension = extension;
    rtpPacket->rtpHeader.padding = padding;
    rtpPacket->rtpHeader.version = version;
    rtpPacket->rtpHeader.payloadType = payloadType;
    rtpPacket->rtpHeader.marker = marker;
    rtpPacket->rtpHeader.seq = seq;
    rtpPacket->rtpHeader.timestamp = timestamp;
    rtpPacket->rtpHeader.ssrc = ssrc;
}


int Parse_RtpHeader(uint8_t* headerBuf, RtpHeader* rtpHeader)
{
    memset(rtpHeader, 0, sizeof(*rtpHeader));
    rtpHeader->version = (headerBuf[0] & 0xC0) >> 6;
    rtpHeader->padding = (headerBuf[0] & 0x20) >> 5;
    rtpHeader->extension = (headerBuf[0] & 0x10) >> 4;
    rtpHeader->csrcLen = (headerBuf[0] & 0x0F);

    rtpHeader->marker = (headerBuf[1] & 0x80) >> 7;
    rtpHeader->payloadType = (headerBuf[1] & 0x7F);

    rtpHeader->seq = ((headerBuf[2] & 0xFF) << 8) | (headerBuf[3] & 0xFF);

    rtpHeader->timestamp = ((headerBuf[4] & 0xFF) << 24) | ((headerBuf[5] & 0xFF) << 16)
        | ((headerBuf[6] & 0xFF) << 8) | ((headerBuf[7] & 0xFF));

    rtpHeader->ssrc = ((headerBuf[8] & 0xFF) << 24) | ((headerBuf[9] & 0xFF) << 16)
        | ((headerBuf[10] & 0xFF) << 8) | ((headerBuf[11] & 0xFF));
    return 0;
}