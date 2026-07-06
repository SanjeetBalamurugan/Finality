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
		virtual ~Window() = default;

		virtual void Initialize(const WindowSpec& specification) = 0;
		virtual void Update() = 0;
		virtual void Shutdown() = 0;

		virtual bool ShouldClose() const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
	};
}