#include "RtspConnection.h"
#include "RtspServer.h"
#include "Rtp.h"
#include "MediaSessionManager.h"
#include "MediaSession.h"
#include "Version.h"
#include "Log.h"
#include <stdio.h>
#include <string.h>


static void Get_PeerIp(int fd, std::string& ip)
{
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);
    getpeername(fd, (struct sockaddr*)&addr, &addrlen);
    ip = inet_ntoa(addr.sin_addr);
}

RtspConnection* RtspConnection::Create_New(RtspServer* rtspserver, int clientfd)
{
    return new RtspConnection(rtspserver, clientfd);
    //    return New<RtspConnection>::allocate(rtspserver, clientfd);
}

RtspConnection::RtspConnection(RtspServer* rtspserver, int clientfd) :
    TcpConnection(rtspserver->Env(), clientfd),
    m_rtspServer(rtspserver),
    m_method(RtspConnection::Method::NONE),
    m_trackId(MediaSession::TrackId::TrackIdNone),
    m_sessionId(rand()),
    m_isRtp_overTcp(false),
    m_streamPrefix("track")
{
    LOGI("RtspConnection() m_clientFd=%d", m_clientFd);

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        m_rtpInstances[i] = NULL;
        m_rtcpInstances[i] = NULL;
    }
    Get_PeerIp(clientfd, m_peerIp);

}

RtspConnection::~RtspConnection()
{
    LOGI("~RtspConnection() m_clientFd=%d", m_clientFd);
    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        if (m_rtpInstances[i])
        {
            MediaSession* session = m_rtspServer->m_sessmgr->Get_Session(m_suffix);

            if (!session)
                session->Remove_RtpInstance(m_rtpInstances[i]);
            delete m_rtpInstances[i];
        }

        if (m_rtcpInstances[i])
            delete m_rtcpInstances[i];
    }

}

void RtspConnection::Handle_ReadBytes() {

    if (m_isRtp_overTcp)
    {
        if (m_inputBuffer.Peek()[0] == '$')
        {
            HandleRtp_OverTcp();
            return;
        }
    }

    if (!Parse_Request())
    {
        LOGE("Parse_Request err");
        goto disConnect;
    }
    switch (m_method)
    {
    case OPTIONS:
        if (!HandleCmd_Option())
            goto disConnect;
        break;
    case DESCRIBE:
        if (!HandleCmd_Describe())
            goto disConnect;
        break;
    case SETUP:
        if (!HandleCmd_Setup())
            goto disConnect;
        break;
    case PLAY:
        if (!HandleCmd_Play())
            goto disConnect;
        break;
    case TEARDOWN:
        if (!HandleCmd_Teardown())
            goto disConnect;
        break;

    default:
        goto disConnect;
    }

    return;
disConnect:
    Handle_Disconnect();
}

bool RtspConnection::Parse_Request()
{

    //解析第一行
    const char* crlf = m_inputBuffer.Find_CRLF();
    if (crlf == NULL) {
        m_inputBuffer.Retrieve_All();
        return false;
    }
    bool ret = Parse_Request1(m_inputBuffer.Peek(), crlf);
    if (ret == false) {
        m_inputBuffer.Retrieve_All();
        return false;
    }
    else {
        m_inputBuffer.Retrieve_Until(crlf + 2);
    }


    //解析第一行之后的所有行
    crlf = m_inputBuffer.Find_LastCRLF();
    if (crlf == NULL)
    {
        m_inputBuffer.Retrieve_All();
        return false;
    }
    ret = Parse_Request2(m_inputBuffer.Peek(), crlf);

    if (ret == false)
    {
        m_inputBuffer.Retrieve_All();
        return false;
    }
    else {
        m_inputBuffer.Retrieve_Until(crlf + 2);
        return true;
    }
}

bool RtspConnection::Parse_Request1(const char* begin, const char* end)
{
    std::string message(begin, end);
    char method[64] = { 0 };
    char url[512] = { 0 };
    char version[64] = { 0 };

    if (sscanf(message.c_str(), "%s %s %s", method, url, version) != 3)
    {
        return false;
    }

    if (!strcmp(method, "OPTIONS")) {
        m_method = OPTIONS;
    }
    else if (!strcmp(method, "DESCRIBE")) {
        m_method = DESCRIBE;
    }
    else if (!strcmp(method, "SETUP")) {
        m_method = SETUP;
    }
    else if (!strcmp(method, "PLAY")) {
        m_method = PLAY;
    }
    else if (!strcmp(method, "TEARDOWN")) {
        m_method = TEARDOWN;
    }
    else {
        m_method = NONE;
        return false;
    }
    if (strncmp(url, "rtsp://", 7) != 0)
    {
        return false;
    }

    uint16_t port = 0;
    char ip[64] = { 0 };
    char suffix[64] = { 0 };

    if (sscanf(url + 7, "%[^:]:%hu/%s", ip, &port, suffix) == 3)
    {

    }
    else if (sscanf(url + 7, "%[^/]/%s", ip, suffix) == 2)
    {
        port = 554;// 如果rtsp请求地址中无端口，默认获取的端口为：554
    }
    else
    {
        return false;
    }

    m_url = url;
    m_suffix = suffix;

    return true;
}

