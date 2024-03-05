#pragma once
#include <thread>

class Thread
{
public:
	virtual ~Thread();
	bool start(void* arg);
	bool detach();
	bool join();

protected:
	Thread();
	virtual void run(void* arg) = 0;

private:
	static void* Thread_Run(void*);

private:
	void* m_arg;
	bool m_isStart;
	bool m_isDetach;
	std::thread m_threadId;
};