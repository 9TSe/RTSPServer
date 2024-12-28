#include "H264MediaSource.h"
#include "../Scheduler/Log.h"

static inline bool startcode3(uint8_t* buf);
static inline bool startcode4(uint8_t* buf);

//std::shared_ptr<H264MediaSource> H264MediaSource::createNew(EnvPtr env, const std::string& file)
H264MediaSource* H264MediaSource::createNew(EnvPtr env, const std::string& file)
{
	return new H264MediaSource(env, file);
	//return std::make_shared<H264MediaSource>(env, file);
}

H264MediaSource::H264MediaSource(EnvPtr env, const std::string& file)
	:MediaSource(env)
{
	m_sourcename = file;
	m_istream.open(file, std::ios::in | std::ios::binary);
	setFps(57);

	for (int i = 0; i < FRAME_DEFAULT_NUM; ++i)
		m_env->threadPool()->addTask(m_task);
}

H264MediaSource::~H264MediaSource()
{
	LOG_CORE_INFO("~H264MediaSource()");
	m_istream.close();
}

static inline bool startcode3(uint8_t* buf)
{
	if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1) return true;
	return false;
}

static inline bool startcode4(uint8_t* buf)
{
	if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1) return true;
	return false;
}
//00 00 00 01 09 10 00 00 00 01
static uint8_t* findNextstartcode(uint8_t* buf, int len)
{
	if (len < 3) return nullptr;
	for (int i = 0; i < len - 3; ++i) //len - 3 cut end
	{
		if (startcode3(buf) || startcode4(buf))
			return buf;	
		++buf;
	}
	if (startcode3(buf)) return buf; //check buf[len]
	return nullptr;
}

int H264MediaSource::getFrameFromH264(uint8_t* frame, int size)
{
	if (!m_istream.is_open()) return -1;
	m_istream.read(reinterpret_cast<char*>(frame), size);
	int ret = m_istream.gcount();

	if (!startcode3(frame) && !startcode4(frame))
	{
		m_istream.seekg(0, std::ios::beg);
		LOG_CORE_ERROR("read error, {} not startcode", m_sourcename.c_str());
		return -1;
	}

	uint8_t* nextstartcode = findNextstartcode(frame + 3, ret - 3); //ret - 3 -> front 00 00 01
	int framesize = 0;
	if (!nextstartcode)
	{
		m_istream.seekg(0, std::ios::beg); 
		LOG_CORE_ERROR("read error, {} not next startcode", m_sourcename.c_str());
		//framesize = ret;
		return -1;
	}
	else
	{
		framesize = nextstartcode - frame;
		m_istream.seekg(framesize - ret, std::ios::cur); //m_istream -> nextstartcode
	}
	return framesize;
}

void H264MediaSource::handleTask()
{
	std::lock_guard<std::mutex> mutex(m_mutex);
	if (m_inqueueFrame.empty()) return;
	MediaFrame* frame = m_inqueueFrame.front();
	int startcodenum = 0;
	while (true)
	{
		frame->m_size = getFrameFromH264(frame->m_temp, FRAME_MAX_SIZE);
		if (frame->m_size < 0) return;

		if (startcode3(frame->m_temp)) startcodenum = 3;
		else startcodenum = 4;

		frame->m_buf = frame->m_temp + startcodenum;
		frame->m_size -= startcodenum;

		uint8_t nalutype = frame->m_buf[0] & 0x1f;
		if (nalutype == 0x09) continue; //AUD
		else if (nalutype == 0x06 || nalutype == 0x07 || nalutype == 0x08) break; //sei sps pps
		else break;
	}
	m_inqueueFrame.pop();
	m_outqueueFrame.push(frame);
}