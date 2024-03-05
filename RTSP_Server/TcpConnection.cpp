#include "TcpConnection.h"
#include "SocketsOps.h"
#include "Log.h"

TcpConnection::TcpConnection(UsageEnvironment* env, int clientfd)
	:m_env(env),
	m_clientFd(clientfd)
{
	m_client_IOEvent = IOEvent::Create_New(clientfd, this);
	m_client_IOEvent->Set_ReadCallback(Read_Callback);
	m_client_IOEvent->Set_WriteCallback(Write_Callback);
	m_client_IOEvent->Set_ErrorCallback(Error_Callback);
	m_client_IOEvent->Enable_Read_Handling(); //default only read

	m_env->Scheduler()->Add_IOEvent(m_client_IOEvent);
}

TcpConnection::~TcpConnection()
{
	m_env->Scheduler()->Remove_IOEvent(m_client_IOEvent);
	delete m_client_IOEvent;
	sockets::close(m_clientFd);
}

void TcpConnection::Set_Disconnect_Callback(DisConnectCallback callback, void* arg)
{
	m_disConnect_callback = callback;
	m_arg = arg;
}

void TcpConnection::Enable_Read_Handling()
{
	if (m_client_IOEvent->IsRead_Handling())
		return;
	m_client_IOEvent->Enable_Read_Handling();
	m_env->Scheduler()->Update_IOEvent(m_client_IOEvent);
}

void TcpConnection::Enable_Write_Handling()
{
	if (m_client_IOEvent->IsWrite_Handling())
		return;
	m_client_IOEvent->Enable_Write_Handling();
	m_env->Scheduler()->Update_IOEvent(m_client_IOEvent);
}

void TcpConnection::Enable_Error_Handling()
{
	if (m_client_IOEvent->IsError_Handling())
		return;
	m_client_IOEvent->Enable_Error_Handling();
	m_env->Scheduler()->Update_IOEvent(m_client_IOEvent);
}

void TcpConnection::Disable_Read_Handling()
{
	if (!m_client_IOEvent->IsRead_Handling())
		return;
	m_client_IOEvent->Disable_Read_Handling();
	m_env->Scheduler()->Update_IOEvent(m_client_IOEvent);
}

void TcpConnection::Disable_Write_Handling()
{
	if (!m_client_IOEvent->IsWrite_Handling())
		return;
	m_client_IOEvent->Disable_Write_Handling();
	m_env->Scheduler()->Update_IOEvent(m_client_IOEvent);
}

void TcpConnection::Disable_Error_Handling()
{
	if (!m_client_IOEvent->IsError_Handling())
		return;
	m_client_IOEvent->Disable_Error_Handling();
	m_env->Scheduler()->Update_IOEvent(m_client_IOEvent);
}

void TcpConnection::Handle_Read()
{
	int ret = m_inputBuffer.Read(m_clientFd);
	if (ret <= 0)
	{
		LOGE("read error, fd = %d, ret = %d", m_clientFd, ret);
		Handle_Disconnect();
		return;
	}
	Handle_ReadBytes();
}

void TcpConnection::Handle_ReadBytes()
{
	LOGI("");
	m_inputBuffer.Retrieve_All();
}

void TcpConnection::Handle_Disconnect()
{
	if (m_disConnect_callback)
		m_disConnect_callback(m_arg, m_clientFd);
}

void TcpConnection::Handle_Write()
{
	LOGI("");
	m_outBuffer.Retrieve_All();
}

void TcpConnection::Handle_Error()
{
	LOGI("");
}

void TcpConnection::Read_Callback(void* arg)
{
	TcpConnection* tcpconnection = (TcpConnection*)arg;
	tcpconnection->Handle_Read();
}

void TcpConnection::Write_Callback(void* arg)
{
	TcpConnection* tcpconnection = (TcpConnection*)arg;
	tcpconnection->Handle_Write();
}

void TcpConnection::Error_Callback(void* arg)
{
	TcpConnection* tcpconnection = (TcpConnection*)arg;
	tcpconnection->Handle_Error();
}