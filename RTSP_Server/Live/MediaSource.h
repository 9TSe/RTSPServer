#pragma once
#include "../Scheduler/ThreadPool.h"
#include "../Scheduler/UsageEnvironment.h"
#include <stdint.h>
#include <mutex>
#include <queue>

constexpr int FRAME_DEFAULT_NUM = 4;
constexpr int FRAME_MAX_SIZE = 600 * 1024;


class MediaFrame
{
public:
	MediaFrame()
		:m_temp(new uint8_t[FRAME_MAX_SIZE])
		,m_buf(nullptr)
		,m_size(0)
	{}
	~MediaFrame()
	{
		delete[] m_temp;
	}
 
	uint8_t* m_temp;
	uint8_t* m_buf;
	int m_size;
};

class MediaSource
{
public:
	using EnvPtr = std::shared_ptr<UsageEnvironment>;
	explicit MediaSource(EnvPtr env);
	virtual ~MediaSource();

	int getFps() { return m_fps; }
	MediaFrame* getFrameFromQueue();
	void putFrameToQueue(MediaFrame* frame);

protected:
	virtual void handleTask() = 0;
	void setFps(int fps) { m_fps = fps; }

private:
	static void taskCallback(void* arg);

protected:
	EnvPtr m_env;
	std::mutex m_mutex;
	ThreadPool::Task m_task;
	MediaFrame m_frames[FRAME_DEFAULT_NUM];
	std::queue<MediaFrame*> m_inqueueFrame;
	std::queue<MediaFrame*> m_outqueueFrame;
	int m_fps;
	std::string m_sourcename;
};