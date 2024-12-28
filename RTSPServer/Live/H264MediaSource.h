#pragma once
#include "MediaSource.h"
#include <fstream>

class H264MediaSource : public MediaSource
{
public:
	using HSPtr = std::shared_ptr<H264MediaSource>;
	H264MediaSource(EnvPtr env, const std::string& file);
	virtual ~H264MediaSource();
	//static HSPtr createNew(EnvPtr env, const std::string& file);
	static H264MediaSource* createNew(EnvPtr env, const std::string& file);

protected:
	virtual void handleTask();

private:
	int getFrameFromH264(uint8_t* frame, int size);

private:
	std::ifstream m_istream;
};