#pragma once
#include <chrono>
#include <string>

static std::string GetTime()
{
	const char* format = "%Y-%m-%d %H:%M:%S";
	time_t nowtime = time(nullptr);
	char now[64];
	strftime(now, sizeof(now), format, localtime(&nowtime));
	return now;
}

#define LOGI(format, ...) fprintf(stderr, "[INFO]%s [%s:%d] " format "\n", GetTime().data(), __FILE__, __LINE__, ##__VA_ARGS__)

#define LOGE(format, ...) fprintf(stderr, "[ERRO]%s [%s:%d] " format "\n", GetTime().data(), __FILE__, __LINE__, ##__VA_ARGS__)