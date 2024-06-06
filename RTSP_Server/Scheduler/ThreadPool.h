#pragma once
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>

class Thread
{
public:
	~Thread();
	bool start(void* arg);
	bool Join();
	bool Detach();

protected:
	Thread();
	virtual void run(void* arg) = 0;

private:
	static void* threadRun(void* arg);

private:
	void* m_arg;
	std::thread m_thread;
	bool m_isdetach;
	bool m_isstart;
};


class ThreadPool
{
public:
	static ThreadPool* createNew(int num);
	explicit ThreadPool(int num);
	~ThreadPool();
	

	class Task
	{
	public:
		using TaskCallback = void(*)(void*);
		Task()
			:callback(nullptr)
			,arg(nullptr)
		{}
		void handle()
		{
			if (callback)
				callback(arg);
		}
		void setTask(TaskCallback cb, void* ag)
		{
			callback = cb;
			arg = ag;
		}
		void operator=(const Task* task)
		{
			callback = task->callback;
			arg = task->arg;
		}

	private:
		TaskCallback callback;
		void* arg;
	};

	void addTask(ThreadPool::Task& task);

private:
	class M_Thread : public Thread
	{
		virtual void run(void* arg);
	};

	void loop();
	void createThread();
	void cancleThread();

private:
	std::mutex m_mutex;
	std::condition_variable m_condition;
	std::queue<Task> m_taskqueue;
	std::vector<M_Thread> m_threads;
	bool m_quit;
};