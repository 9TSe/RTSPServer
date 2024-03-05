#pragma once
#include "ThreadPool.h"
#include "EventScheduler.h"

class UsageEnvironment
{
public:
	UsageEnvironment(EventScheduler* scheduler, ThreadPool* threadpool);
	~UsageEnvironment();

	static UsageEnvironment* Create_New(EventScheduler* scheduler, ThreadPool* threadpool);
	EventScheduler* Scheduler();
	ThreadPool* Thread_Pool();

private:
	EventScheduler* m_scheduler;
	ThreadPool* m_threadpool;
};