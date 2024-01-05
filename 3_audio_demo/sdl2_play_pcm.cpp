#define SDL_MAIN_HANDLED
#define PCM_FILE_NAME "D:/ffmpeg/learn/test.pcm"
#pragma warning( disable : 4996 )
#include <iostream>
#include <stdio.h>
#include <SDL.h>

static Uint8* audio_chunk;
static Uint32 audio_len;
static Uint8* audio_pos;

void Fill_Audio(void* data, Uint8* stream, int len)
{
	SDL_memset(stream, 0, len);
	if (audio_len == 0) return;
	len = len > audio_len ? audio_len : len;
	SDL_MixAudio(stream, audio_pos, len, SDL_MIX_MAXVOLUME);
	audio_pos += len;
	audio_len -= len;
}

int main()
{
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		std::cout << "init SDL fail - " << SDL_GetError << std::endl;
		return -1;
	}

	SDL_AudioSpec SDL_spec;
	SDL_spec.freq = 44100;
	SDL_spec.format = AUDIO_S16SYS;
	SDL_spec.channels = 2;
	SDL_spec.silence = 0;
	SDL_spec.samples = 1024;
	SDL_spec.callback = Fill_Audio;

	if (SDL_OpenAudio(&SDL_spec, nullptr) < 0)
	{
		std::cout << "SDL open fail\n";
		return -1;
	}
	SDL_PauseAudio(0);

	FILE* pcm_file = fopen(PCM_FILE_NAME, "rb+"); //允许以二进制格式读取和写入文件
	if (!pcm_file)
	{
		std::cout << "open file fail\n";
		return -1;
	}

	int pcm_buf_size = 409600; //一次性从pcm文件中读取的数据大小
	char* pcm_buf = (char*)malloc(pcm_buf_size);
	int data_sum = 0;
	size_t count;

	while (1)
	{
		count = fread(pcm_buf, 1, pcm_buf_size, pcm_file);
		if (count != pcm_buf_size) //播放完后重新循环播放
		{
			std::cout << "read pcm end, count = " << count << std::endl;
			fseek(pcm_file, 0, SEEK_SET); //0 代表将文件指针重新放置开头位置
			fread(pcm_buf, 1, pcm_buf_size, pcm_file);
			data_sum = 0;
			//break;
		}
		std::cout << "now playing " << data_sum << "bytes data\n";

		data_sum += pcm_buf_size;
		audio_chunk = (Uint8*)pcm_buf;
		audio_len = pcm_buf_size;
		audio_pos = audio_chunk;
		/*假设音频的采样率位44100，即每秒钟采样44100次。
		AAC一般将1024次采样编码成一帧，所以一秒就有44100 / 1024 = 43帧。
		RTP包发送的每一帧数据的时间增量为44100 / 43 = 1025。
		每一帧数据的时间间隔为1000 / 43 = 23ms。*/
		while (audio_len > 0)
			SDL_Delay(23);
	}
	free(pcm_buf);
	SDL_Quit();
	return 0;
}