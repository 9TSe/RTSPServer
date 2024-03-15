//����aac��ʽ����Ƶ
#define SDL_MAIN_HANDLED //SDL ��Ҫ�ṩĬ�ϵ� main �������
#pragma warning( disable : 4996 )
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <SDL.h>
#include "Adts.h"
#include "fdk-aac/aacdecoder_lib.h"
#define AAC_FILE_NAME "D:/ffmpeg/learn/test.aac"

static Uint8* audio_chunk;
static Uint32 audio_len;
static Uint8* audio_pos;

//data : SDL_AudioSpec.userdata
//stream ���ָ���� SDL �ڲ���Ƶ�����ڴ��ָ�룬ֻҪ�����ݿ��������ָ��ĵ�ַ�����ܲ��������ˡ�
//len ��ʾ��Ƶ�������Ĵ�С
//SDL ����ƵӲ���豸��ʱ��SDL ��ͻᴴ��һ���̣߳�����ʱִ�лص����� sdl_audio_callback()
//���� SDL �̶߳�ûص�һ�κ���������Ҫ̫����
void Fill_Audio(void* data, Uint8* stream, int len)
{
    SDL_memset(stream, 0, len);
    if (audio_len == 0) return;
    len = len > audio_len ? audio_len : len; //ȡmin, audio_len��pcm_framesize
    //������ΧΪ0~128,����ΪSDL_MIX_MAXVOLUME��ʾ��������Ƶ����
    //��audio_pos��Ƶ ��� stream, len���ֽڼ�����С
    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    audio_pos += len; //������һ������֡�����
    audio_len -= len; //ʹ�������е�while�ȴ���ֹ,ͬʱҲ�����Լ��㹻ʱ��ִ�иĺ���
}

int main()
{
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) //��ʼ����Ƶ��ϵͳ | ��ʼ����ʱ����ϵͳ
	{
		std::cout << "init SDL fail - " << SDL_GetError() << std::endl;
		return -1;
	}
    //typedef struct SDL_AudioSpec 
    //{
    //    int freq;                   // ������ (samples per second)
    //    SDL_AudioFormat format;     // ��Ƶ��ʽ
    //    Uint8 channels;             // ������ (1 for mono, 2 for stereo)
    //    Uint8 silence;              // ����ֵ (ͨ��Ϊ 0)
    //    Uint16 samples;             // ��Ƶ��������С (��λ�ǲ�����)
    //    Uint16 padding;             // �����õ���� (ͨ��Ϊ 0)
    //    Uint32 size;                // ��Ƶ��������С (��λ���ֽ�)
    //    SDL_AudioCallback callback; // �ص�����ָ�룬���ڴ�����Ƶ����
    //    void* userdata;             // �û��Զ�������
    //};
    SDL_AudioSpec SDL_spec;
    SDL_spec.freq = 44100;
    SDL_spec.format = AUDIO_S16SYS; //16-bit signed integer ����Ƶ���ݸ�ʽ,ʹ������ϵͳ���ֽ�˳��
    SDL_spec.channels = 2;
    SDL_spec.silence = 0;
    SDL_spec.samples = 1024; //��Ƶ��������С(��λ�ǲ�����)
    SDL_spec.callback = Fill_Audio;

    if (SDL_OpenAudio(&SDL_spec, nullptr) < 0) //����Ƶ�豸������ SDL_spec ��Ҫ����г�ʼ����
    {
        std::cout << "open audio fail\n";
        return -1;
    }

    SDL_PauseAudio(0);//����SDL��Ƶ�豸����ͣ״̬������Ϊ0����Ƶ�豸�����ָ�������״̬����ʼ������Ƶ���ݣ�
                      //��������Ϊ 1 ʱ����Ƶ�豸������ͣ��ֹͣ������Ƶ���ݡ�

    FILE* aac_file = fopen(AAC_FILE_NAME, "rb");
    if (!aac_file)
    {
        std::cout << "open file fail\n";
        return -1;
    }

    HANDLE_AACDECODER aac_coder = aacDecoder_Open(TT_MP4_ADTS, 1); //aac������ʵ����
    uint8_t* frame = (uint8_t*)malloc(2000); //���aac����
    unsigned int pcm_framesize = 2 * 1024 * sizeof(INT_PCM); //signed short
    unsigned char* pcm_frame = (unsigned char*)malloc(pcm_framesize); //���pcm��ʽ������

    AdtsHeader adts_header;
    int adts_headerlen; //ADTSͷ��
    int adts_contentlen;//ADTS���س�
    unsigned int adts_len = 0; //ADTS��ǰ֡�ܳ�(ͷ+����)

    int count = 0; //countѭ������, ��֡
    AAC_DECODER_ERROR aac_error;

    while (1)
    {
        adts_headerlen = fread(frame, 1, 7, aac_file); //ͷ����Ŀǰ�Զ���7�ֽ�, ��ͷ
        if (adts_headerlen <= 0) //û�����ݻ����
        {
            std::cout << "read header error\n";
            break;
        }
        if (Parse_AdtsHeader(frame, &adts_header) < 0) //����ͷ���ݷ����ڰ���
        {
            std::cout << "parse header error\n";
            break;
        }

        //��ȡaacfile��ͷ����(һ��֡)������
        adts_contentlen = fread(frame + 7, 1, adts_header.aacFrameLength - 7, aac_file);
        if (adts_contentlen < 0)
        {
            std::cout << "read content error\n";
            break;
        }

        adts_len = adts_header.aacFrameLength;

        //AAC�������ǽ����õ�,��ʲô,��ͨ�������������ȥ
        //�ӻ�������(frame)
        //�ⲿ���뻺�����Ĵ�С(length)
        //������Ϣ(len) �Ա��������aacDecoder Fillʱ������ȷ��pBuffer�е���ȷλ���Ի�ȡ��һ�����ݡ�
        aac_error = aacDecoder_Fill(aac_coder, &frame, &adts_header.aacFrameLength, &adts_len);
        if (aac_error > 0)
        {
            std::cout << "decoder fill error\n";
            break;
        }

        //��ʼ��������, ������Ϊ pcm ��ʽ�����ݴ洢��pcm_frame��
        //���һλ�Ǳ�־λ :
        //0 �޸�����Ϣ
        //AACDEC_CONCEAL��ֵΪ 1������ʾ��Ҫ�������ݵ��ڸǣ�concealment������
        //AACDEC_FLUSH��ֵΪ 2������ʾ��Ҫ�����������ݣ�������˲����飨filter banks��������ӳٵ���Ƶ��
        //AACDEC_INTR��ֵΪ 4������ʾ���������ǲ������ģ�discontinuous������Ҫ����ͬ���������ڲ�״̬��
        aac_error = aacDecoder_DecodeFrame(aac_coder, (INT_PCM*)pcm_frame,
            pcm_framesize / sizeof(INT_PCM), 0);
        if (aac_error > 0)
        {
            std::cout << "decode frame error\n";
            break;
        }

        audio_chunk = (Uint8*)pcm_frame;
        audio_len = pcm_framesize;
        audio_pos = audio_chunk;
        while (audio_len > 0) //�ȴ�ʱ����SDL�߳����FillAudio����������
            SDL_Delay(1);

        printf("count=%d, aac_FrameLength=%d, pcm_FrameSize=%d \n", count, adts_header.aacFrameLength, pcm_framesize);
        count++;
    }
    free(frame);
    free(pcm_frame);
    fclose(aac_file);
    SDL_Quit();
    return 0;
}