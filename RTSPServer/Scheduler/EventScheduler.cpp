#include "EventScheduler.h"
#include "EpollPoller.h"
#include "Event.h"


std::unique_ptr<EventScheduler> EventScheduler::createNew()
{
	return std::make_unique<EventScheduler>();
}

EventScheduler::EventScheduler()
	:m_quit(false)
{
	m_epoll = EpollPoller::createNew();
	m_timerManager = TimerManager::createNew(this);
}

EventScheduler::~EventScheduler()
{
	LOG_CORE_INFO("~EventScheduler");
	delete m_epoll;
	delete m_timerManager;
}

void EventScheduler::start()
{
	while (!m_quit)
	{
		handleTriggerEvents();
		m_epoll->handleEvent();
	}
}

void EventScheduler::handleTriggerEvents()
{
	if (!m_triggerEvents.empty())
	{
		for (auto& it : m_triggerEvents)
			it->handleEvent();
		m_triggerEvents.clear();
	}
}

Timer::TimerId EventScheduler::addTimeEventRunEvery(TimerEvent* event, Timer::TimeInterval interval)
{
	Timer::TimeStamp stamp = Timer::getCurTime();
	stamp += interval;
	return m_timerManager->addTimer(event, stamp, interval);
}