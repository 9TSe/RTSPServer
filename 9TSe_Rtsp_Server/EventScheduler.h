#pragma once
#include <vector>
#include <queue>
#include <mutex>
#include <stdint.h>
#include "Timer.h"
#include "Event.h"

class Poller;

class EventScheduler
{
public:
	enum PollerType
	{
		POLLER_SELECT,
		POLLER_POLL,
		POLLER_EPOLL
	};

	explicit EventScheduler(PollerType type);
	virtual ~EventScheduler();
	static EventScheduler* Create_New(PollerType type);

	bool Add_TriggerEvent(TriggerEvent* event);
	Timer::TimerId Add_TimedEvent_RunAfter(TimerEvent* event, Timer::TimeInterval delay);
	Timer::TimerId Add_TimedEvent_RunAt(TimerEvent* event, Timer::TimeStamp when);
	Timer::TimerId Add_TimedEvent_RunEvery(TimerEvent* event, Timer::TimeInterval interval);
	bool Remove_TimedEvent(Timer::TimerId timerid);
	bool Add_IOEvent(IOEvent* event);
	bool Update_IOEvent(IOEvent* event);
	bool Remove_IOEvent(IOEvent* event);

	void Loop();
	Poller* poller();
	void Set_TimerManager_ReadCallback(EventCallback callback, void* arg);

private:
	void Handle_TriggerEvents();

private:
	bool m_quit;
	Poller* m_poller;
	TimerManager* m_timerManager;
	std::vector<TriggerEvent*> m_triggerEvent;
	std::mutex m_mutex;

	EventCallback m_timerManager_readCallback;
	void* m_timerManager_arg;
};