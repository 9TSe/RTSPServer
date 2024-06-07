#include "Timer.h"
#include "Event.h"
#include "EpollPoller.h"
#include "EventScheduler.h"
#include "Log.h"
#include <sys/timerfd.h>


Timer::Timer(TimerEvent* event, TimeStamp stamp, TimeInterval interval, TimerId id)
	:m_timerEvent(event)
	, m_timerid(id)
	, m_timestamp(stamp)
	, m_timeinterval(interval)
{
	if (m_timeinterval > 0)
		m_repeat = true;
	else
		m_repeat = false;
}

Timer::~Timer(){}

bool Timer::handleEvent()
{
	if (!m_timerEvent)
		return false;
	return m_timerEvent->handleEvent();
}


Timer::TimeStamp Timer::getCurTime() //for compute "func" use time, could use twice to achive
{
	//return std::chrono::steady_clock::now().time_since_epoch().count();
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (now.tv_sec * 1000 + now.tv_nsec / 1000000);
}



static bool timerFdSetTime(int fd, Timer::TimeStamp stamp, Timer::TimeInterval interval)
{
	itimerspec timespec;
	timespec.it_value.tv_sec = stamp / 1000;
	timespec.it_value.tv_nsec = stamp % 1000 * 1000 * 1000; //ms -> ns
	timespec.it_interval.tv_sec = interval / 1000;
	timespec.it_interval.tv_nsec = interval % 1000 * 1000 * 1000;
	int tmp = timerfd_settime(fd, TFD_TIMER_ABSTIME, &timespec, nullptr);
	if (tmp < 0)
	{
		return false;
		LOGE("timerfd settime error");
	}
	return true;
}

TimerManager::TimerManager(EventScheduler* scheduler)
	:m_epoll(scheduler->getEpoll())
	, m_lastTimeid(0)
{
	m_timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
	if (m_timerFd < 0)
	{
		LOGE("timerfd create fail");
		return;
	}
	else
		LOGI("m_timerfd = %d", m_timerFd);

	m_timeIOevent = IOEvent::createNew(m_timerFd, this);
	m_timeIOevent->setReadCallback(readCallback);
	m_timeIOevent->enableReadEvent();
	modifyTimeout();
	m_epoll->addIOEvent(m_timeIOevent);
}

TimerManager* TimerManager::createNew(EventScheduler* scheduler)
{
	if (!scheduler) return nullptr;
	return new TimerManager(scheduler);
}

TimerManager::~TimerManager()
{
	m_epoll->removeIOEvent(m_timeIOevent);
	delete m_timeIOevent;
}


void TimerManager::readCallback(void* arg)
{
	TimerManager* timermanager = (TimerManager*)arg;
	timermanager->handleRead();
}

void TimerManager::handleRead()
{
	Timer::TimeStamp stamp = Timer::getCurTime();
	if (!m_events.empty() && !m_timers.empty())
	{
		auto it = m_events.begin();
		Timer timer = it->second;
		int64_t expire = timer.m_timestamp - stamp;
		if (expire <= 0) //before
		{
			bool timevent_isStop = timer.handleEvent(); //exe -> false
			m_events.erase(it);
			if (timer.m_repeat) //interval > 0
			{
				if (timevent_isStop)
					m_timers.erase(timer.m_timerid);
				else
				{
					timer.m_timestamp = stamp + timer.m_timeinterval;
					m_events.insert(std::make_pair(timer.m_timestamp, timer));
				}
			}
			else //interval == 0
				m_timers.erase(timer.m_timerid);
		}
	}
	modifyTimeout();
}

void TimerManager::modifyTimeout()
{
	auto it = m_events.begin(); //min stamp
	if (it != m_events.end()) //exist >= 1
	{
		Timer timer = it->second;
		timerFdSetTime(m_timerFd, timer.getStamp(), timer.getInterval());
	}
	else
		timerFdSetTime(m_timerFd, 0, 0);
}


Timer::TimerId TimerManager::addTimer(TimerEvent* timevent, Timer::TimeStamp stamp, Timer::TimeInterval interval)
{
	++m_lastTimeid;
	//Timer time(timevent, m_lastTimeid, stamp, interval);
	Timer time(timevent, stamp, interval, m_lastTimeid);
	m_timers.insert(std::make_pair(m_lastTimeid, time));
	m_events.insert(std::make_pair(stamp, time));
	modifyTimeout();
	return m_lastTimeid;
}

bool TimerManager::removeTimer(Timer::TimerId timerId)
{
	auto it = m_timers.find(timerId);
	if (it != m_timers.end())
		m_timers.erase(timerId);
	modifyTimeout();
	return true;
}