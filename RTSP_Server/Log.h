#pragma once
#include<time.h>
#include<string>
#include<vector>

static std::string Get_Present_Time()
{
	const char* time_fmt = "%Y-%m-%d %H:%M:%S";
	time_t present_second_time = time(nullptr);
	char present_time[64];
	strftime(present_time, sizeof(present_time), time_fmt, localtime(&present_second_time));
	return present_time;
}

static std::string Get_Filename(std::string file)
{
#ifndef _WIN32
	std::string pattern = "/";
#else
	std::string pattern = "\\";
#endif

	std::string::size_type pos; //size_t/unsigned int
	std::vector<std::string> path;
	file += pattern;
	int file_size = file.size();
	for (int i = 0; i < file_size; ++i)
	{
		pos = file.find(pattern, i); //find pattern begin with i
		if (pos < file_size)
		{
			std::string cutstr = file.substr(i, pos - i);// /curstr/
			path.push_back(cutstr);
			i = pos + pattern.size() - 1;
		}
	}
	return path.back();
}

//LOGI("This is a log message with arguments: %d, %s", 42, "Hello");
//[INFO]CurrentTime [example.cpp:10] This is a log message with arguments: 42, Hello

#define LOGI(format, ...) \
fprintf(stderr, "[INOF]%s [%s:%d]" format "\n", Get_Present_Time().data(), __FILE__,__LINE__,##__VA_ARGS__)

#define LOGE(format, ...) \
fprintf(stderr, "[ERROR]%s [%s:%d]" format "\n", Get_Present_Time().data(), __FILE__,__LINE__,##__VA_ARGS__)

