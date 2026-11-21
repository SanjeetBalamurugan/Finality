/**
 * @file
 * @ingroup tools
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for ImGUIRenderer.h.
 */
#pragma once
#include <memory>
#include <Core/RenderDevice.h>

#include <imgui.h>

namespace FINALITY
{
	/** @todo TODO: Document ImGUIRenderer class semantics, ownership and usage. */
	class ImGUIRenderer
	{
	private:
	public:
		ImGUIRenderer(WindowSpec spec, RenderDevice* device)
			: m_Specification(spec), m_Device(device){};
		virtual ~ImGUIRenderer() = default;

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
