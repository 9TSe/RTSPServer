#include "UsageEnvironment.h"

UsageEnvironment::UsageEnvironment(EventScheduler* scheduler, ThreadPool* threadpool)
	:m_scheduler(scheduler),
	m_threadpool(threadpool)
{}

UsageEnvironment::~UsageEnvironment()
{}

UsageEnvironment* UsageEnvironment::Create_New(EventScheduler* scheduler, ThreadPool* threadpool)
{
	return new UsageEnvironment(scheduler, threadpool);
}

EventScheduler* UsageEnvironment::Scheduler()
{
	return m_scheduler;
}

ThreadPool* UsageEnvironment::Thread_Pool()
{
	return m_threadpool;
}