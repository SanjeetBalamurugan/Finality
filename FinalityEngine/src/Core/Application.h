#pragma once
#include "Core.h"
#include "RenderTypes.h"
#include "Window.h"
#include "RenderDevice.h"

#include <memory>
#include "Game.h"

namespace FINALITY
{
	class ImGUIRenderer;

	class FAPI Application
	{
	public:
		Application();
		~Application();

		void Initialize(const RendererAPI& api, const WindowSpec& spec, std::unique_ptr<Game> game);
		void Update();
		void Shutdown();

		void RequestClose() { m_Running = false; }

		static RendererAPI GetActiveAPI() { return s_CurrentAPI; }
		static Application& Get() { return *s_Instance; }
		
		RenderDevice& GetDevice() { return *m_RenderDevice; }
		Window& GetWindow() { return *m_Window; }
		ImGUIRenderer* GetImGUIRenderer();

	private:
		static RendererAPI s_CurrentAPI;

		std::unique_ptr<Window> m_Window;
		std::unique_ptr<RenderDevice> m_RenderDevice;
		std::unique_ptr<Game> m_CurrentGame;

		std::unique_ptr<ImGUIRenderer> m_ImGUIRenderer;

		bool m_Running = false;
		static inline Application* s_Instance = nullptr;
	};
}
