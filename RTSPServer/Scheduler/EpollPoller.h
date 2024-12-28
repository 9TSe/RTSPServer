#pragma once
#include <unordered_map>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>

class IOEvent;
class EpollPoller
{
public:
	static EpollPoller* createNew();
	EpollPoller();
	~EpollPoller();

	bool addIOEvent(IOEvent* event);
	bool updateIOEvent(IOEvent* event);
	bool removeIOEvent(IOEvent* event);
	void handleEvent();

private:
	int m_epollFd;
	std::unordered_map<int, IOEvent*> m_eventMap;
	std::vector<epoll_event> m_epollEvents;
};