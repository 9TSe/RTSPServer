#include "ThreadPool.h"
#include "Log.h"

Thread::Thread()
	:m_arg(nullptr)
	,m_isdetach(false)
	,m_isstart(false)
{}

Thread::~Thread()
{
	if (m_isstart && !m_isdetach)
		Detach();
}

bool Thread::Join()
{
	if (m_isdetach || !m_isstart)
		return false;
	m_thread.join();
	return true;
}

bool Thread::start(void* arg)
{
	m_arg = arg;
	m_thread = std::thread(&Thread::threadRun, this);
	m_isstart = true;
	return true;
}

bool Thread::Detach()
{
	if (!m_isstart)
		return false;
	if (m_isdetach)
		return true;
	m_thread.detach();
	m_isdetach = true;
	return true;
}

void* Thread::threadRun(void* arg)
{
	Thread* thread = (Thread*)arg;
	thread->run(thread->m_arg);
	return nullptr;
}



std::shared_ptr<ThreadPool> ThreadPool::createNew(int num)
{
	return std::make_shared<ThreadPool>(num);
}

ThreadPool::ThreadPool(int num)
	:m_threads(num)
	, m_quit(false)
{
	createThread();
}

ThreadPool::~ThreadPool()
{
	LOGI("~ThreadPool()");
	cancleThread();
}

void ThreadPool::createThread()
{
	std::unique_lock<std::mutex> mutex(m_mutex);
	for (auto& thread : m_threads)
		thread.start(this);
}

void ThreadPool::cancleThread()
{
	std::unique_lock<std::mutex> mutex(m_mutex);
	m_quit = true;
	m_condition.notify_all();
	for (auto& thread : m_threads)
		thread.Join();
	m_threads.clear();
}

void ThreadPool::M_Thread::run(void* arg)
{
	ThreadPool* threadpool = (ThreadPool*)arg;
	threadpool->loop();
}

void ThreadPool::loop()
{
	while (!m_quit)
	{
		std::unique_lock<std::mutex> mutex(m_mutex);
		if (m_taskqueue.empty())
			m_condition.wait(mutex);
		if (m_taskqueue.empty()) //prevent other thread together wait
			continue; 

		Task task = m_taskqueue.front();
		m_taskqueue.pop();
		task.handle();
	}
}

void ThreadPool::addTask(ThreadPool::Task& task)
{
	std::unique_lock<std::mutex> mutex(m_mutex);
	m_taskqueue.push(task);
	m_condition.notify_all();
}