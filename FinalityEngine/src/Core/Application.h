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
		void Init(RendererAPI api);

	private:
		RendererAPI m_CurrentAPI = RendererAPI::NONE;
		std::unique_ptr<Window> m_Window;
	};
}
