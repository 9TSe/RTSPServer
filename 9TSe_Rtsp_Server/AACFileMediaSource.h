#pragma once
#include <string>
#include "MediaSource.h"

class AACFileMediaSource : public MediaSource
{
public:
	AACFileMediaSource(UsageEnvironment* env, const std::string& file);
	virtual ~AACFileMediaSource();
	static AACFileMediaSource* Create_New(UsageEnvironment* env, const std::string& file);

protected:
	virtual void Handle_Task();



private:
	struct AdtsHeader
	{
		unsigned int syncword;			//12bit		'1111 1111 1111',show one ADTS-frame begin
		unsigned int id;				//1bit		0 for MPEG-4, 1 for MPEG-2
		unsigned int layer;				//2bit		always'00'
		unsigned int protectionAbsent;  //1bit		1 show not crc, 0 show crc
		unsigned int profile;			//1bit		show the level of AAC
		unsigned int samplingFreqIndex; //4bit		show Sampling frequency(采样
		unsigned int privateBit;		//1bit
		unsigned int channelCfg;		//3bit		show Track number(声道
		unsigned int originalCopy;		//1bit
		unsigned int home;				//1bit

		unsigned int copyrightIdentificationBit;	//1bit
		unsigned int copyrightIdentificationStart;	//1bit
		unsigned int aacFrameLength;				//13bit		a ADTS-frame's length which includes ADTS headers and AAC raw streams
		unsigned int adtsBufferFullness;			//11bit		0x7FF show Bit-rate variable stream

		
		//show ADTS-frame have 'number_of_raw_data_blocks_in_frame + 1' AAC Raw frame
		//so number_of_raw_data_blocks_in_frame == 0
		//表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
		unsigned int numberOfRawDataBlockInFrame; //2 bit
	};

	bool Parse_AdtsHeader(uint8_t* in, AdtsHeader* res);
	int GetFrame_From_AACFile(uint8_t* buf, int size);

private:
	FILE* m_file;
	AdtsHeader m_adtsHeader;
};