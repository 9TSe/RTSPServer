#pragma once
#include <memory>
class ThreadPool;
class EventScheduler;


class UsageEnvironment //: public std::enable_shared_from_this<UsageEnvironment>
{
public:
	UsageEnvironment(std::unique_ptr<ThreadPool>&& threadpool, std::unique_ptr<EventScheduler>&& scheduler);
	~UsageEnvironment();

	static std::shared_ptr<UsageEnvironment> createNew(std::unique_ptr<ThreadPool>&& threadpool, std::unique_ptr<EventScheduler>&& scheduler);
	ThreadPool* threadPool() { return m_threadpool.get(); }
	EventScheduler* eventScheduler() { return m_eventscheduler.get(); }
private:
	std::unique_ptr<ThreadPool> m_threadpool;
	std::unique_ptr<EventScheduler> m_eventscheduler;
};