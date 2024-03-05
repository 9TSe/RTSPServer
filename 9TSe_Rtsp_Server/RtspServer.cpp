#include "RtspServer.h"
#include "RtspConnection.h"
#include "Log.h"

RtspServer* RtspServer::Create_New(UsageEnvironment* env, MediaSessionManager* sessmgr, Ipv4Address& addr) 
{
    return new RtspServer(env, sessmgr, addr);
}
RtspServer::RtspServer(UsageEnvironment* env, MediaSessionManager* sessmgr, Ipv4Address& addr) 
    :m_sessmgr(sessmgr),
    m_env(env),
    m_addr(addr),
    m_listen(false)
{
    m_fd = sockets::Create_TcpSocket(); //����ʱ���Ѿ��Ƿ�������
    sockets::Set_ReuseAddr(m_fd, 1); //���ö˿ڸ���,����ȴ�MSLʱ��
    if (!sockets::bind(m_fd, addr.Get_Ip(), m_addr.Get_Port()))
        return;

    LOGI("rtsp://%s:%d fd=%d", addr.Get_Ip().data(), addr.Get_Port(), m_fd);

    m_acceptIOEvent = IOEvent::Create_New(m_fd, this);
    //���һ�����ӷ���������,��ô�ͻᴥ���ɶ��¼�(readcallback
    m_acceptIOEvent->Set_ReadCallback(Read_Callback);//���ûص���socket�ɶ� ����ָ��
    m_acceptIOEvent->Enable_Read_Handling(); //����ص�����(�ɶ�

    m_closeTriggerEvent = TriggerEvent::Create_New(this);
    m_closeTriggerEvent->Set_TriggerCallback(Callback_CloseConnect);//���ûص��Ĺر����� ����ָ��
    //��ѭ������ʱ�Żᴥ��Callback_CloseConnect����
}

RtspServer::~RtspServer()
{
    if (m_listen)
        m_env->Scheduler()->Remove_IOEvent(m_acceptIOEvent);
    delete m_acceptIOEvent;
    delete m_closeTriggerEvent;
    sockets::close(m_fd);
}

void RtspServer::Start() 
{
    LOGI("");
    m_listen = true;
    sockets::listen(m_fd, 60);
    m_env->Scheduler()->Add_IOEvent(m_acceptIOEvent);
}

void RtspServer::Read_Callback(void* arg) 
{
    RtspServer* rtspserver = (RtspServer*)arg;
    rtspserver->Handle_Read();
}

void RtspServer::Handle_Read() 
{
    int clientfd = sockets::accept(m_fd);
    if (clientfd < 0)
    {
        LOGE("Handle_Read error,clientfd=%d", clientfd);
        return;
    }
    //����������,�Ϳɴ����ر�����
    RtspConnection* conn = RtspConnection::Create_New(this, clientfd);
    //����Callback_DisConnect����,this��Ϊarg�������ݹ�ȥ
    conn->Set_Disconnect_Callback(RtspServer::Callback_DisConnect, this);
    m_connMap.insert(std::make_pair(clientfd, conn));

}
void RtspServer::Callback_DisConnect(void* arg, int clientfd)
{
    RtspServer* server = (RtspServer*)arg;
    server->Handle_DisConnect(clientfd);
}

void RtspServer::Handle_DisConnect(int clientfd) 
{
    std::lock_guard <std::mutex> lck(m_mutex);
    m_disConnList.push_back(clientfd); //����ȡ�����Ӷ�����

    m_env->Scheduler()->Add_TriggerEvent(m_closeTriggerEvent);
    //��ʱ�ἤ��m_closeTriggerEvent�еĻص�����,������
}

void RtspServer::Callback_CloseConnect(void* arg) 
{
    RtspServer* server = (RtspServer*)arg;
    server->Handle_CloseConnect();
}
void RtspServer::Handle_CloseConnect() 
{
    std::lock_guard <std::mutex> lck(m_mutex);

    for (std::vector<int>::iterator it = m_disConnList.begin(); it != m_disConnList.end(); ++it) 
    {
        int clientfd = *it;
        std::map<int, RtspConnection*>::iterator _it = m_connMap.find(clientfd);
        assert(_it != m_connMap.end());
        delete _it->second;
        m_connMap.erase(clientfd);
    }
    m_disConnList.clear();
}