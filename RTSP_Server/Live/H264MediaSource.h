#pragma once
#include "MediaSource.h"
#include <fstream>

class H264MediaSource : public MediaSource
{
public:
	H264MediaSource(UsageEnvironment* env, const std::string& file);
	virtual ~H264MediaSource();
	static H264MediaSource* createNew(UsageEnvironment* env, const std::string& file);

protected:
	virtual void handleTask();

private:
	int getFrameFromH264(uint8_t* frame, int size);

private:
	std::ifstream m_istream;
};