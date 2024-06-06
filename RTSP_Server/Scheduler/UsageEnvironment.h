#pragma once
class ThreadPool;
class EventScheduler;


class UsageEnvironment
{
public:
	UsageEnvironment(ThreadPool* threadpool, EventScheduler* scheduler);
	~UsageEnvironment();

	static UsageEnvironment* createNew(ThreadPool* threadpool, EventScheduler* scheduler);
	ThreadPool* threadPool() { return m_threadpool; }
	EventScheduler* eventScheduler() { return m_eventscheduler; }
private:
	ThreadPool* m_threadpool;
	EventScheduler* m_eventscheduler;
};