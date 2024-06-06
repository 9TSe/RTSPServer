#pragma once
#include <stdint.h>
#include <map>
#include <chrono>

class TimerEvent; 
class EpollPoller; 
class EventScheduler; 
class IOEvent;

class Timer
{
public:
	using TimerId = uint32_t;
	using TimeStamp = int64_t;
	using TimeInterval = uint32_t;

	~Timer();
	static TimeStamp getCurTime();

	TimeStamp getStamp() { return m_timestamp; }
	TimeInterval getInterval() { return m_timeinterval; }

private:
	friend class TimerManager;
	Timer(TimerEvent* event, TimeStamp timestamp, TimeInterval timeinterval, TimerId timerid);
	bool handleEvent();

	TimerEvent* m_timerEvent;
	TimerId m_timerid;
	TimeStamp m_timestamp;
	TimeInterval m_timeinterval;
	bool m_repeat;
};

class TimerManager
{
public:
	static TimerManager* createNew(EventScheduler* scheduler);
	TimerManager(EventScheduler* scheduler);
	~TimerManager();

	Timer::TimerId addTimer(TimerEvent* timevent, Timer::TimeStamp stamp, Timer::TimeInterval interval);
	bool removeTimer(Timer::TimerId timerId);
private:
	static void readCallback(void* arg);
	void handleRead();
	void modifyTimeout();

private:
	EpollPoller* m_epoll;
	std::map<Timer::TimerId, Timer> m_timers;
	std::multimap<Timer::TimeStamp, Timer> m_events; //1 k reflect n v
	Timer::TimerId m_lastTimeid;
	int m_timerFd;
	IOEvent* m_timeIOevent;
};