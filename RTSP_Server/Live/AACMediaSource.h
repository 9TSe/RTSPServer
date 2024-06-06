#pragma once
#include <fstream>
#include "MediaSource.h"

class AACMediaSource : public MediaSource
{
public:
	static AACMediaSource* createNew(UsageEnvironment* env, const std::string& file);
	AACMediaSource(UsageEnvironment* env, const std::string& file);
	virtual ~AACMediaSource();

protected:
	virtual void handleTask();

private:
	struct AdtsHeader
	{
		uint16_t synword ; //12bit
		uint8_t id; // 1
		uint8_t layer; // 2
		uint8_t protection_absent; //1
		uint8_t profile; //2
		uint8_t sampling_frequency_index; //4
		uint8_t private_bit; //1
		uint8_t channel_configuration; //3
		uint8_t orininal_copy; //1
		uint8_t home; //1
		uint8_t copyrigth_identification_bit; //1
		uint8_t copyrigth_identification_stat; //1
		uint16_t aac_frame_length; //13
		uint16_t adts_bufferfullness; //11
		uint8_t number_of_raw_data_blocks_in_frame; //2
	};

	bool parseAdtsHeader(uint8_t* in, AdtsHeader* adtsheader);
	int getFrameFromAACFile(uint8_t* buf, int size);

private:
	std::ifstream m_istream;
	AdtsHeader m_adtsHeader;
};