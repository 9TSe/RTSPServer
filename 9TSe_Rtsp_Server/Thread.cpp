#include "Thread.h"

Thread::Thread()
	:m_arg(nullptr),
	m_isStart(false),
	m_isDetach(false)
{}

Thread::~Thread()
{
	if (m_isStart == true && m_isDetach == false)
		detach();
}

bool Thread::start(void* arg)
{
	m_arg = arg;
	m_threadId = std::thread(&Thread::Thread_Run, this);
	m_isStart = true;
	return true;
}

bool Thread::detach()
{
	if (m_isStart = false)
		return false;
	if (m_isDetach == true)
		return true;

	m_threadId.detach();
	m_isDetach = true;
	return true;
}

bool Thread::join()
{
	if (m_isStart == false || m_isDetach == true)
		return false;

	m_threadId.join();
	return true;
}

void* Thread::Thread_Run(void* arg)
{
	Thread* thread = (Thread*)arg;
	thread->run(thread->m_arg);
	return nullptr;
}