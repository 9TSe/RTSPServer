#pragma once
#include <string>
#include "MediaSource.h"

class H264FileMediaSource : public MediaSource
{
public:
	H264FileMediaSource(UsageEnvironment* env, const std::string& file);
	virtual ~H264FileMediaSource();
	static H264FileMediaSource* Create_New(UsageEnvironment* env, const std::string& file);

protected:
	virtual void Handle_Task();
private:
	int GetFrame_From_H264File(uint8_t* frame, int size);

private:
	FILE* m_file;
};