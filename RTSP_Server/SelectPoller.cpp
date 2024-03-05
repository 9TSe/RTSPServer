#include "SelectPoller.h"
#include "Log.h"

SelectPoller::SelectPoller()
{
	FD_ZERO(&m_readSet); //set fd zero
	FD_ZERO(&m_writeSet);
	FD_ZERO(&m_exceptionSet);
}

SelectPoller::~SelectPoller()
{}

SelectPoller* SelectPoller::Create_New()
{
	return new SelectPoller();
}

bool SelectPoller::Add_IOEvent(IOEvent* event)
{
	return Update_IOEvent(event);
}

bool SelectPoller::Update_IOEvent(IOEvent* event)
{
	int fd = event->Get_Fd();
	if (fd < 0)
	{
		LOGE("fd = %d", fd);
		return false;
	}

	FD_CLR(fd, &m_readSet); //clear fd which from m_readSet
	FD_CLR(fd, &m_writeSet);
	FD_CLR(fd, &m_exceptionSet);

	IOEventMap::iterator it = m_eventMap.find(fd);
	if (it != m_eventMap.end())
	{
		if (event->IsRead_Handling())
			FD_SET(fd, &m_readSet);
		if (event->IsWrite_Handling())
			FD_SET(fd, &m_writeSet);
		if (event->IsError_Handling())
			FD_SET(fd, &m_exceptionSet);
	}
	else
	{
		if (event->IsRead_Handling())
			FD_SET(fd, &m_readSet);
		if (event->IsWrite_Handling())
			FD_SET(fd, &m_writeSet);
		if (event->IsError_Handling())
			FD_SET(fd, &m_exceptionSet);

		m_eventMap.insert(std::make_pair(fd, event));
	}

	if (m_eventMap.empty())
		m_maxNumSockets = 0;
	else
		m_maxNumSockets = m_eventMap.rbegin()->first + 1;

	return true;
}

bool SelectPoller::Remove_IOEvent(IOEvent* event)
{
	int fd = event->Get_Fd();
	if (fd < 0)
		return false;

	FD_CLR(fd, &m_readSet);
	FD_CLR(fd, &m_writeSet);
	FD_CLR(fd, &m_exceptionSet);

	IOEventMap::iterator it = m_eventMap.find(fd);
	if (it != m_eventMap.end())
		m_eventMap.erase(it);

	if (m_eventMap.empty())
		m_maxNumSockets = 0;
	else
		m_maxNumSockets = m_eventMap.rbegin()->first + 1;

	return true;
}

void SelectPoller::Handle_Event()
{
	fd_set readset = m_readSet;
	fd_set writeset = m_writeSet;
	fd_set exceptionset = m_exceptionSet;

	timeval timeout;
	timeout.tv_sec = 1000;
	timeout.tv_usec = 0;

	int ret = select(m_maxNumSockets, &readset, &writeset, &exceptionset, &timeout);
	if (ret < 0)
		return;

	int present_event = 0;
	for (IOEventMap::iterator it = m_eventMap.begin(); it != m_eventMap.end(); ++it)
	{
		present_event = 0;

		if (FD_ISSET(it->first, &readset))
			present_event |= IOEvent::EVENT_READ;
		if (FD_ISSET(it->first, &writeset))
			present_event |= IOEvent::EVENT_WRITE;
		if (FD_ISSET(it->first, &exceptionset))
			present_event |= IOEvent::EVENT_ERROR;

		if (present_event != 0)
		{
			it->second->Set_REvent(present_event);
			m_IOEvent.push_back(it->second);
		}
	}

	for (auto& ioevent : m_IOEvent)
		ioevent->Handle_Event();

	m_IOEvent.clear();
}