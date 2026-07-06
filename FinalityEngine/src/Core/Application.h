#pragma once
#include "Core.h"
#include "RenderTypes.h"
#include "Window.h"

#include <memory>

namespace FINALITY
{
	class FAPI Application
	{
	public:
		void Initialize(const RendererAPI& api, const WindowSpec& spec);
		void Update();
		void Shutdown();

	private:
		RendererAPI m_CurrentAPI = RendererAPI::NONE;
		std::unique_ptr<Window> m_Window;

		bool m_Running = false;
	};
}
