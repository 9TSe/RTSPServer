#include "EventScheduler.h"
#include "SocketsOps.h"
#include "SelectPoller.h"
#include "Log.h"

#ifndef _WIN32
#include <sys/eventfd.h>
#endif


EventScheduler::EventScheduler(PollerType type)
	:m_quit(false)
{
#ifdef _WIN32
	WSADATA win_sockmsg;
	int ret = WSAStartup(MAKEWORD(2, 2), &win_sockmsg);
	if (ret != 0)
	{
		switch (ret)
		{
		case WSASYSNOTREADY: { //no tready
			LOGE("restart pc or check internet library");
			break;
			}
		case WSAVERNOTSUPPORTED: {
			LOGE("update internet library");
			break;
			}
		case WSAEPROCLIM: {
			LOGE("shut unnecessary app to ensure enough internet source");
			break;
			}
		case WSAEINPROGRESS: {
			LOGE("restart");
			break;
			}
		}
	}

	if (HIBYTE(win_sockmsg.wVersion) != 2 || LOBYTE(win_sockmsg.wVersion) != 2)
	{
		LOGE("internet library version error");
		return;
	}
#endif

	switch (type)
	{
	case POLLER_SELECT:
		m_poller = SelectPoller::Create_New();
		break;
	/*
	case POLLER_EPOLL:
		m_poller = EPollPoller::Create_New();
		break;
	*/
	default:
		_exit(-1);
		break;
	}
	m_timerManager = TimerManager::Create_New(this);
	//win系统的定时器回调由子线程托管, 非win系统则通过select网络模型
}

EventScheduler::~EventScheduler()
{
	delete m_timerManager;
	delete m_poller;

#ifdef _WIN32
	WSACleanup();
#endif
}

EventScheduler* EventScheduler::Create_New(PollerType type)
{
	if (type != POLLER_SELECT && type != POLLER_POLL && type != POLLER_EPOLL)
		return nullptr;
	return new EventScheduler(type);
}

bool EventScheduler::Add_TriggerEvent(TriggerEvent* event)
{
	m_triggerEvent.push_back(event);
	return true;
}

Timer::TimerId EventScheduler::Add_TimedEvent_RunAfter(TimerEvent* event, Timer::TimeInterval delay)
{
	Timer::TimeStamp timestamp = Timer::Get_CurTime();
	timestamp += delay;
	return m_timerManager->Add_Timer(event, timestamp, 0);
}

Timer::TimerId EventScheduler::Add_TimedEvent_RunAt(TimerEvent* event, Timer::TimeStamp when)
{
	return m_timerManager->Add_Timer(event, when, 0);
}

Timer::TimerId EventScheduler::Add_TimedEvent_RunEvery(TimerEvent* event, Timer::TimeInterval interval)
{
	Timer::TimeStamp timestamp = Timer::Get_CurTime();
	timestamp += interval;
	return m_timerManager->Add_Timer(event, timestamp, interval);
}

bool EventScheduler::Remove_TimedEvent(Timer::TimerId timerid)
{
	return m_timerManager->Remove_Timer(timerid);
}

bool EventScheduler::Add_IOEvent(IOEvent* event)
{
	return m_poller->Add_IOEvent(event);
}

bool EventScheduler::Update_IOEvent(IOEvent* event)
{
	return m_poller->Update_IOEvent(event);
}

bool EventScheduler::Remove_IOEvent(IOEvent* event)
{
	return m_poller->Remove_IOEvent(event);
}

void EventScheduler::Loop()
{
#ifdef _WIN32
	std::thread([](EventScheduler* sch) {
		while (!sch->m_quit)
		{
			if (sch->m_timerManager_readCallback)
				sch->m_timerManager_readCallback(sch->m_timerManager_arg);
		}
		}, this).detach();
#endif
		while (!m_quit)
		{
			Handle_TriggerEvents();
			m_poller->Handle_Event();
		}
}

void EventScheduler::Handle_TriggerEvents()
{
	if (!m_triggerEvent.empty())
	{
		for (std::vector<TriggerEvent*>::iterator it = m_triggerEvent.begin();
			it != m_triggerEvent.end(); ++it)
		{
			(*it)->Handle_Event();
		}

		m_triggerEvent.clear();
	}
}

Poller* EventScheduler::poller()
{
	return m_poller;
}

void EventScheduler::Set_TimerManager_ReadCallback(EventCallback callback, void* arg)
{
	m_timerManager_readCallback = callback;
	m_timerManager_arg = arg;
}