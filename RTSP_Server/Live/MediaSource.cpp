#include "MediaSource.h"
#include "../Scheduler/Log.h"

MediaSource::MediaSource(EnvPtr env)
	:m_env(env)
	,m_fps(0)
{
	for (int i = 0; i < FRAME_DEFAULT_NUM; ++i)
		m_inqueueFrame.push(&m_frames[i]);
	m_task.setTask(taskCallback, this);
}

MediaSource::~MediaSource()
{
	LOG_CORE_INFO("~MediaSource()");
}

MediaFrame* MediaSource::getFrameFromQueue()
{
	std::lock_guard<std::mutex> mutex(m_mutex);
	if (m_outqueueFrame.empty())
		return nullptr;
	MediaFrame* frame = m_outqueueFrame.front();
	m_outqueueFrame.pop();
	return frame;
}

void MediaSource::putFrameToQueue(MediaFrame* frame)
{
	std::lock_guard<std::mutex> mutex(m_mutex);
	m_inqueueFrame.push(frame);
	m_env->threadPool()->addTask(m_task);
}

void MediaSource::taskCallback(void* arg)
{
	MediaSource* source = (MediaSource*)arg;
	source->handleTask();
}