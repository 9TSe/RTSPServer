#define AAC_FILE_NAME "D:/ffmpeg/learn/test.aac"
#pragma warning(disable:4996)
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fdk-aac/aacdecoder_lib.h"

//data����buf�е���Ч��Ϣ(����֡ͷ), data_size ��ֵΪbuf��framesize;
int Get_ADTSframe(unsigned char* buf, int buf_size, unsigned char* data, unsigned int* data_size)
{
	if (!buf || !data || !data_size)
		return -1;

	int framesize = 0;
	while (1)
	{
		if (buf_size < 7) //ͷ��Ϣ
			return -1; 
		if (buf[0] == 0xff && ((buf[1] & 0xf0) == 0xf0)) //�Ƚ���������ȼ�����λ�����
		{
			//��ֵ��������ȼ����
			//aac_frame_length (<<>> ���ȼ� ���� &)
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

	while (!feof(aac_file)) //��������0 �����ļ���δ��ȡ����
	{
		aac_datasize = fread(aac_buf + offset, 1, 1024 * 1024 - offset, aac_file);
		unsigned char* aac_data = aac_buf; //��ΪҪ�õ�framesize��Ҫ�ƶ�ָ������ͷ
		while (1)
		{
			//��ʵbuf��frame����һ����
			int ret = Get_ADTSframe(aac_data, aac_datasize, aac_frame, &aac_framesize);
			if (ret == -1)
			{
				std::cout << "adts frame end\n";
				break;
			}
			else if (ret == 1) //ADTS����Ϣ������
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

			/*sampleRate: ������PCM��Ƶ�źŵĲ����ʣ���λ��Hz��������SBR�����Ĳ����ʣ���
			frameSize : ������PCM��Ƶ�źŵ�֡��С��
				����AAC - LC��1024��960��
				����HE - AAC��v2����2048��1920��
				����AAC - LD��AAC - ELD��512��480��
			numChannels : ������PCM��Ƶ�ź��������Ƶͨ����������
			pChannelType : ÿ�������Ƶͨ������Ƶͨ�����͡�
			pChannelIndices : ÿ�������Ƶͨ������Ƶͨ��������
			aacSampleRate : ����SBR�Ĳ����ʣ�����������Ϣ����
			profile : MPEG - 2�����ļ��������ļ�ͷ���� - 1�������ã����磬MPEG - 4������
			aot : ��Ƶ�������ͣ�����ASC�������MPEG - 2����������Ϊ�ʵ���ֵ�����磬AAC - LCΪ2����
			channelConfig : ͨ�����ã�0��PCE���壬1����������2����������...����
			bitRate : ˲ʱ�����ʡ�
			aacSamplesPerFrame : AAC���ĵ�ÿ֡������������ASC����
				����AAC - LC��1024��960��
				����AAC - LD��AAC - ELD��512��480��
			aacNumChannels : ����AAC���Ĵ������PS��MPS����֮ǰ������Ƶͨ��������ע�⣺�ⲻ�����յ����ͨ��������
			extAot : ��չ��Ƶ�������ͣ�����ASC����
			extSamplingRate : ��չ�����ʣ�����ASC����
			outputDelay : �����������ӳٵ���������
			flags : �ڲ���־�ĸ��������ɽ�����д�룬�ⲿֻ�ܶ�ȡ��
			epConfig : epConfig��������ASC����ֻ֧�ּ���0�� - 1��ʾû��ER�����磬AOT = 2��MPEG - 2 AAC�ȣ���
			numLostAccessUnits : ��� aacDecoder_DecodeFrame() ���� AAC_DEC_TRANSPORT_SYNC_ERROR�������������ӳ���ƵĶ�ʧ���ʵ�Ԫ���������������ʧ�ܣ����С��0��
			numTotalBytes : ͨ�����������ݵ����ֽ�����
			numBadBytes : �� numTotalBytes �п���Ϊ���д�������ֽ�����
			numTotalAccessUnits : ͨ�����������ݵ��ܷ��ʵ�Ԫ����
			numBadAccessUnits : �� numTotalAccessUnits �п���Ϊ���д�����ܷ��ʵ�Ԫ����
			drcProgRefLev : DRC����̬��Χ���ƣ�����ο���ƽ��������������̵Ĳο�����
			drcPresMode : DRC����̬��Χ���ƣ�����ģʽ��*/
			CStreamInfo* pcm_frame = aacDecoder_GetStreamInfo(aac_coder); //��ȡ��������Ƶ���������Ϣ
			printf("pcmFrame: channels=%d,simmpleRate=%d,frameSize=%d\n",
				pcm_frame->numChannels, pcm_frame->sampleRate, pcm_frame->frameSize);

			//������ʽ
			char profile_str[10] = { 0 };
			unsigned char profile = (aac_frame[2] & 0xC0) >> 6; //aac����,profile
			switch (profile)
			{
			case 0: sprintf(profile_str, "Main"); break;
			case 1: sprintf(profile_str, "LC"); break;
			case 2: sprintf(profile_str, "SSR"); break;
			default: sprintf(profile_str, "unknown"); break;
			}

			//������
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