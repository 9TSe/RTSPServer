#pragma once
#include "UsageEnvironment.h"
#include "Event.h"
#include "Buffer.h"

class TcpConnection
{
public:
	typedef void (*DisConnectCallback)(void*, int);
	TcpConnection(UsageEnvironment* env, int clientfd);
	virtual ~TcpConnection();

	void Set_Disconnect_Callback(DisConnectCallback callback, void* arg);

protected:
	void Enable_Read_Handling();
	void Enable_Write_Handling();
	void Enable_Error_Handling();
	void Disable_Read_Handling();
	void Disable_Write_Handling();
	void Disable_Error_Handling();

	void Handle_Read();
	virtual void Handle_ReadBytes();
	virtual void Handle_Write();
	virtual void Handle_Error();
	void Handle_Disconnect();

private:
	static void Read_Callback(void* arg);
	static void Write_Callback(void* arg);
	static void Error_Callback(void* arg);

protected:
	UsageEnvironment* m_env;
	int m_clientFd;
	IOEvent* m_client_IOEvent;
	DisConnectCallback m_disConnect_callback;
	void* m_arg;
	Buffer m_inputBuffer;
	Buffer m_outBuffer;
	char m_buffer[2048];
};