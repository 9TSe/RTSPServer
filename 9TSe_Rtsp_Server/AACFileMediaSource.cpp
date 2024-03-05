#include <string.h>
#include "AACFileMediaSource.h"
#include "Log.h"

AACFileMediaSource::AACFileMediaSource(UsageEnvironment* env, const std::string& file)
	:MediaSource(env)
{
	m_sourceName = file;
	m_file = fopen(file.c_str(), "rb");
	Set_Fps(43);
	for (int i = 0; i < DEFAULT_FRAME_NUM; ++i)
		m_env->Thread_Pool()->Add_Task(m_task);
}

AACFileMediaSource::~AACFileMediaSource()
{
	fclose(m_file);
}

AACFileMediaSource* AACFileMediaSource::Create_New(UsageEnvironment* env, const std::string& file)
{
	return new AACFileMediaSource(env, file);
}

void AACFileMediaSource::Handle_Task()
{
	std::lock_guard<std::mutex> lck(m_mutex);

	if (m_frameInput_Queue.empty())
		return;
	MediaFrame* frame = m_frameInput_Queue.front();
	frame->m_size = GetFrame_From_AACFile(frame->m_temp, FRAME_MAX_SIZE);
	if (frame->m_size < 0)
		return;
	frame->m_buf = frame->m_temp;
	m_frameInput_Queue.pop();
	m_frameOutput_Queue.push(frame);
}

int AACFileMediaSource::GetFrame_From_AACFile(uint8_t* buf, int size)
{
	if (!m_file)
		return -1;
	uint8_t tmpbuf[7];
	int ret = fread(tmpbuf, 1, 7, m_file);
	if (ret <= 0)
	{
		fseek(m_file, 0, SEEK_SET);
		ret = fread(tmpbuf, 1, 7, m_file);
		if (ret <= 0)
			return -1;
	}

	if (!Parse_AdtsHeader(tmpbuf, &m_adtsHeader))
		return -1;
	if (m_adtsHeader.aacFrameLength > size)
		return -1;

	memcpy(buf, tmpbuf, 7);
	ret = fread(buf + 7, 1, m_adtsHeader.aacFrameLength - 7, m_file);
	if (ret < 0)
	{
		LOGE("read error");
		return -1;
	}
	return m_adtsHeader.aacFrameLength;
}

//强制类型转换 > '>>' > '&'
bool AACFileMediaSource::Parse_AdtsHeader(uint8_t* in, AdtsHeader* res)
{
	memset(res, 0, sizeof(*res));
	if ((in[0] == 0xFF) && ((in[1] & 0xF0) == 0xF0))
	{
		res->id = ((unsigned int)in[1] & 0x08) >> 3; //1111 1000
		res->layer = ((unsigned int)in[1] & 0x06) >> 1;
		res->protectionAbsent = (unsigned int)in[1] & 0x01;
		res->profile = ((unsigned int)in[2] & 0xc0) >> 6; //1100
		res->samplingFreqIndex = ((unsigned int)in[2] & 0x3c) >> 2;
		res->privateBit = ((unsigned int)in[2] & 0x02) >> 1;
		res->channelCfg = ((((unsigned int)in[2] & 0x01) << 2) | (((unsigned int)in[3] & 0xc0) >> 6));
		res->originalCopy = ((unsigned int)in[3] & 0x20) >> 5;
		res->home = ((unsigned int)in[3] & 0x10) >> 4;
		res->copyrightIdentificationBit = ((unsigned int)in[3] & 0x08) >> 3;
		res->copyrightIdentificationStart = (unsigned int)in[3] & 0x04 >> 2; //0100
		res->aacFrameLength = (((((unsigned int)in[3]) & 0x03) << 11) |
								(((unsigned int)in[4] & 0xFF) << 3) |
								((unsigned int)in[5] & 0xE0) >> 5);
		res->adtsBufferFullness = (((unsigned int)in[5] & 0x1f) << 6 |
									((unsigned int)in[6] & 0xfc) >> 2);
		res->numberOfRawDataBlockInFrame = ((unsigned int)in[6] & 0x03);
		return true;
	}
	else
	{
		LOGE("failed to parse adts_header");
		return false;
	}
}