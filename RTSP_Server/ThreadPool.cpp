#include "ThreadPool.h"
#include "Log.h"

ThreadPool::ThreadPool(int num)
	:m_threads(num),
	m_quit(false)
{
	Create_Threads();
}

ThreadPool::~ThreadPool()
{
	Cancel_Threads();
}

ThreadPool* ThreadPool::Create_New(int num)
{
	return new ThreadPool(num);
}

void ThreadPool::Create_Threads()
{
	std::unique_lock<std::mutex> lck(m_mutex);
	for (auto& thread : m_threads)
		thread.start(this);
}

void ThreadPool::Cancel_Threads()
{
	std::unique_lock<std::mutex> lck(m_mutex);
	m_quit = true;
	m_con.notify_all();
	for (auto& thread : m_threads)
		thread.join();
	m_threads.clear();
}

void ThreadPool::M_Thread::run(void* arg)
{
	ThreadPool* threadpool = (ThreadPool*)arg;
	threadpool->Loop();
}

void ThreadPool::Loop()
{
	while (!m_quit)
	{
		std::unique_lock<std::mutex> lck(m_mutex);
		if (m_taskQueue.empty())
			m_con.wait(lck);

		if (m_taskQueue.empty())
			continue;

		Task task = m_taskQueue.front();
		m_taskQueue.pop();
		task.Handle();
	}
}

void ThreadPool::Add_Task(ThreadPool::Task& task)
{
	std::unique_lock<std::mutex> lck(m_mutex);
	m_taskQueue.push(task);
	m_con.notify_all();
}