bool RtspConnection::Parse_Request2(const char* begin, const char* end)
{
    std::string message(begin, end);

    if (!Parse_CSeq(message)) {
        return false;
    }
    if (m_method == OPTIONS) {
        return true;
    }
    else if (m_method == DESCRIBE) {
        return Parse_Describe(message);
    }
    else if (m_method == SETUP)
    {
        return Parse_Setup(message);
    }
    else if (m_method == PLAY) {
        return Parse_Play(message);
    }
    else if (m_method == TEARDOWN) {
        return true;
    }
    else {
        return false;
    }

}

bool RtspConnection::Parse_CSeq(std::string& message)
{
    std::size_t pos = message.find("CSeq");
    if (pos != std::string::npos)
    {
        uint32_t cseq = 0;
        sscanf(message.c_str() + pos, "%*[^:]: %u", &cseq);
        m_cseq = cseq;
        return true;
    }

    return false;
}

bool RtspConnection::Parse_Describe(std::string& message)
{
    if ((message.rfind("Accept") == std::string::npos)
        || (message.rfind("sdp") == std::string::npos))
    {
        return false;
    }

    return true;
}

bool RtspConnection::Parse_Setup(std::string& message)
{
    m_trackId = MediaSession::TrackIdNone;
    std::size_t pos;

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; i++) {
        pos = m_url.find(m_streamPrefix + std::to_string(i));
        if (pos != std::string::npos)
        {
            if (i == 0) {
                m_trackId = MediaSession::TrackId0;
            }
            else if (i == 1)
            {
                m_trackId = MediaSession::TrackId1;
            }

        }

    }

    if (m_trackId == MediaSession::TrackIdNone) {
        return false;
    }

    pos = message.find("Transport");
    if (pos != std::string::npos)
    {
        if ((pos = message.find("RTP/AVP/TCP")) != std::string::npos)
        {
            uint8_t rtpChannel, rtcpChannel;
            m_isRtp_overTcp = true;

            if (sscanf(message.c_str() + pos, "%*[^;];%*[^;];%*[^=]=%hhu-%hhu",
                &rtpChannel, &rtcpChannel) != 2)
            {
                return false;
            }

            m_rtpChannel = rtpChannel;

            return true;
        }
        else if ((pos = message.find("RTP/AVP")) != std::string::npos)
        {
            uint16_t rtpPort = 0, rtcpPort = 0;
            if (((message.find("unicast", pos)) != std::string::npos))
            {
                if (sscanf(message.c_str() + pos, "%*[^;];%*[^;];%*[^=]=%hu-%hu",
                    &rtpPort, &rtcpPort) != 2)
                {
                    return false;
                }
            }
            else if ((message.find("multicast", pos)) != std::string::npos)
            {
                return true;
            }
            else
                return false;

            m_peerRtp_port = rtpPort;
            m_peerRtcp_port = rtcpPort;
        }
        else
        {
            return false;
        }

        return true;
    }

    return false;
}


bool RtspConnection::Parse_Play(std::string& message)
{
    std::size_t pos = message.find("Session");
    if (pos != std::string::npos)
    {
        uint32_t sessionId = 0;
        if (sscanf(message.c_str() + pos, "%*[^:]: %u", &sessionId) != 1)
            return false;
        return true;
    }

    return false;
}

bool RtspConnection::HandleCmd_Option()
{

    snprintf(m_buffer, sizeof(m_buffer),
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %u\r\n"
        "Public: DESCRIBE, ANNOUNCE, SETUP, PLAY, RECORD, PAUSE, GET_PARAMETER, TEARDOWN\r\n"
        "Server: %s\r\n"
        "\r\n", m_cseq, PROJECT_VERSION);

    if (Send_Message(m_buffer, strlen(m_buffer)) < 0)
        return false;

    return true;
}

