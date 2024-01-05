//播放aac格式的音频
#define SDL_MAIN_HANDLED //SDL 不要提供默认的 main 函数入口
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
//stream 这个指针是 SDL 内部音频数据内存的指针，只要把数据拷贝到这个指针的地址，就能播放声音了。
//len 表示音频缓存区的大小
//SDL 打开音频硬件设备的时候，SDL 库就会创建一个线程，来及时执行回调函数 sdl_audio_callback()
//至于 SDL 线程多久回调一次函数，不需要太关心
void Fill_Audio(void* data, Uint8* stream, int len)
{
    SDL_memset(stream, 0, len);
    if (audio_len == 0) return;
    len = len > audio_len ? audio_len : len; //取min, audio_len是pcm_framesize
    //音量范围为0~128,设置为SDL_MIX_MAXVOLUME表示完整的音频音量
    //将audio_pos音频 混进 stream, len以字节计数大小
    SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
    audio_pos += len; //跳至下一个播放帧的起点
    audio_len -= len; //使主函数中的while等待终止,同时也给予自己足够时间执行改函数
}

int main()
{
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) //初始化音频子系统 | 初始化定时器子系统
	{
		std::cout << "init SDL fail - " << SDL_GetError() << std::endl;
		return -1;
	}
    //typedef struct SDL_AudioSpec 
    //{
    //    int freq;                   // 采样率 (samples per second)
    //    SDL_AudioFormat format;     // 音频格式
    //    Uint8 channels;             // 声道数 (1 for mono, 2 for stereo)
    //    Uint8 silence;              // 静音值 (通常为 0)
    //    Uint16 samples;             // 音频缓冲区大小 (单位是采样数)
    //    Uint16 padding;             // 对齐用的填充 (通常为 0)
    //    Uint32 size;                // 音频缓冲区大小 (单位是字节)
    //    SDL_AudioCallback callback; // 回调函数指针，用于处理音频数据
    //    void* userdata;             // 用户自定义数据
    //};
    SDL_AudioSpec SDL_spec;
    SDL_spec.freq = 44100;
    SDL_spec.format = AUDIO_S16SYS; //16-bit signed integer 的音频数据格式,使用主机系统的字节顺序
    SDL_spec.channels = 2;
    SDL_spec.silence = 0;
    SDL_spec.samples = 1024; //音频缓冲区大小(单位是采样数)
    SDL_spec.callback = Fill_Audio;

    if (SDL_OpenAudio(&SDL_spec, nullptr) < 0) //打开音频设备并根据 SDL_spec 的要求进行初始化。
    {
        std::cout << "open audio fail\n";
        return -1;
    }

    SDL_PauseAudio(0);//控制SDL音频设备的暂停状态。参数为0，音频设备将被恢复到正常状态并开始播放音频数据；
                      //而当参数为 1 时，音频设备将被暂停，停止播放音频数据。

    FILE* aac_file = fopen(AAC_FILE_NAME, "rb");
    if (!aac_file)
    {
        std::cout << "open file fail\n";
        return -1;
    }

    HANDLE_AACDECODER aac_coder = aacDecoder_Open(TT_MP4_ADTS, 1); //aac解码器实例化
    uint8_t* frame = (uint8_t*)malloc(2000); //存放aac数据
    unsigned int pcm_framesize = 2 * 1024 * sizeof(INT_PCM); //signed short
    unsigned char* pcm_frame = (unsigned char*)malloc(pcm_framesize); //存放pcm格式的数据

    AdtsHeader adts_header;
    int adts_headerlen; //ADTS头长
    int adts_contentlen;//ADTS负载长
    unsigned int adts_len = 0; //ADTS当前帧总长(头+负载)

    int count = 0; //count循环次数, 计帧
    AAC_DECODER_ERROR aac_error;

    while (1)
    {
        adts_headerlen = fread(frame, 1, 7, aac_file); //头长是目前自定的7字节, 读头
        if (adts_headerlen <= 0) //没有数据或出错
        {
            std::cout << "read header error\n";
            break;
        }
        if (Parse_AdtsHeader(frame, &adts_header) < 0) //处理头数据放置于包内
        {
            std::cout << "parse header error\n";
            break;
        }

        //读取aacfile中头后面(一段帧)的数据
        adts_contentlen = fread(frame + 7, 1, adts_header.aacFrameLength - 7, aac_file);
        if (adts_contentlen < 0)
        {
            std::cout << "read content error\n";
            break;
        }

        adts_len = adts_header.aacFrameLength;

        //AAC解码器是解码用的,解什么,就通过这个函数传进去
        //从缓冲区中(frame)
        //外部输入缓冲区的大小(length)
        //返回信息(len) 以便后续调用aacDecoder Fill时，可以确定pBuffer中的正确位置以获取下一个数据。
        aac_error = aacDecoder_Fill(aac_coder, &frame, &adts_header.aacFrameLength, &adts_len);
        if (aac_error > 0)
        {
            std::cout << "decoder fill error\n";
            break;
        }

        //开始编码数据, 将解析为 pcm 格式的数据存储至pcm_frame中
        //最后一位是标志位 :
        //0 无附加信息
        //AACDEC_CONCEAL（值为 1）：表示需要进行数据的掩盖（concealment）处理。
        //AACDEC_FLUSH（值为 2）：表示需要丢弃输入数据，并清空滤波器组（filter banks），输出延迟的音频。
        //AACDEC_INTR（值为 4）：表示输入数据是不连续的（discontinuous），需要重新同步解码器内部状态。
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
        while (audio_len > 0) //等待时间让SDL线程完成FillAudio函数的任务
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