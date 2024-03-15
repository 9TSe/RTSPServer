#define _CRT_SECURE_NO_WARNINGS
#pragma once
#include <time.h>
#include <chrono>
#include <string>

static std::string Get_CurTimeStr(const char* fmt = "%Y-%m-%d %H:%M:%S")
{
	time_t t = time(nullptr);
	char str[64];
	strftime(str, sizeof(str), fmt, localtime(&t));
	return str;
}

#define LOGI(format, ...) \
fprintf(stderr, "[INFO]%s [%s:%d %s()] " format "\n", Get_CurTimeStr().data(), __FILE__, __LINE__,__func__,##__VA_ARGS__)

#define LOGE(format, ...) \
fprintf(stderr, "[ERROR]%s [%s:%d %s()] " format "\n", Get_CurTimeStr().data(),__FILE__, __LINE__,__func__,##__VA_ARGS__)