#pragma once
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "Thread.h"

class ThreadPool
{
public:
	class Task
	{
	public:
		using TaskCallback = void(*)(void*);
		Task()
			:m_taskCallback(nullptr),
			m_arg(nullptr)
		{}

		void Set_TaskCallback(TaskCallback callback, void* arg)
		{
			m_taskCallback = callback;
			m_arg = arg;
		}

		void Handle()
		{
			if (m_taskCallback)
				m_taskCallback(m_arg);
		}

		bool operator=(const Task& task)
		{
			this->m_taskCallback = task.m_taskCallback;
			this->m_arg = task.m_arg;
		}

	private:
		TaskCallback m_taskCallback;
		void* m_arg;
	};

	explicit ThreadPool(int num);
	~ThreadPool();

	static ThreadPool* Create_New(int num);
	void Add_Task(Task& task);

private:
	void Loop();

	class M_Thread : public Thread
	{
	protected:
		virtual void run(void* arg);
	};

	void Create_Threads();
	void Cancel_Threads();

private:
	std::queue<Task> m_taskQueue;
	std::mutex m_mutex;
	std::condition_variable m_con;
	std::vector<M_Thread> m_threads;
	bool m_quit;
};