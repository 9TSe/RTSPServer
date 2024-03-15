#pragma once
#include <time.h>
#include <chrono>
#include <string>

static int64_t Get_CurTime() //sys start to now ms
{
	long long now = std::chrono::steady_clock::now().time_since_epoch().count();
	return now / 1000000;
}

static int64_t Get_CurMillisecond() //get ms stamp
{
	return std::chrono::duration_cast<std::chrono::milliseconds>
		(std::chrono::system_clock::now().time_since_epoch()).count();
}

static int64_t Get_CurrentMillisecond(bool systemTime = false) //systemTime 是否为系统时间
{
	return Get_CurMillisecond();
}