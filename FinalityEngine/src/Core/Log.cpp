#include "Log.h"

std::shared_ptr<spdlog::logger> FINALITY::Logger::s_CoreLogger;
std::shared_ptr<spdlog::logger> FINALITY::Logger::s_ClientLogger;
std::shared_ptr<FINALITY::ConsoleSink> FINALITY::Logger::s_ConsoleSink;

void FINALITY::Logger::Init()
{
	s_ConsoleSink = std::make_shared<ConsoleSink>();
	s_ConsoleSink->set_pattern("%^[%T] %n: %v%$");
	spdlog::set_pattern("%^[%T] %n: %v%$");

	s_CoreLogger = std::make_shared<spdlog::logger>("ENGINE", s_ConsoleSink);
	s_CoreLogger->set_level(spdlog::level::trace);

	s_ClientLogger = std::make_shared<spdlog::logger>("CLIENT", s_ConsoleSink);
	s_ClientLogger->set_level(spdlog::level::trace);
}

void FINALITY::ConsoleSink::sink_it_(const spdlog::details::log_msg& msg)
{
	spdlog::memory_buf_t formatted;
	base_sink<std::mutex>::formatter_->format(msg, formatted);

	m_Entries.push_back({ std::string(formatted.data(), formatted.size()), msg.level });
	if (m_Entries.size() > s_MaxEntries)
	{
		m_Entries.pop_front();
	}
}

std::vector<FINALITY::LogEntry> FINALITY::ConsoleSink::CopyEntries()
{
	std::lock_guard<std::mutex> lock(base_sink<std::mutex>::mutex_);
	return std::vector<LogEntry>(m_Entries.begin(), m_Entries.end());
}

void FINALITY::ConsoleSink::Clear()
{
	std::lock_guard<std::mutex> lock(base_sink<std::mutex>::mutex_);
	m_Entries.clear();
}