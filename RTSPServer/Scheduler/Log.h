#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
namespace spdLog {

	class  Log {
	public:
		static void Init();
		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
	};
}


#define LOG_CORE_TRACE(...)  ::spdLog::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define LOG_CORE_INFO(...)   ::spdLog::Log::GetCoreLogger()->info(__VA_ARGS__)
#define LOG_CORE_WARN(...)   ::spdLog::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define LOG_CORE_ERROR(...)  ::spdLog::Log::GetCoreLogger()->error(__VA_ARGS__)
#define LOG_CORE_FATAL(...)  ::spdLog::Log::GetCoreLogger()->fatal(__VA_ARGS__)