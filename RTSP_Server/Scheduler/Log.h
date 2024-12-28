#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
namespace spdLog {

	class  Log {
	public:
		static void Init();
		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		//inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		//static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}


#define LOG_CORE_TRACE(...)  ::spdLog::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define LOG_CORE_INFO(...)   ::spdLog::Log::GetCoreLogger()->info(__VA_ARGS__)
#define LOG_CORE_WARN(...)   ::spdLog::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define LOG_CORE_ERROR(...)  ::spdLog::Log::GetCoreLogger()->error(__VA_ARGS__)
#define LOG_CORE_FATAL(...)  ::spdLog::Log::GetCoreLogger()->fatal(__VA_ARGS__)

// #define LOG_TRACE(...)  ::spdLog::Log::GetClientLogger()->trace(__VA_ARGS__)
// #define LOG_INFO(...)   ::spdLog::Log::GetClientLogger()->info(__VA_ARGS__)
// #define LOG_WARN(...)   ::spdLog::Log::GetClientLogger()->warn(__VA_ARGS__)
// #define LOG_ERROR(...)  ::spdLog::Log::GetClientLogger()->error(__VA_ARGS__)
// #define LOG_FATAL(...)  ::spdLog::Log::GetClientLogger()->fatal(__VA_ARGS__)