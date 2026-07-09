#pragma once
#include "Core.h"
#include "RenderTypes.h"
#include "Window.h"
#include "RenderDevice.h"

#include <memory>
#include "Game.h"

namespace FINALITY
{
	class FAPI Application
	{
	public:
		void Initialize(const RendererAPI& api, const WindowSpec& spec, std::unique_ptr<Game> game);
		void Update();
		void Shutdown();

		static RendererAPI GetActiveAPI() { return s_CurrentAPI; }

	private:
		static RendererAPI s_CurrentAPI;
		std::unique_ptr<Window> m_Window;
		std::unique_ptr<RenderDevice> m_RenderDevice;
		std::unique_ptr<Game> m_CurrentGame;

		bool m_Running = false;
	};
}
