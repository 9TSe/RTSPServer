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

	FILE* pcm_file = fopen(PCM_FILE_NAME, "rb+"); //�����Զ����Ƹ�ʽ��ȡ��д���ļ�
	if (!pcm_file)
	{
		std::cout << "open file fail\n";
		return -1;
	}

	int pcm_buf_size = 409600; //һ���Դ�pcm�ļ��ж�ȡ�����ݴ�С
	char* pcm_buf = (char*)malloc(pcm_buf_size);
	int data_sum = 0;
	size_t count;

	while (1)
	{
		count = fread(pcm_buf, 1, pcm_buf_size, pcm_file);
		if (count != pcm_buf_size) //�����������ѭ������
		{
			std::cout << "read pcm end, count = " << count << std::endl;
			fseek(pcm_file, 0, SEEK_SET); //0 �����ļ�ָ�����·��ÿ�ͷλ��
			fread(pcm_buf, 1, pcm_buf_size, pcm_file);
			data_sum = 0;
			//break;
		}
		std::cout << "now playing " << data_sum << "bytes data\n";

		data_sum += pcm_buf_size;
		audio_chunk = (Uint8*)pcm_buf;
		audio_len = pcm_buf_size;
		audio_pos = audio_chunk;
		/*������Ƶ�Ĳ�����λ44100����ÿ���Ӳ���44100�Ρ�
		AACһ�㽫1024�β��������һ֡������һ�����44100 / 1024 = 43֡��
		RTP�����͵�ÿһ֡���ݵ�ʱ������Ϊ44100 / 43 = 1025��
		ÿһ֡���ݵ�ʱ����Ϊ1000 / 43 = 23ms��*/
		while (audio_len > 0)
			SDL_Delay(23);
	}
	free(pcm_buf);
	SDL_Quit();
	return 0;
}