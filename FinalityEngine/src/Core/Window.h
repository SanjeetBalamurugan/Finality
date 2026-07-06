#pragma once
#include "Core.h"

#include <string>

namespace FINALITY
{
	struct WindowSpec
	{
		std::string title;
		int width;
		int height;
		bool isFullscreen;
	};

	class FAPI Window
	{
	public:
		Window(WindowSpec specification)
			: m_Specification(specification) {};

		virtual void OnInit() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnEnd() = 0;
	private:
		WindowSpec m_Specification;
	};
}