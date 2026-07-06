#pragma once

#include "Core.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>

namespace FINALITY
{
	class FAPI Logger
	{
	public:
		static void Init();
		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};
}

#define FI_CORE_ERROR(...)  ::FINALITY::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define FI_CORE_WARN(...)   ::FINALITY::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define FI_CORE_INFO(...)   ::FINALITY::Logger::GetCoreLogger()->info(__VA_ARGS__)

#define ME_ERROR(...)       ::FINALITY::Logger::GetClientLogger()->error(__VA_ARGS__)
#define ME_WARN(...)        ::FINALITY::Logger::GetClientLogger()->warn(__VA_ARGS__)
#define ME_INFO(...)        ::FINALITY::Logger::GetClientLogger()->info(__VA_ARGS__)