bool RtspConnection::HandleCmd_Describe()
{
    MediaSession* session = m_rtspServer->m_sessmgr->Get_Session(m_suffix);

    if (!session) {
        LOGE("can't find session:%s", m_suffix.c_str());
        return false;
    }
    std::string sdp = session->Generate_SdpDescription();

    memset((void*)m_buffer, 0, sizeof(m_buffer));
    snprintf((char*)m_buffer, sizeof(m_buffer),
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %u\r\n"
        "Content-Length: %u\r\n"
        "Content-Type: application/sdp\r\n"
        "\r\n"
        "%s",
        m_cseq,
        (unsigned int)sdp.size(),
        sdp.c_str());

    if (Send_Message(m_buffer, strlen(m_buffer)) < 0)
        return false;

    return true;
}


bool RtspConnection::HandleCmd_Setup() {
    char sessionName[100];
    if (sscanf(m_suffix.c_str(), "%[^/]/", sessionName) != 1)
    {
        return false;
    }
    MediaSession* session = m_rtspServer->m_sessmgr->Get_Session(sessionName);
    if (!session) {
        LOGE("can't find session:%s", sessionName);
        return false;
    }

    if (m_trackId >= MEDIA_MAX_TRACK_NUM || m_rtpInstances[m_trackId] || m_rtcpInstances[m_trackId]) {
        return false;
    }

    if (session->Is_StartMulticast()) {
        snprintf((char*)m_buffer, sizeof(m_buffer),
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %d\r\n"
            "Transport: RTP/AVP;multicast;"
            "destination=%s;source=%s;port=%d-%d;ttl=255\r\n"
            "Session: %08x\r\n"
            "\r\n",
            m_cseq,
            session->GetMuliticast_DestAddr().c_str(),
            sockets::Get_LocalIp().c_str(),
            session->GetMuliticast_DestRtpPort(m_trackId),
            session->GetMuliticast_DestRtpPort(m_trackId) + 1,
            m_sessionId);
    }
    else {


        if (m_isRtp_overTcp)
        {
            //创建rtp over tcp
            CreateRtp_OverTcp(m_trackId, m_clientFd, m_rtpChannel);
            m_rtpInstances[m_trackId]->Set_SessionId(m_sessionId);

            session->Add_RtpInstance(m_trackId, m_rtpInstances[m_trackId]);

            snprintf((char*)m_buffer, sizeof(m_buffer),
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %d\r\n"
                "Server: %s\r\n"
                "Transport: RTP/AVP/TCP;unicast;interleaved=%hhu-%hhu\r\n"
                "Session: %08x\r\n"
                "\r\n",
                m_cseq, PROJECT_VERSION,
                m_rtpChannel,
                m_rtpChannel + 1,
                m_sessionId);
        }
        else
        {
            //创建 rtp over udp
            if (CreateRtpRtcp_OverUdp(m_trackId, m_peerIp, m_peerRtp_port, m_peerRtcp_port) != true)
            {
                LOGE("failed to CreateRtpRtcp_OverUdp");
                return false;
            }

            m_rtpInstances[m_trackId]->Set_SessionId(m_sessionId);
            m_rtcpInstances[m_trackId]->Set_SessionId(m_sessionId);


            session->Add_RtpInstance(m_trackId, m_rtpInstances[m_trackId]);

            snprintf((char*)m_buffer, sizeof(m_buffer),
                "RTSP/1.0 200 OK\r\n"
                "CSeq: %u\r\n"
                "Server: %s\r\n"
                "Transport: RTP/AVP;unicast;client_port=%hu-%hu;server_port=%hu-%hu\r\n"
                "Session: %08x\r\n"
                "\r\n",
                m_cseq, PROJECT_VERSION,
                m_peerRtp_port,
                m_peerRtcp_port,
                m_rtpInstances[m_trackId]->Get_LocalPort(),
                m_rtcpInstances[m_trackId]->Get_LocalPort(),
                m_sessionId);
        }

    }

    if (Send_Message(m_buffer, strlen(m_buffer)) < 0)
        return false;

    return true;
}

bool RtspConnection::HandleCmd_Play()
{
    snprintf((char*)m_buffer, sizeof(m_buffer),
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Server: %s\r\n"
        "Range: npt=0.000-\r\n"
        "Session: %08x; timeout=60\r\n"
        "\r\n",
        m_cseq, PROJECT_VERSION,
        m_sessionId);

    if (Send_Message(m_buffer, strlen(m_buffer)) < 0)
        return false;

    for (int i = 0; i < MEDIA_MAX_TRACK_NUM; ++i)
    {
        if (m_rtpInstances[i]) {
            m_rtpInstances[i]->Set_Alive(true);
        }

        if (m_rtcpInstances[i]) {
            m_rtcpInstances[i]->Set_Alive(true);
        }

    }

    return true;
}

