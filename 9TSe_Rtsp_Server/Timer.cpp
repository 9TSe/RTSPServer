#include "Timer.h"
#include "Event.h"
#include "EventScheduler.h"
#include "Poller.h"
#include "Log.h"
#include <time.h>
#include <chrono>

#ifndef _WIN32
#include <sys/timerfd.h>
#endif

/*
struct timespec
{
	time_t tv_sec;
	long   tv_nsec;
};

struct itimerspec
{
	timespec it_interval; //定时间隔周期
	timespec it_value;	  //第一次超时时间
};
//it_interval not zero -> cyclic timer
//it_value and it_interval == 0 -> stop timer
*/

static bool TimerFd_SetTime(int fd, Timer::TimeStamp when, Timer::TimeInterval period)
{
#ifndef _WIN32
	itimerspec newval;
	newval.it_value.tv_sec = when / 1000;
	newval.it_value.tv_nsec = when % 1000 * 1000 * 1000;
	newval.it_interval.tv_sec = period / 1000;
	newval.it_interval.tv_nsec = period % 1000 * 1000 * 1000;

	int oldvalue = timerfd_settime(fd, TFD_TIMER_ABSTIME, &newval, nullptr);
	if (oldvalue < 0)
		return false;
	return true;
#endif
	return true;
}

Timer::Timer(TimerEvent* event, TimeStamp timestamp, TimeInterval timeinterval, TimerId timerid)
	:m_timerEvent(event),
	m_timeStamp(timestamp),
	m_timeInterval(timeinterval),
	m_timerId(timerid)
{
	if (timeinterval > 0)
		m_repeat = true; //repeat->timer
	else
		m_repeat = false;
}

Timer::~Timer()
{}

Timer::TimeStamp Timer::Get_CurTime()
{
#ifndef _WIN32
	timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return (now.tv_sec * 1000 + now.tv_nsec / 1000000);
#else
	long long now = std::chrono::steady_clock::now().time_since_epoch().count();
	return now / 1000000;
#endif
}

Timer::TimeStamp Timer::Get_CurTimeStamp()
{
	//cast -> time to time
	return std::chrono::duration_cast<std::chrono::milliseconds>
		(std::chrono::system_clock::now().time_since_epoch()).count();
}

bool Timer::Handle_Event()
{
	if (!m_timerEvent)
		return false;
	return m_timerEvent->Handle_Event();
}



TimerManager::TimerManager(EventScheduler* scheduler)
	:m_poller(scheduler->poller()),
	m_lastTimerid(0)
{
#ifndef _WIN32
	m_timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	if (m_timerFd < 0)
	{
		LOGE("create timerFd error");
		return;
	}
	else
		LOGI("fd = %d", m_timerFd);

	m_timerIOEvent = IOEvent::Create_New(m_timerFd, this);
	m_timerIOEvent->Set_ReadCallback(Read_Callback);
	m_timerIOEvent->Enable_Read_Handling();
	Modify_Timeout();
	m_poller->Add_IOEvent(m_timerIOEvent);
#else
	scheduler->Set_TimerManager_ReadCallback(Read_Callback, this);
#endif
}

TimerManager* TimerManager::Create_New(EventScheduler* scheduler)
{
	if (!scheduler)
		return nullptr;
	return new TimerManager(scheduler);
}

TimerManager::~TimerManager()
{
#ifndef _WIN32
	m_poller->Remove_IOEvent(m_timerIOEvent);
	delete m_timerIOEvent;
#endif
}

void TimerManager::Read_Callback(void* arg)
{
	TimerManager* timermanager = (TimerManager*)arg;
	timermanager->Handle_Read();
}

void TimerManager::Handle_Read()
{
	Timer::TimeStamp timestamp = Timer::Get_CurTime();
	if (!m_timers.empty() && !m_events.empty())
	{
		std::multimap<Timer::TimeStamp, Timer>::iterator it = m_events.begin();
		Timer timer = it->second;
		int expire = timer.m_timeStamp - timestamp;

		if (timer.m_timeStamp < timestamp || expire == 0) //expire <= 0
		{
			bool timerEvent_isStop = timer.Handle_Event(); //exe->false
			m_events.erase(it);
			if (timer.m_repeat)
			{
				if (timerEvent_isStop)
					m_timers.erase(timer.m_timerId);
				else
				{
					timer.m_timeStamp = timestamp + timer.m_timeInterval;
					m_events.insert(std::make_pair(timer.m_timeStamp, timer));
				}
			}
			else
				m_timers.erase(timer.m_timerId);
		}
	}
	Modify_Timeout();
}

void TimerManager::Modify_Timeout()
{
#ifndef _WIN32
	std::multimap<Timer::TimeStamp, Timer>::iterator it = m_events.begin();
	if (it != m_events.end()) //exist >= 1 timer
	{
		Timer timer = it->second;
		TimerFd_SetTime(m_timerFd, timer.m_timeStamp, timer.m_timeInterval);
	}
	else
		TimerFd_SetTime(m_timerFd, 0, 0);
#endif
}

bool TimerManager::Remove_Timer(Timer::TimerId timerid)
{
	std::map<Timer::TimerId, Timer>::iterator it = m_timers.find(timerid);
	if (it != m_timers.end())
		m_timers.erase(timerid);

	Modify_Timeout();
	return true;
}

Timer::TimerId TimerManager::Add_Timer(TimerEvent* event, Timer::TimeStamp timestamp,
	Timer::TimeInterval timeinterval)
{
	++m_lastTimerid;
	Timer timer(event, timestamp, timeinterval, m_lastTimerid);

	m_timers.insert(std::make_pair(m_lastTimerid, timer));
	m_events.insert(std::make_pair(timestamp, timer));
	Modify_Timeout();
	return m_lastTimerid;
}