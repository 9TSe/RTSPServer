#include "UsageEnvironment.h"
#include "EventScheduler.h"
#include "ThreadPool.h"

UsageEnvironment::UsageEnvironment(ThreadPool* threadpool, EventScheduler* scheduler)
	:m_threadpool(threadpool)
	, m_eventscheduler(scheduler)
{}

UsageEnvironment* UsageEnvironment::createNew(ThreadPool* threadpool, EventScheduler* scheduler)
{
	return new UsageEnvironment(threadpool, scheduler);
}

UsageEnvironment::~UsageEnvironment()
{}