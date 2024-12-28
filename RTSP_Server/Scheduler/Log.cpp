#include "Log.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>
namespace spdLog {
	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	//std::shared_ptr<spdlog::logger> Log::s_ClientLogger;
	void Log::Init()
	{
		spdlog::set_pattern("[%T] %n: %v %$");
		s_CoreLogger = spdlog::stdout_color_mt("RTSPServer");
		s_CoreLogger->set_level(spdlog::level::trace);

		// s_ClientLogger = spdlog::stdout_color_mt("RTSPClient");
		// s_ClientLogger->set_level(spdlog::level::trace);
	}
}
