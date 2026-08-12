#pragma once
#include <memory>
#include <Core/RenderDevice.h>

#include <imgui.h>

namespace FINALITY
{
	class ImGUIRenderer
	{
	private:
	public:
		ImGUIRenderer(WindowSpec spec, RenderDevice* device)
			: m_Specification(spec), m_Device(device){};
		~ImGUIRenderer() {};

		void InitImGUI();
		void ShutDown();

		static std::unique_ptr<ImGUIRenderer> Create(RenderDevice* device);
		virtual void Initialize() = 0;
		virtual void Update() = 0;
		virtual void Destroy() = 0;

		WindowSpec GetWindowSpecification() const { return m_Specification; }
		RenderDevice* GetRenderDevice() const { return m_Device; }
	private:
		WindowSpec m_Specification;
		RenderDevice* m_Device;

		RendererAPI m_ActiveAPI = RendererAPI::NONE;
	};
}