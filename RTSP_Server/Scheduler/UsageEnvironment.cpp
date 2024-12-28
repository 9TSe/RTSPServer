#include "UsageEnvironment.h"
#include "EventScheduler.h"
#include "ThreadPool.h"
#include "Log.h"

UsageEnvironment::UsageEnvironment(std::shared_ptr<ThreadPool>&& threadpool, std::shared_ptr<EventScheduler>&& scheduler)
	:m_threadpool(std::move(threadpool))
	,m_eventscheduler(std::move(scheduler))
{}

std::shared_ptr<UsageEnvironment> UsageEnvironment::createNew(std::shared_ptr<ThreadPool>&& threadpool, std::shared_ptr<EventScheduler>&& scheduler)
{
	return std::make_shared<UsageEnvironment>(std::move(threadpool), std::move(scheduler));
}

UsageEnvironment::~UsageEnvironment()
{
	LOG_CORE_INFO("~UsageEnvironment()");
}