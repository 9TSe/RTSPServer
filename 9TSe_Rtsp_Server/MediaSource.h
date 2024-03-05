#pragma once
#include <queue>
#include <mutex>
#include <stdint.h>
#include "UsageEnvironment.h"
#include "ThreadPool.h"

#define FRAME_MAX_SIZE (1024*200)
#define DEFAULT_FRAME_NUM 4

class MediaFrame
{
public:
	MediaFrame()
		:m_temp(new uint8_t[FRAME_MAX_SIZE]),
		m_buf(nullptr),
		m_size(0)
	{}

	~MediaFrame()
	{
		delete []m_temp;
	}

//private:

	uint8_t* m_temp; //continuer
	uint8_t* m_buf;	 //quote continuer
	int m_size;
};

class MediaSource
{
public:
	explicit MediaSource(UsageEnvironment* env);
	virtual ~MediaSource();

	MediaFrame* GetFrame_From_OutputQueue();
	void PutFrame_To_InputQueue(MediaFrame* frame);
	int Get_Fps() const { return m_fps; }
	std::string Get_Sourcename() { return m_sourceName; }

protected:
	virtual void Handle_Task() = 0;
	void Set_Fps(int fps) { m_fps = fps; }

private:
	static void Task_Callback(void* arg);

protected:
	UsageEnvironment* m_env;
	MediaFrame m_frames[DEFAULT_FRAME_NUM];
	std::queue<MediaFrame*> m_frameInput_Queue;
	std::queue<MediaFrame*> m_frameOutput_Queue;

	std::mutex m_mutex;
	ThreadPool::Task m_task;
	int m_fps;
	std::string m_sourceName;
};