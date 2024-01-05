#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <windows.h>
#include "rtp.h"
#include <thread>
#pragma warning(disable:4996)
#define AAC_FILE_NAME   "D:/ffmpeg/learn/test.aac"
#define H264_FILE_NAME   "D:/ffmpeg/learn/test.h264"
#define SERVER_PORT      8554
#define BUF_MAX_SIZE     (1024*1024)

static int createTcpSocket()
{
    int on = 1;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
    return sockfd;
}

static int bindSocketAddr(int sockfd, const char* ip, int port)
{
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(sockaddr)) < 0)
        return -1;
    return 0;
}

static int acceptClient(int sockfd, char* ip, int* port)
{
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t len = sizeof(addr);
    int clientfd = accept(sockfd, (struct sockaddr*)&addr, &len);
    if (clientfd < 0)
        return -1;
    strcpy(ip, inet_ntoa(addr.sin_addr));
    *port = ntohs(addr.sin_port);
    return clientfd;
}

static inline int startCode3(char* buf)
{
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
        return 1;
    else
        return 0;
}

static inline int startCode4(char* buf)
{
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1)
        return 1;
    else
        return 0;
}

static char* findNextStartCode(char* buf, int len)
{
    if (len < 3)
        return NULL;
    for (int i = 0; i < len - 3; ++i)
    {
        if (startCode3(buf) || startCode4(buf))
            return buf;
        ++buf;
    }
    /*if (startCode3(buf))
        return buf;*/
    return NULL;
}

static int getFrameFromH264File(FILE* fp, char* frame, int size) 
{
    int frameSize = -1;
    int rSize = fread(frame, 1, size, fp);

    if (!startCode3(frame) && !startCode4(frame))
        return -1;

    char* nextStartCode = findNextStartCode(frame + 3, rSize - 3);
    if (!nextStartCode)
        return -1;
    else
    {
        frameSize = (nextStartCode - frame);
        fseek(fp, frameSize - rSize, SEEK_CUR);
    }
    return frameSize;
}

struct AdtsHeader
{
    unsigned int syncword;
    uint8_t id;
    uint8_t layer;
    uint8_t protectionAbsent;
    uint8_t profile;
    uint8_t samplingFreqIndex;
    uint8_t privateBit;
    uint8_t channelCfg;
    uint8_t originalCopy;
    uint8_t home;
    uint8_t copyrightIdentificationBit;
    uint8_t copyrightIdentificationStart;
    unsigned int aacFrameLength;
    unsigned int adtsBufferFullness;
    uint8_t numberOfRawDataBlockInFrame;
};

static int Parse_AdtsHeader(uint8_t* in, AdtsHeader* res)
{
    static int frame_number = 0;
    memset(res, 0, sizeof(*res));

    if ((in[0] == 0xFF) && ((in[1] & 0xF0) == 0xF0))
    {
        res->id = ((uint8_t)in[1] & 0x08) >> 3;
        res->layer = ((uint8_t)in[1] & 0x06) >> 1;
        res->protectionAbsent = (uint8_t)in[1] & 0x01;
        res->profile = ((uint8_t)in[2] & 0xc0) >> 6;
        res->samplingFreqIndex = ((uint8_t)in[2] & 0x3c) >> 2;
        res->privateBit = ((uint8_t)in[2] & 0x02) >> 1;
        res->channelCfg = ((((uint8_t)in[2] & 0x01) << 2) | (((unsigned int)in[3] & 0xc0) >> 6));
        res->originalCopy = ((uint8_t)in[3] & 0x20) >> 5;
        res->home = ((uint8_t)in[3] & 0x10) >> 4;
        res->copyrightIdentificationBit = ((uint8_t)in[3] & 0x08) >> 3;
        res->copyrightIdentificationStart = ((uint8_t)in[3] & 0x04) >> 2;
        res->aacFrameLength = (((((unsigned int)in[3]) & 0x03) << 11) |
            (((unsigned int)in[4] & 0xFF) << 3) |
            ((unsigned int)in[5] & 0xE0) >> 5);

        res->adtsBufferFullness = (((unsigned int)in[5] & 0x1f) << 6 |
            ((unsigned int)in[6] & 0xfc) >> 2);
        res->numberOfRawDataBlockInFrame = ((uint8_t)in[6] & 0x03);
        return 0;
    }
    else
    {
        std::cout << "parse adts header fail\n";
        return -1;
    }
}

