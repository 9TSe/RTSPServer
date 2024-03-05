#include "MediaSource.h"
#include "Log.h"

MediaSource::MediaSource(UsageEnvironment* env)
	:m_env(env),
	m_fps(0)
{
	for (int i = 0; i < DEFAULT_FRAME_NUM; ++i)
		m_frameInput_Queue.push(&m_frames[i]);
	m_task.Set_TaskCallback(Task_Callback, this);
}

MediaSource::~MediaSource()
{
	LOGI("~MediaSource()");
}

MediaFrame* MediaSource::GetFrame_From_OutputQueue()
{
	std::lock_guard<std::mutex> lck(m_mutex);
	if (m_frameOutput_Queue.empty())
		return nullptr;

	MediaFrame* frame = m_frameOutput_Queue.front();
	m_frameOutput_Queue.pop();
	return frame;
}

void MediaSource::PutFrame_To_InputQueue(MediaFrame* frame)
{
	std::lock_guard<std::mutex> lock(m_mutex);
	m_frameInput_Queue.push(frame);
	m_env->Thread_Pool()->Add_Task(m_task);
}

void MediaSource::Task_Callback(void* arg)
{
	MediaSource* source = (MediaSource*)arg;
	source->Handle_Task();
}