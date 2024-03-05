#pragma once
#include "Poller.h"
#include <vector>

#ifndef _WIN32
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#else
#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#endif

class SelectPoller : public Poller
{
public:
	SelectPoller();
	virtual ~SelectPoller();

	static SelectPoller* Create_New();

	virtual bool Add_IOEvent(IOEvent* event);
	virtual bool Update_IOEvent(IOEvent* event);
	virtual bool Remove_IOEvent(IOEvent* event);
	virtual void Handle_Event();

private:
	fd_set m_readSet;
	fd_set m_writeSet;
	fd_set m_exceptionSet;
	int m_maxNumSockets;
	std::vector<IOEvent*> m_IOEvent; // sava temporary activity ioevent object
};