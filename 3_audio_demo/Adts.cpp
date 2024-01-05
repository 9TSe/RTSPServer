#include "Adts.h"
#include <cstring>
#include <stdio.h>

int Parse_AdtsHeader(uint8_t* headerBuf, AdtsHeader* adtsHeader)
{
    memset(adtsHeader, 0, sizeof(*adtsHeader)); //初始化
    if ((headerBuf[0] == 0xFF) && ((headerBuf[1] & 0xF0) == 0xF0))//三个 '1111'代表 1个ADTS帧的开始
    {
        adtsHeader->id = ((unsigned int)headerBuf[1] & 0x08) >> 3; //如果id==1, >>3后变为1
        adtsHeader->layer = ((unsigned int)headerBuf[1] & 0x06) >> 1;// 0000 0110, layer一般为0 &后为0
        adtsHeader->protectionAbsent = (unsigned int)headerBuf[1] & 0x01;
        adtsHeader->profile = ((unsigned int)headerBuf[2] & 0xc0) >> 6; //1100 0000
        adtsHeader->samplingFreqIndex = ((unsigned int)headerBuf[2] & 0x3c) >> 2; // 0011 1100
        adtsHeader->privateBit = ((unsigned int)headerBuf[2] & 0x02) >> 1; // 0000 0010
        // 0000 0001 -> 0000 0100 | 1100 0000 -> 0000 0011
        adtsHeader->channelCfg = ((((unsigned int)headerBuf[2] & 0x01) << 2) |
                (((unsigned int)headerBuf[3] & 0xc0) >> 6));
        adtsHeader->originalCopy = ((unsigned int)headerBuf[3] & 0x20) >> 5; // 0010 0000
        adtsHeader->home = ((unsigned int)headerBuf[3] & 0x10) >> 4;
        adtsHeader->copyrightIdentificationBit = ((unsigned int)headerBuf[3] & 0x08) >> 3;
        adtsHeader->copyrightIdentificationStart = ((unsigned int)headerBuf[3] & 0x04) >> 2;

        //0000 0000 0000 0011 -> 0001 1111 1111 1111
        adtsHeader->aacFrameLength = (((((unsigned int)headerBuf[3]) & 0x03) << 11) |
                (((unsigned int)headerBuf[4] & 0xFF) << 3) |
                ((unsigned int)headerBuf[5] & 0xE0) >> 5);
        adtsHeader->adtsBufferFullness = (((unsigned int)headerBuf[5] & 0x1f) << 6 |
                ((unsigned int)headerBuf[6] & 0xfc) >> 2);
        adtsHeader->numberOfRawDataBlockInFrame = ((unsigned int)headerBuf[6] & 0x03);
        return 0;
    }
    else 
    {
        std::cout << "fail to parse" << std::endl;
        return -1;
    }
}
int Convert_AdtsHeader_ToBuf(struct AdtsHeader* adtsHeader, uint8_t* adtsHeaderBuf)
{
    adtsHeaderBuf[0] = 0xFF;
    adtsHeaderBuf[1] = 0xF1;
    adtsHeaderBuf[2] = ((adtsHeader->profile) << 6) + (adtsHeader->samplingFreqIndex << 2) + (adtsHeader->channelCfg >> 2);
    adtsHeaderBuf[3] = (((adtsHeader->channelCfg & 3) << 6) + (adtsHeader->aacFrameLength >> 11));
    adtsHeaderBuf[4] = ((adtsHeader->aacFrameLength & 0x7FF) >> 3);
    adtsHeaderBuf[5] = (((adtsHeader->aacFrameLength & 7) << 5) + 0x1F);
    adtsHeaderBuf[6] = 0xFC;
    return 0;
}