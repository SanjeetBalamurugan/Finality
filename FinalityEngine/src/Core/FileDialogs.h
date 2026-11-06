#pragma once
#include <Core/Core.h>
#include <string>

namespace FINALITY
{
	class FAPI FileDialogs
	{
	public:
		static std::string OpenFile(const char* filter);
		static std::string SaveFile(const char* filter, const char* defaultName);
	};
}