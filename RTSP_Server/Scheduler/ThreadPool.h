#pragma once
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <vector>
#include <functional>
#include <atomic>

#include <memory>

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


class ThreadPool //: public std::enable_shared_from_this<ThreadPool>
{
public:
	static std::shared_ptr<ThreadPool> createNew(int num);
	explicit ThreadPool(int num);
	~ThreadPool();

	class Task
	{
	public:
		using TaskCallback = std::function<void(void*)>;
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
	std::atomic_bool m_quit;
};