static int rtpSendAACFrame(int clientSockfd, RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize)
{
    rtpPacket->payload[0] = 0x00;
    rtpPacket->payload[1] = 0x10;
    rtpPacket->payload[2] = (frameSize & 0x1FE0) >> 5; 
    rtpPacket->payload[3] = (frameSize & 0x1F) << 3;
    memcpy(rtpPacket->payload + 4, frame, frameSize);
    int ret = rtpSendPacketOverTcp(clientSockfd, rtpPacket, frameSize + 4, 0x02);
    if (ret < 0)
    {
        printf("failed to send rtp packet\n");
        return -1;
    }
    rtpPacket->rtpHeader.seq++;
    rtpPacket->rtpHeader.timestamp += 1025;
    return 0;
}


static int rtpSendH264Frame(int clientSockfd, RtpPacket* rtpPacket, char* frame, uint32_t frameSize)
{
    int sendByte = 0;
    int ret;
    uint8_t naluType = frame[0];
    printf("%s frameSize=%d \n", __FUNCTION__, frameSize);
    if (frameSize <= RTP_MAX_PKT_SIZE)
    {
        memcpy(rtpPacket->payload, frame, frameSize);
        ret = rtpSendPacketOverTcp(clientSockfd, rtpPacket, frameSize, 0x00);
        if (ret < 0)
            return -1;

        rtpPacket->rtpHeader.seq++;
        sendByte += ret;
        if ((naluType & 0x1F) == 7 || (naluType & 0x1F) == 8) 
            return sendByte;
    }
    else
    {
        int pktNum = frameSize / RTP_MAX_PKT_SIZE;
        int remainPktSize = frameSize % RTP_MAX_PKT_SIZE;
        int pos = 1;
        for (int i = 0; i < pktNum; i++)
        {
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            rtpPacket->payload[1] = naluType & 0x1F;

            if (i == 0)
                rtpPacket->payload[1] |= 0x80;
            else if (remainPktSize == 0 && i == pktNum - 1) 
                rtpPacket->payload[1] |= 0x40;

            memcpy(rtpPacket->payload + 2, frame + pos, RTP_MAX_PKT_SIZE);
            ret = rtpSendPacketOverTcp(clientSockfd, rtpPacket, RTP_MAX_PKT_SIZE + 2, 0x00);
            if (ret < 0)
                return -1;

            rtpPacket->rtpHeader.seq++;
            sendByte += ret;
            pos += RTP_MAX_PKT_SIZE;
        }

        if (remainPktSize > 0)
        {
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            rtpPacket->payload[1] = naluType & 0x1F;
            rtpPacket->payload[1] |= 0x40; //end

            memcpy(rtpPacket->payload + 2, frame + pos, remainPktSize + 2);
            ret = rtpSendPacketOverTcp(clientSockfd, rtpPacket, remainPktSize + 2, 0x00);
            if (ret < 0)
                return -1;

            rtpPacket->rtpHeader.seq++;
            sendByte += ret;
        }
    }
    return sendByte;
}

static int handleCmd_OPTIONS(char* result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Public: OPTIONS, DESCRIBE, SETUP, PLAY\r\n"
        "\r\n",
        cseq);
    return 0;
}

