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
    m_fd = sockets::Create_TcpSocket(); //创建时就已经是非阻塞的
    sockets::Set_ReuseAddr(m_fd, 1); //设置端口复用,无需等待MSL时间
    if (!sockets::bind(m_fd, addr.Get_Ip(), m_addr.Get_Port()))
        return;

    LOGI("rtsp://%s:%d fd=%d", addr.Get_Ip().data(), addr.Get_Port(), m_fd);

    m_acceptIOEvent = IOEvent::Create_New(m_fd, this);
    //如果一个连接发出了请求,那么就会触发可读事件(readcallback
    m_acceptIOEvent->Set_ReadCallback(Read_Callback);//设置回调的socket可读 函数指针
    m_acceptIOEvent->Enable_Read_Handling(); //激活回调函数(可读

    m_closeTriggerEvent = TriggerEvent::Create_New(this);
    m_closeTriggerEvent->Set_TriggerCallback(Callback_CloseConnect);//设置回调的关闭连接 函数指针
    //被循环激活时才会触发Callback_CloseConnect函数
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
    //播放完走了,就可触发关闭连接
    RtspConnection* conn = RtspConnection::Create_New(this, clientfd);
    //触发Callback_DisConnect函数,this作为arg参数传递过去
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
    m_disConnList.push_back(clientfd); //插入取消连接队列中

    m_env->Scheduler()->Add_TriggerEvent(m_closeTriggerEvent);
    //此时会激活m_closeTriggerEvent中的回调函数,即如下
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