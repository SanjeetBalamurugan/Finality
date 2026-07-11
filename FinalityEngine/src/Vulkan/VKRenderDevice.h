#pragma once

#include <Core/Core.h>
#include <Core/Window.h>
#include <Core/RenderDevice.h>
#include "VKCore.h"
#include <vector>
#include <memory>
#include "VkPhysicalDevices.h"
#include "VkQueue.h"
#include "VKSwapChain.h"
#include "VKUniformBuffer.h"

#include <Renderer/Material.h>
#include "VKDeletionQueue.h"
#include "VKDescriptorAllocator.h"
#include <Core/Framebuffer.h>

#include "VKFramebuffer.h"

namespace FINALITY
{
	class VKRenderDevice : public RenderDevice
	{
	private:
		void CreateInstance();
		void SetupDebugMessanger();

		void CreateSurface(const NativeWindowHandle& handle);
		void DestroySurface() const;

		void CreateDevice();
		void DestroyDevice();

		void CreateCommandBuffers(uint32_t count);
		void CreateCommandBufferPool();
		void BeginCommandBuffers(VkCommandBuffer cmdBuf, uint32_t usageFlags);

	public:
		void Initialize(const NativeWindowHandle& handle) override;
		void Shutdown() override;

		void BeginFrame() override;
		void EndFrame() override;
		void PresentFrame() override;

		void WaitIdle() override { vkDeviceWaitIdle(m_Device); }

		void Clear(float r, float g, float b, float a) override;
		void SetWindowSpec(const WindowSpec& spec) override;
		VkDevice GetActiveDevice() const { return m_Device; }

		void DrawQueue(const std::vector<RenderPacket>& queue) override;

		std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex>& vertices) override;
		std::shared_ptr<Mesh> CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) override;
		std::shared_ptr<Pipeline> CreatePipeline(const PipelineConfig& config) override;

		VkDescriptorSetLayout GetGlobalDescriptorSetLayout() const { return m_GlobalDescriptorSetLayout; }
		VkDescriptorSet GetGlobalDescriptorSet(uint32_t index) const { return m_GlobalDescriptorSets[index]; }

		VkPhysicalDevice GetActivePhysicalDevice() { return m_Devices.SelectedDevice().device; }
		VkCommandPool GetCommandPool() { return m_CMDBufPool; }
		VkQueue GetGraphicsQueue() { return m_Queue.GetQueue(); }

		VkDescriptorSetLayout GetMaterialDescriptorSetLayout() const { return m_MaterialDescriptorSetLayout; }

		void SubmitResourceToGarbageCollection(std::function<void()>&& cleanupOperation)
		{
			m_FrameDeletionQueues[m_ImageIndex].Push(std::move(cleanupOperation));
		}

		VkRenderPass GetActiveRenderPass() const
		{
			return static_cast<VKFramebuffer*>(m_PostProcessingFramebuffer.get())->GetVKRenderPass();
		}

	private:
		WindowSpec m_Spec;
		const std::vector<const char*> m_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		bool m_EnableValidationLayers = false;

		VkInstance m_Instance = nullptr;
		VkApplicationInfo m_AppInfo;
		VkDebugUtilsMessengerEXT m_DebugMessanger;
		VkSurfaceKHR m_Surface = nullptr;

		VkPhysicalDevices m_Devices;
		uint32_t m_QueueFamily;
		VkDevice m_Device;

		std::unique_ptr<VKSwapChain> m_SwapChain;
		VKQueue m_Queue;

		std::vector<VkCommandBuffer> m_CMDBuffers;
		VkCommandPool m_CMDBufPool;
		uint32_t m_ImageIndex = 0;

		VkClearColorValue m_ClearColor;

		VkDescriptorSetLayout m_GlobalDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_GlobalDescriptorPool = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> m_GlobalDescriptorSets;

		VKUniformBuffer m_GlobalUBO;

		VkDescriptorSetLayout m_MaterialDescriptorSetLayout = VK_NULL_HANDLE;
		VKDescriptorAllocator m_MaterialDescriptorAllocator;
		std::unordered_map<const Material*, VkDescriptorSet> m_MaterialDescriptorCache;

		std::vector<VKDeletionQueue> m_FrameDeletionQueues;
		std::shared_ptr<Framebuffer> m_PostProcessingFramebuffer = nullptr;
	};
}