bool RtspConnection::HandleCmd_Teardown()
{
    snprintf((char*)m_buffer, sizeof(m_buffer),
        "RTSP/1.0 200 OK\r\n"
        "CSeq: %d\r\n"
        "Server: %s\r\n"
        "\r\n",
        m_cseq, PROJECT_VERSION);

    if (Send_Message(m_buffer, strlen(m_buffer)) < 0)
    {
        return false;
    }

    return true;
}

int RtspConnection::Send_Message(void* buf, int size)
{
    LOGI("%s", buf);
    int ret;

    m_outBuffer.Append(buf, size);
    ret = m_outBuffer.Write(m_clientFd);
    m_outBuffer.Retrieve_All();

    return ret;
}

int RtspConnection::Send_Message()
{
    int ret = m_outBuffer.Write(m_clientFd);
    m_outBuffer.Retrieve_All();
    return ret;
}

bool RtspConnection::CreateRtpRtcp_OverUdp(MediaSession::TrackId trackId, std::string peerIp,
    uint16_t peerRtpPort, uint16_t peerRtcpPort)
{
    int rtpSockfd, rtcpSockfd;
    int16_t rtpPort, rtcpPort;
    bool ret;

    if (m_rtpInstances[trackId] || m_rtcpInstances[trackId])
        return false;

    int i;
    for (i = 0; i < 10; ++i) {// 重试10次
        rtpSockfd = sockets::Create_UdpSocket();
        if (rtpSockfd < 0)
        {
            return false;
        }

        rtcpSockfd = sockets::Create_UdpSocket();
        if (rtcpSockfd < 0)
        {
            sockets::close(rtpSockfd);
            return false;
        }

        uint16_t port = rand() & 0xfffe;
        if (port < 10000)
            port += 10000;

        rtpPort = port;
        rtcpPort = port + 1;

        ret = sockets::bind(rtpSockfd, "0.0.0.0", rtpPort);
        if (ret != true)
        {
            sockets::close(rtpSockfd);
            sockets::close(rtcpSockfd);
            continue;
        }

        ret = sockets::bind(rtcpSockfd, "0.0.0.0", rtcpPort);
        if (ret != true)
        {
            sockets::close(rtpSockfd);
            sockets::close(rtcpSockfd);
            continue;
        }

        break;
    }

    if (i == 10)
        return false;

    m_rtpInstances[trackId] = RtpInstance::CreateNew_OverUdp(rtpSockfd, rtpPort,
        peerIp, peerRtpPort);
    m_rtcpInstances[trackId] = RtcpInstance::Create_New(rtcpSockfd, rtcpPort,
        peerIp, peerRtcpPort);

    return true;
}

bool RtspConnection::CreateRtp_OverTcp(MediaSession::TrackId trackId, int sockfd,
    uint8_t rtpChannel)
{
    m_rtpInstances[trackId] = RtpInstance::CreateNew_OverTcp(sockfd, rtpChannel);

    return true;
}

void RtspConnection::HandleRtp_OverTcp()
{
    int num = 0;
    while (true)
    {
        num += 1;
        uint8_t* buf = (uint8_t*)m_inputBuffer.Peek();
        uint8_t rtpChannel = buf[1];
        int16_t rtpSize = (buf[2] << 8) | buf[3];

        int16_t bufSize = 4 + rtpSize;

        if (m_inputBuffer.Readable_Bytes() < bufSize) {
            // 缓存数据小于一个RTP数据包的长度
            return;
        }
        else {
            if (0x00 == rtpChannel) {
                RtpHeader rtpHeader;
                Parse_RtpHeader(buf + 4, &rtpHeader);
                LOGI("num=%d,rtpSize=%d", num, rtpSize);
            }
            else if (0x01 == rtpChannel)
            {
                RtcpHeader rtcpHeader;
                Parse_RtcpHeader(buf + 4, &rtcpHeader);

                LOGI("num=%d,rtcpHeader.packetType=%d,rtpSize=%d", num, rtcpHeader.packetType, rtpSize);
            }
            else if (0x02 == rtpChannel) {
                RtpHeader rtpHeader;
                Parse_RtpHeader(buf + 4, &rtpHeader);
                LOGI("num=%d,rtpSize=%d", num, rtpSize);
            }
            else if (0x03 == rtpChannel)
            {
                RtcpHeader rtcpHeader;
                Parse_RtcpHeader(buf + 4, &rtcpHeader);

                LOGI("num=%d,rtcpHeader.packetType=%d,rtpSize=%d", num, rtcpHeader.packetType, rtpSize);
            }

            m_inputBuffer.Retrieve(bufSize);
        }
    }

}