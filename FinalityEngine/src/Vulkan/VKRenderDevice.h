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
		void RecordCommandBuffers();

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
	};
}
