#pragma once
#include "Buffer.h"

class UsageEnvironment;
class IOEvent;

class TcpConnection
{
public:
	using DisConnectCallback = void(*)(void*, int);
	TcpConnection(UsageEnvironment* env, int fd);
	virtual ~TcpConnection();

	void setDisConnectCallback(DisConnectCallback callback, void* arg);

protected:
	void handleRead();
	void handleDisconnect();
	virtual void handleReadBytes();

private:
	static void readCallback(void* arg);

protected:
	UsageEnvironment* m_env;
	int m_fd;
	IOEvent* m_clientIOEvent;
	DisConnectCallback m_disconCallback;
	void* m_arg;

	Buffer m_inputbuf;
	Buffer m_outputbuf;
	char m_buffer[2048];
};