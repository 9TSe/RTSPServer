#define AAC_FILE_NAME "D:/ffmpeg/learn/test.aac"
#pragma warning(disable:4996)
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fdk-aac/aacdecoder_lib.h"

//data载入buf中的有效信息(包括帧头), data_size 赋值为buf中framesize;
int Get_ADTSframe(unsigned char* buf, int buf_size, unsigned char* data, unsigned int* data_size)
{
	if (!buf || !data || !data_size)
		return -1;

	int framesize = 0;
	while (1)
	{
		if (buf_size < 7) //头信息
			return -1; 
		if (buf[0] == 0xff && ((buf[1] & 0xf0) == 0xf0)) //比较运算符优先级高于位运算符
		{
			//赋值运算符优先级最低
			//aac_frame_length (<<>> 优先级 高于 &)
			framesize |= ((buf[3] & 0x03) << 11);
			framesize |= buf[4] << 3;
			framesize |= ((buf[5] & 0xe0) >> 5);
			break;
		}
		--buf_size;
		++buf;
	}
	if (buf_size < framesize)
		return 1;
	memcpy(data, buf, framesize);
	*data_size = framesize;
	return 0;
}

int main()
{
	FILE* aac_file = fopen(AAC_FILE_NAME, "rb");
	if (!aac_file)
	{
		std::cout << "open aac fail\n";
		return -1;
	}
	HANDLE_AACDECODER aac_coder = aacDecoder_Open(TT_MP4_ADTS, 1);
	AAC_DECODER_ERROR aac_error;
	unsigned char* aac_frame = (unsigned char*)malloc(1024 * 5);
	unsigned char* aac_buf = (unsigned char*)malloc(1024 * 1024);
	unsigned int pcm_size = 8 * 1024 * sizeof(INT_PCM);
	unsigned char* pcm_data = (unsigned char*)malloc(pcm_size);
	
	int count = 0;
	int offset = 0;
	int aac_datasize = 0;
	unsigned int aac_framesize = 0;
	unsigned int valid = 0;

	while (!feof(aac_file)) //函数返回0 代表文件尚未读取结束
	{
		aac_datasize = fread(aac_buf + offset, 1, 1024 * 1024 - offset, aac_file);
		unsigned char* aac_data = aac_buf; //因为要得到framesize就要移动指针来找头
		while (1)
		{
			//其实buf和frame都是一样的
			int ret = Get_ADTSframe(aac_data, aac_datasize, aac_frame, &aac_framesize);
			if (ret == -1)
			{
				std::cout << "adts frame end\n";
				break;
			}
			else if (ret == 1) //ADTS中信息不完整
			{
				memcpy(aac_buf, aac_data, aac_datasize);
				offset = aac_datasize;
				std::cout << "adts frame broken\n";
			}
			valid = aac_framesize;
			aac_error = aacDecoder_Fill(aac_coder, &aac_frame, &aac_framesize, &valid);
			if (aac_error > 0)
				std::cout << "fill decoder\n";

			aac_error = aacDecoder_DecodeFrame(aac_coder, (INT_PCM*)pcm_data,
				pcm_size / sizeof(INT_PCM), 0);
			if (aac_error > 0)
				std::cout << "decode error\n";

			/*sampleRate: 解码后的PCM音频信号的采样率（单位：Hz）（经过SBR处理后的采样率）。
			frameSize : 解码后的PCM音频信号的帧大小。
				对于AAC - LC：1024或960。
				对于HE - AAC（v2）：2048或1920。
				对于AAC - LD和AAC - ELD：512或480。
			numChannels : 解码后的PCM音频信号中输出音频通道的数量。
			pChannelType : 每个输出音频通道的音频通道类型。
			pChannelIndices : 每个输出音频通道的音频通道索引。
			aacSampleRate : 不带SBR的采样率（来自配置信息）。
			profile : MPEG - 2配置文件（来自文件头）（ - 1：不适用（例如，MPEG - 4））。
			aot : 音频对象类型（来自ASC）：针对MPEG - 2比特流设置为适当的值（例如，AAC - LC为2）。
			channelConfig : 通道配置（0：PCE定义，1：单声道，2：立体声，...）。
			bitRate : 瞬时比特率。
			aacSamplesPerFrame : AAC核心的每帧样本数（来自ASC）。
				对于AAC - LC：1024或960。
				对于AAC - LD和AAC - ELD：512或480。
			aacNumChannels : 经过AAC核心处理后（在PS或MPS处理之前）的音频通道数量。注意：这不是最终的输出通道数量！
			extAot : 扩展音频对象类型（来自ASC）。
			extSamplingRate : 扩展采样率（来自ASC）。
			outputDelay : 解码器额外延迟的样本数。
			flags : 内部标志的副本。仅由解码器写入，外部只能读取。
			epConfig : epConfig级别（来自ASC）：只支持级别0， - 1表示没有ER（例如，AOT = 2，MPEG - 2 AAC等）。
			numLostAccessUnits : 如果 aacDecoder_DecodeFrame() 返回 AAC_DEC_TRANSPORT_SYNC_ERROR，则此整数将反映估计的丢失访问单元的数量。如果估算失败，则会小于0。
			numTotalBytes : 通过解码器传递的总字节数。
			numBadBytes : 从 numTotalBytes 中考虑为带有错误的总字节数。
			numTotalAccessUnits : 通过解码器传递的总访问单元数。
			numBadAccessUnits : 从 numTotalAccessUnits 中考虑为带有错误的总访问单元数。
			drcProgRefLev : DRC（动态范围控制）程序参考电平。定义低于满量程的参考级别。
			drcPresMode : DRC（动态范围控制）呈现模式。*/
			CStreamInfo* pcm_frame = aacDecoder_GetStreamInfo(aac_coder); //获取解码后的音频流的相关信息
			printf("pcmFrame: channels=%d,simmpleRate=%d,frameSize=%d\n",
				pcm_frame->numChannels, pcm_frame->sampleRate, pcm_frame->frameSize);

			//码流格式
			char profile_str[10] = { 0 };
			unsigned char profile = (aac_frame[2] & 0xC0) >> 6; //aac级别,profile
			switch (profile)
			{
			case 0: sprintf(profile_str, "Main"); break;
			case 1: sprintf(profile_str, "LC"); break;
			case 2: sprintf(profile_str, "SSR"); break;
			default: sprintf(profile_str, "unknown"); break;
			}

			//采样率
			char frequence_str[10] = { 0 };
			unsigned char sampling_frequency_index = (aac_frame[2] & 0x3C) >> 2;
			switch (sampling_frequency_index)
			{
			case 0: sprintf(frequence_str, "96000Hz"); break;
			case 1: sprintf(frequence_str, "88200Hz"); break;
			case 2: sprintf(frequence_str, "64000Hz"); break;
			case 3: sprintf(frequence_str, "48000Hz"); break;
			case 4: sprintf(frequence_str, "44100Hz"); break;
			case 5: sprintf(frequence_str, "32000Hz"); break;
			case 6: sprintf(frequence_str, "24000Hz"); break;
			case 7: sprintf(frequence_str, "22050Hz"); break;
			case 8: sprintf(frequence_str, "16000Hz"); break;
			case 9: sprintf(frequence_str, "12000Hz"); break;
			case 10: sprintf(frequence_str, "11025Hz"); break;
			case 11: sprintf(frequence_str, "8000Hz"); break;
			default:sprintf(frequence_str, "unknown"); break;
			}

			printf("%5d| %8s|  %8s| %5d| %5d |\n",
				count, profile_str, frequence_str, aac_framesize, pcm_size);
			aac_datasize -= aac_framesize;
			aac_data += aac_framesize;
			++count;
		}
		std::cout << "---------------------------------------------\n";
	}
	free(pcm_data);
	free(aac_buf);
	free(aac_frame);
	fclose(aac_file);
	return 0;
}