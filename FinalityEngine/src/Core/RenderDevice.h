#pragma once
#include "Core.h"

#include "Window.h"

namespace FINALITY
{
	class FAPI RenderDevice
	{
	public:
		~RenderDevice() = default;

		virtual void Initialize() = 0;
		virtual void Shutdown() = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void PresentFrame() = 0;

		virtual void Clear(float r, float g, float b, float a) = 0;

		virtual void SetWindowSpec(const WindowSpec& spec) = 0;
	};
}