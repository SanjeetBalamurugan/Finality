#pragma once
#include <Vulkan/VKCore.h>

#include <ImGUI/ImGUIRenderer.h>
#include <Core/Framebuffer.h>

namespace FINALITY
{
	class VKImGUIRenderer : public ImGUIRenderer
	{
	public:
		VKImGUIRenderer(WindowSpec spec, RenderDevice* device)
			: ImGUIRenderer(spec, device) {}
		~VKImGUIRenderer() {};

		void Destroy() override;
		void Update() override;
		void Initialize() override;

		VkCommandBuffer PrepareCommandBuffer(int image);
		VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }
	private:
		void CreateDescriptorPool();

		std::vector<VkCommandBuffer> m_CMDBufs;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	};
}