static int handleCmd_DESCRIBE(char* result, int cseq, char* url)
{
    char sdp[500];
    char localIp[100];

    sscanf(url, "rtsp://%[^:]:", localIp);

    sprintf(sdp, "v=0\r\n"
        "o=- 9%ld 1 IN IP4 %s\r\n"
        "t=0 0\r\n"
        "a=control:*\r\n"
        "m=video 0 RTP/AVP/TCP 96\r\n"
        "a=rtpmap:96 H264/90000\r\n"
        "a=control:track0\r\n"
        "m=audio 1 RTP/AVP/TCP 97\r\n"
        "a=rtpmap:97 mpeg4-generic/44100/2\r\n"
        "a=fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3;config=1210;\r\n"
        "a=control:track1\r\n",
        time(NULL), localIp);

    sprintf(result, "RTSP/1.0 200 OK\r\nCSeq: %d\r\n"
        "Content-Base: %s\r\n"
        "Content-type: application/sdp\r\n"
        "Content-length: %zu\r\n\r\n"
        "%s",
        cseq, url, strlen(sdp), sdp);
    return 0;
}

static int handleCmd_SETUP(char* result, int cseq)
{
    if (cseq == 3) 
    {
        sprintf(result, "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n"
            "Session: 66334873\r\n"
            "\r\n",
            cseq);
    }
    else if (cseq == 4) 
    {
        sprintf(result, "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Transport: RTP/AVP/TCP;unicast;interleaved=2-3\r\n"
            "Session: 66334873\r\n"
            "\r\n",
            cseq);
    }
    return 0;
}

static int handleCmd_PLAY(char* result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Range: npt=0.000-\r\n"
        "Session: 66334873; timeout=10\r\n\r\n",
        cseq);
    return 0;
}

