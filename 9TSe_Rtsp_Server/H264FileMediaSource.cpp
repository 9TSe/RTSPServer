#include "H264FileMediaSource.h"
#include "Log.h"
#include <fcntl.h>

static inline int Start_Code3(uint8_t* buf);
static inline int Start_Code4(uint8_t* buf);

H264FileMediaSource::H264FileMediaSource(UsageEnvironment* env, const std::string& file)
	:MediaSource(env)
{
	m_sourceName = file;
	m_file = fopen(file.c_str(), "rb");
	Set_Fps(25);

	for (int i = 0; i < DEFAULT_FRAME_NUM; ++i)
		m_env->Thread_Pool()->Add_Task(m_task);
}

H264FileMediaSource::~H264FileMediaSource()
{
	fclose(m_file);
}

H264FileMediaSource* H264FileMediaSource::Create_New(UsageEnvironment* env, const std::string& file)
{
	return new H264FileMediaSource(env, file);
}

void H264FileMediaSource::Handle_Task()
{
	std::lock_guard<std::mutex> lck(m_mutex);
	if (m_frameInput_Queue.empty())
		return;
	MediaFrame* frame = m_frameInput_Queue.front();
	int startcode_num = 0;
	while (true)
	{
		frame->m_size = GetFrame_From_H264File(frame->m_temp, FRAME_MAX_SIZE);
		if (frame->m_size < 0)
			return;
		if (Start_Code3(frame->m_temp))
			startcode_num = 3;
		else
			startcode_num = 4;
		frame->m_buf = frame->m_temp + startcode_num;
		frame->m_size -= startcode_num;

		uint8_t nalutype = frame->m_buf[0] & 0x1f;

		if (nalutype == 0x09)
			continue;
		else if (nalutype == 0x07 || nalutype == 0x08)
			break;
		else
			break;
	}
	m_frameInput_Queue.pop();
	m_frameOutput_Queue.push(frame);
}

static inline int Start_Code3(uint8_t* buf)
{
	if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
		return 1;
	else
		return 0;
}

static inline int Start_Code4(uint8_t* buf)
{
	if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1)
		return 1;
	else
		return 0;
}

static uint8_t* FindNext_StartCode(uint8_t* buf, int len)
{
	if (len < 3)
		return nullptr;
	for (int i = 0; i < len - 3; ++i)
	{
		if (Start_Code3(buf) || Start_Code4(buf))
			return buf;
		++buf;
	}
	if (Start_Code3(buf))
		return buf;
	return nullptr;
}

int H264FileMediaSource::GetFrame_From_H264File(uint8_t* frame, int size)
{
	if (!m_file)
		return -1;

	int framesize;
	uint8_t* nextstartcode;
	int ret = fread(frame, 1, size, m_file);
	if (!Start_Code3(frame) && !Start_Code4(frame))
	{
		fseek(m_file, 0, SEEK_SET);
		LOGE("read %s error, no startcode", m_sourceName.c_str());
		return -1;
	}

	nextstartcode = FindNext_StartCode(frame + 3, ret - 3);
	if (!nextstartcode)
	{
		fseek(m_file, 0, SEEK_SET);
		framesize = ret;
		LOGE("read %s error, no nextstartcode, ret = %d", m_sourceName.c_str(), ret);
	}
	else
	{
		framesize = (nextstartcode - frame);
		fseek(m_file, framesize - ret, SEEK_CUR);
	}
	return framesize;
}