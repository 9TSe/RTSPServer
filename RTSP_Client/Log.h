#pragma once
#include <time.h>
#include <string>
#pragma warning( disable : 4996 )

static std::string Get_Time() {
    const char* format = "%Y-%m-%d %H:%M:%S";
    time_t t = time(nullptr);

    char tc[64];
    strftime(tc, sizeof(tc), format, localtime(&t));

    std::string timeStr = tc;
    return timeStr;
}
#define LOGI(format, ...)  fprintf(stderr,"[I]%s [%d %s()] " format "\n", Get_Time().data(),__LINE__,__func__ ,##__VA_ARGS__)
#define LOGE(format, ...)  fprintf(stderr,"[E]%s [%d %s()] " format "\n",Get_Time().data(),__LINE__,__func__ ,##__VA_ARGS__)
