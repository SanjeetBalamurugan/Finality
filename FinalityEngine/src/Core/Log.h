#pragma once

#include "Core.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/base_sink.h>
#include <memory>
#include <deque>
#include <mutex>
#include <string>

template class FAPI std::shared_ptr<spdlog::logger>;

namespace FINALITY
{
	struct LogEntry
	{
		std::string Message;
		std::string RawPayload;
		std::string LoggerName;
		std::string stackTrace;
		spdlog::level::level_enum Level;
		std::string FilePath;
		int LineNumber;
		int Count = 1;
	};	

	class FAPI ConsoleSink : public spdlog::sinks::base_sink<std::mutex>
	{
	public:
		std::vector<LogEntry> CopyEntries();
		void Clear();

	protected:
		void sink_it_(const spdlog::details::log_msg& msg) override;
		void flush_() override {}

	private:
		std::deque<LogEntry> m_Entries;
		static constexpr size_t s_MaxEntries = 1000;
	};

	template class FAPI std::shared_ptr<FINALITY::ConsoleSink>;
	class FAPI Logger
	{
	public:
		static void Init();
		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
		inline static std::shared_ptr<ConsoleSink>& GetConsoleSink() { return s_ConsoleSink; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
		static std::shared_ptr<ConsoleSink> s_ConsoleSink;
	};
}

#ifdef FINALITY_BUILD_DLL
	#define FI_CORE_ERROR(...)  SPDLOG_LOGGER_CALL(::FINALITY::Logger::GetCoreLogger(), spdlog::level::err, __VA_ARGS__)
	#define FI_CORE_WARN(...)   SPDLOG_LOGGER_CALL(::FINALITY::Logger::GetCoreLogger(), spdlog::level::warn, __VA_ARGS__)
	#define FI_CORE_INFO(...)   SPDLOG_LOGGER_CALL(::FINALITY::Logger::GetCoreLogger(), spdlog::level::info, __VA_ARGS__)
#endif