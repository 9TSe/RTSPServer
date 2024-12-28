#pragma once
#include <mutex>
#include <vector>
#include <memory>
#include "Timer.h"
#include "EpollPoller.h"


class IOEvent;
class EpollPoller;
class TimerManager;
class TriggerEvent;
class BoostPoller;

class EventScheduler
{
public:
	explicit EventScheduler();
	~EventScheduler();
	static std::unique_ptr<EventScheduler> createNew();

	bool addIOEvent(IOEvent* event) { return m_epoll->addIOEvent(event); }
	bool updateIOEvent(IOEvent* event) { return m_epoll->updateIOEvent(event); }
	bool removeIOEvent(IOEvent* event) { return m_epoll->removeIOEvent(event); }

	void addTriggerEvent(TriggerEvent* event) { m_triggerEvents.push_back(event); }
	Timer::TimerId addTimeEventRunEvery(TimerEvent* event, Timer::TimeInterval interval);

	void start();
	EpollPoller* getEpoll() { return m_epoll; }
private:
	void handleTriggerEvents();

private:
	EpollPoller* m_epoll;
	TimerManager* m_timerManager;
	std::vector<TriggerEvent*> m_triggerEvents;
	bool m_quit;
};