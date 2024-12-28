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




// #include <iostream>
// #include <thread>
// #include <mutex>
// #include <condition_variable>
// #include <future>
// #include <atomic>
// #include <queue>
// #include <vector>
// #include <syncstream>
// #include <functional>

// inline std::size_t default_thread_pool_size()noexcept {
//     std::size_t num_threads = std::thread::hardware_concurrency() * 2;
//     num_threads = num_threads == 0 ? 2 : num_threads;
//     return num_threads;
// }

// class ThreadPool {
// public:
//     using Task = std::packaged_task<void()>;

//     ThreadPool(const ThreadPool&) = delete;
//     ThreadPool& operator=(const ThreadPool&) = delete;

//     ThreadPool(std::size_t num_thread = default_thread_pool_size())
//         : stop_{ false }, num_threads_{ num_thread } {
//         start();
//     }

//     ~ThreadPool() {
//         stop();
//     }

//     void stop() {
//         stop_.store(true);
//         cv_.notify_all();
//         for (auto& thread : pool_) {
//             if (thread.joinable()) {
//                 thread.join();
//             }
//         }
//         pool_.clear();
//     }

//     template<typename F, typename... Args>
//     std::future<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> submit(F&& f, Args&&...args) 
// 	{
//         using RetType = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;
//         if (stop_.load()) {
//             throw std::runtime_error("ThreadPool is stopped");
//         }

//         auto task = std::make_shared<std::packaged_task<RetType()>>(
//             std::bind(std::forward<F>(f), std::forward<Args>(args)...));
//         std::future<RetType> ret = task->get_future();

//         {
//             std::lock_guard<std::mutex> lc{ mutex_ };
//             tasks_.emplace([task] {(*task)(); });
//         }
//         cv_.notify_one();
//         return ret;
//     }

//     void start() {
//         for (std::size_t i = 0; i < num_threads_; ++i) {
//             pool_.emplace_back([this] {
//                 while (!stop_) {
//                     Task task;
//                     {
//                         std::unique_lock<std::mutex> lc{ mutex_ };
//                         cv_.wait(lc, [this] {return stop_ || !tasks_.empty(); });
//                         if (tasks_.empty())
//                             return;
//                         task = std::move(tasks_.front());
//                         tasks_.pop();
//                     }
//                     task();
//                 }
//             });
//         }
//     }

// private:
//     std::mutex                mutex_;
//     std::condition_variable   cv_;
//     std::atomic<bool>         stop_;
//     std::atomic<std::size_t>  num_threads_;
//     std::queue<Task>          tasks_;
//     std::vector<std::thread>  pool_;
// };