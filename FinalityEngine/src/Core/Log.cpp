#include "Log.h"

std::shared_ptr<spdlog::logger> FINALITY::Logger::s_CoreLogger;
std::shared_ptr<spdlog::logger> FINALITY::Logger::s_ClientLogger;

void FINALITY::Logger::Init()
{
	spdlog::set_pattern("%^[%T] %n: %v%$");

	s_CoreLogger = spdlog::stdout_color_mt("ENGINE");
	s_CoreLogger->set_level(spdlog::level::trace);

	s_ClientLogger = spdlog::stdout_color_mt("CLIENT");
	s_ClientLogger->set_level(spdlog::level::trace);
}
