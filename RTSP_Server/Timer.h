#pragma once
#include <map>
#include <stdint.h>

class EventScheduler;
class Poller;
class TimerEvent;
class IOEvent;

class Timer
{
public:
	using TimerId = uint32_t;
	using TimeStamp = int64_t;
	using TimeInterval = uint32_t;

	~Timer();
	//ms
	static TimeStamp Get_CurTime(); 
	static TimeStamp Get_CurTimeStamp();

private:
	friend class TimerManager;
	Timer(TimerEvent* event, TimeStamp timestamp, TimeInterval timeinterval, TimerId timerid);

	bool Handle_Event();

private:
	TimerEvent* m_timerEvent;
	TimeStamp m_timeStamp;
	TimeInterval m_timeInterval;
	TimerId m_timerId;
	bool m_repeat;
};



class TimerManager
{
public:
	TimerManager(EventScheduler* scheduler);
	~TimerManager();

	static TimerManager* Create_New(EventScheduler* scheduler);
	Timer::TimerId Add_Timer(TimerEvent* event, Timer::TimeStamp timestamp,
		Timer::TimeInterval timeinterval);
	bool Remove_Timer(Timer::TimerId timerid);

private:
	static void Read_Callback(void* arg);
	void Handle_Read();
	void Modify_Timeout();

private:
	Poller* m_poller;
	std::map<Timer::TimerId, Timer> m_timers;
	std::multimap<Timer::TimeStamp, Timer> m_events; //one key reflect n value
	uint32_t m_lastTimerid;

#ifndef _WIN32
	int m_timerFd;
	IOEvent* m_timerIOEvent;
#endif 
};