static void doClient(int clientSockfd, const char* clientIP, int clientPort) 
{
    char method[40], url[100], version[40];
    int CSeq;
    char* rBuf = (char*)malloc(BUF_MAX_SIZE);
    char* sBuf = (char*)malloc(BUF_MAX_SIZE);

    while (true) 
    {
        int recvLen = recv(clientSockfd, rBuf, BUF_MAX_SIZE, 0);
        if (recvLen <= 0)
            break;

        rBuf[recvLen] = '\0';
        printf("接收请求 rBuf = %s \n", rBuf);

        const char* sep = "\n";
        char* line = strtok(rBuf, sep);
        while (line) 
        {
            if (strstr(line, "OPTIONS") ||
                strstr(line, "DESCRIBE") ||
                strstr(line, "SETUP") ||
                strstr(line, "PLAY")) 
            {
                if (sscanf(line, "%s %s %s\r\n", method, url, version) != 3)
                {}      // error 
            }
            else if (strstr(line, "CSeq")) 
            {
                if (sscanf(line, "CSeq: %d\r\n", &CSeq) != 1)
                {}      // error
            }
            else if (!strncmp(line, "Transport:", strlen("Transport:"))) 
            {
                if (sscanf(line, "Transport: RTP/AVP/TCP;unicast;interleaved=0-1\r\n") != 0) 
                {
                    printf("parse Transport error \n");
                }
            }
            line = strtok(NULL, sep);
        }

        if (!strcmp(method, "OPTIONS")) 
        {
            if (handleCmd_OPTIONS(sBuf, CSeq))
            {
                printf("failed to handle options\n");
                break;
            }
        }
        else if (!strcmp(method, "DESCRIBE")) 
        {
            if (handleCmd_DESCRIBE(sBuf, CSeq, url))
            {
                printf("failed to handle describe\n");
                break;
            }
        }
        else if (!strcmp(method, "SETUP")) 
        {
            if (handleCmd_SETUP(sBuf, CSeq))
            {
                printf("failed to handle setup\n");
                break;
            }
        }
        else if (!strcmp(method, "PLAY")) 
        {
            if (handleCmd_PLAY(sBuf, CSeq))
            {
                printf("failed to handle play\n");
                break;
            }
        }
        else 
        {
            printf("未定义的method = %s \n", method);
            break;
        }
        printf("响应 sBuf = %s \n", sBuf);

        send(clientSockfd, sBuf, strlen(sBuf), 0);
        if (!strcmp(method, "PLAY")) 
        {

            std::thread t1([&]() {
                int frameSize, startCode;
                char* frame = (char*)malloc(500000);
                RtpPacket* rtpPacket = (RtpPacket*)malloc(500000);
                FILE* fp = fopen(H264_FILE_NAME, "rb");
                if (!fp) 
                {
                    printf("读取 %s 失败\n", H264_FILE_NAME);
                    return;
                }
                rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_H264, 
                    0,0, 0, 0x88923423);

                printf("start play\n");

                while (true) 
                {
                    frameSize = getFrameFromH264File(fp, frame, 500000);
                    if (frameSize < 0)
                    {
                        printf("读取%s结束,frameSize=%d \n", H264_FILE_NAME, frameSize);
                        break;
                    }
                    if (startCode3(frame))
                        startCode = 3;
                    else
                        startCode = 4;
                    frameSize -= startCode;
                    rtpSendH264Frame(clientSockfd, rtpPacket, frame + startCode, frameSize);
                    rtpPacket->rtpHeader.timestamp += 90000 / 25;
                    //Sleep(40);//->30,20,
                    Sleep(20);
                    //usleep(40000);//1000/25 * 1000
                }
                free(frame);
                free(rtpPacket);
                });

            std::thread t2([&]() {
                AdtsHeader adtsHeader;          
                FILE* fp = fopen(AAC_FILE_NAME, "rb");
                if (!fp) 
                {
                    printf("读取 %s 失败\n", AAC_FILE_NAME);
                    return;
                }
                uint8_t* frame = (uint8_t*)malloc(5000);
                RtpPacket* rtpPacket = (RtpPacket*)malloc(5000);
                rtpHeaderInit(rtpPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_AAC, 1, 0, 0, 0x32411);
                int ret;
                while (true)
                {
                    ret = fread(frame, 1, 7, fp);
                    if (ret <= 0)
                    {
                        printf("fread err\n");
                        break;
                    }
                    printf("fread ret=%d \n", ret);

                    if (Parse_AdtsHeader(frame, &adtsHeader) < 0)
                    {
                        printf("parseAdtsHeader err\n");
                        break;
                    }
                    ret = fread(frame, 1, adtsHeader.aacFrameLength - 7, fp);
                    if (ret <= 0)
                    {
                        printf("fread err\n");
                        break;
                    }

                    rtpSendAACFrame(clientSockfd,
                        rtpPacket, frame, adtsHeader.aacFrameLength - 7);

                    Sleep(23);
                    //usleep(23223);//1000/43.06 * 1000
                }
                free(frame);
                free(rtpPacket);
                });

            t1.join();
            t2.join();
            break;
        }
        memset(method, 0, sizeof(method));
        memset(url, 0, sizeof(url));
        CSeq = 0;
    }
    closesocket(clientSockfd);
    free(rBuf);
    free(sBuf);
}

int main(int argc, char* argv[])
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("PC Server Socket Start Up Error \n");
        return -1;
    }

    int serverSockfd;
    serverSockfd = createTcpSocket();
    if (serverSockfd < 0)
    {
        WSACleanup();
        printf("failed to create tcp socket\n");
        return -1;
    }

    if (bindSocketAddr(serverSockfd, "0.0.0.0", SERVER_PORT) < 0)
    {
        printf("failed to bind addr\n");
        return -1;
    }

    if (listen(serverSockfd, 10) < 0)
    {
        printf("failed to listen\n");
        return -1;
    }

    printf("%s rtsp://127.0.0.1:%d\n", __FILE__, SERVER_PORT);

    while (true) 
    {
        int clientSockfd;
        char clientIp[40];
        int clientPort;
        clientSockfd = acceptClient(serverSockfd, clientIp, &clientPort);
        if (clientSockfd < 0)
        {
            printf("failed to accept client\n");
            return -1;
        }
        printf("accept client;client ip:%s,client port:%d\n", clientIp, clientPort);
        doClient(clientSockfd, clientIp, clientPort);
    }
    closesocket(serverSockfd);
    WSACleanup();
    return 0;
}
