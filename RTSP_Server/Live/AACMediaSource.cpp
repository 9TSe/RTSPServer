#include "AACMediaSource.h"
#include <string.h>
#include "../Scheduler/Log.h"

//std::shared_ptr<AACMediaSource> AACMediaSource::createNew(EnvPtr env, const std::string& file)
AACMediaSource* AACMediaSource::createNew(EnvPtr env, const std::string& file)
{
	//return std::make_shared<AACMediaSource>(env, file);
	return new AACMediaSource(env, file);
}

AACMediaSource::AACMediaSource(EnvPtr env, const std::string& file)
	:MediaSource(env)
{
	m_sourcename = file;
	m_istream.open(file, std::ios::in | std::ios::binary);
	setFps(43);
	for (int i = 0; i < FRAME_DEFAULT_NUM; ++i)
		m_env->threadPool()->addTask(m_task);
}

AACMediaSource::~AACMediaSource()
{
	LOG_CORE_INFO("~AACMediaSource()");
	m_istream.close();
}

bool AACMediaSource::parseAdtsHeader(uint8_t* in, AdtsHeader* adtsheader)
{
	memset(adtsheader, 0, sizeof(*adtsheader));
	if ((in[0] == 0xff) && ((in[1] & 0xf0) == 0xf0))
	{
		adtsheader->id = (in[1] & 0x08) >> 3;
		adtsheader->layer = (in[1] & 0x06) >> 1;
		adtsheader->protection_absent = (in[1] & 0x01);
		adtsheader->profile = (in[2] & 0xc0) >> 6;
		adtsheader->sampling_frequency_index = (in[2] & 0x3c) >> 2;
		adtsheader->private_bit = (in[2] & 0x02) >> 1;
		adtsheader->channel_configuration = ((in[2] & 0x01) << 2) | ((in[3] & 0xc0) >> 6);
		adtsheader->orininal_copy = (in[3] & 0x20) >> 5;
		adtsheader->home = (in[3] & 0x10) >> 4;
		adtsheader->copyrigth_identification_bit = (in[3] & 0x08) >> 3;
		adtsheader->copyrigth_identification_stat = (in[3] & 0x04) >> 2;
		adtsheader->aac_frame_length = ((static_cast<unsigned int>(in[3]) & 0x03) << 11) | 
										((static_cast<unsigned int>(in[4]) & 0xff) << 3) | 
										((static_cast<unsigned int>(in[5]) & 0xe0) >> 5);
		adtsheader->adts_bufferfullness = ((static_cast<unsigned int>(in[5]) & 0x1f) << 6) |
											((static_cast<unsigned int>(in[6]) & 0xfc) >> 2);
		adtsheader->number_of_raw_data_blocks_in_frame = (in[6] & 0x03);
		return true;
	}
	else
	{
		LOG_CORE_ERROR("fail parse adtsheader, synword != 0xfff");
		return false;
	}
}

int AACMediaSource::getFrameFromAACFile(uint8_t* buf, int size)
{
	if (!m_istream.is_open()) return -1;
	uint8_t tmpbuf[7] = { 0 };
	m_istream.read(reinterpret_cast<char*>(tmpbuf), 7);
	int headerRead = m_istream.gcount();
	if (headerRead <= 0)
	{
		m_istream.seekg(0, std::ios::beg);
		m_istream.read(reinterpret_cast<char*>(tmpbuf), 7);
		if (m_istream.gcount() <= 0) return -1;
	}
	if (!parseAdtsHeader(tmpbuf, &m_adtsHeader)) return -1;
	if (m_adtsHeader.aac_frame_length > size)
	{
		LOG_CORE_ERROR("aac frame length > size");
		return -1;
	}

	memcpy(buf, tmpbuf, 7);
	m_istream.read(reinterpret_cast<char*>(buf + 7), m_adtsHeader.aac_frame_length - 7);
	if (m_istream.gcount() < 0)
	{
		LOG_CORE_ERROR("read frame error");
		return -1;
	}
	return m_adtsHeader.aac_frame_length;
}

void AACMediaSource::handleTask()
{
	std::lock_guard<std::mutex> mutex(m_mutex);
	if (m_inqueueFrame.empty()) return;
	MediaFrame* frame = m_inqueueFrame.front();
	frame->m_size = getFrameFromAACFile(frame->m_temp, FRAME_MAX_SIZE);
	if (frame->m_size < 0) return;
	frame->m_buf = frame->m_temp;
	m_inqueueFrame.pop();
	m_outqueueFrame.push(frame);
}