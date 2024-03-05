#pragma once
#include <mutex>
#include "UsageEnvironment.h"
#include "Event.h"
#include "MediaSession.h"
#include "InetAddress.h"

class MediaSessionManager;
class RtspConnection;
class RtspServer 
{
public:
    static RtspServer* Create_New(UsageEnvironment* env, MediaSessionManager* sessmgr, Ipv4Address& addr);

    RtspServer(UsageEnvironment* env, MediaSessionManager* sessmgr, Ipv4Address& addr);
    ~RtspServer();

public:
    MediaSessionManager* m_sessmgr;
    void Start();
    UsageEnvironment* Env() const {return m_env;}
private:
    static void Read_Callback(void*);
    void Handle_Read();
    static void Callback_DisConnect(void* arg, int clientfd);
    void Handle_DisConnect(int clientFd);
    static void Callback_CloseConnect(void* arg);
    void Handle_CloseConnect();

private:
    UsageEnvironment* m_env;
    int  m_fd;
    Ipv4Address m_addr;
    bool m_listen;
    IOEvent* m_acceptIOEvent;
    std::mutex m_mutex;

    std::map<int, RtspConnection*> m_connMap; // <clientFd,conn> ά�����б�����������
    std::vector<int> m_disConnList;//���б�ȡ�������� clientFd
    TriggerEvent* m_closeTriggerEvent;// �ر����ӵĴ����¼�

};