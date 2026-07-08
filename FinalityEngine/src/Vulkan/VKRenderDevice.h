#pragma once

#include <Core/Core.h>

#include <Core/Window.h>
#include <Core/RenderDevice.h>
#include "VKCore.h"

#include <vector>
#include "VkPhysicalDevices.h"
#include "VkQueue.h"

namespace FINALITY
{
	class VKRenderDevice : public RenderDevice
	{
	private:
		void CreateInstance();
		bool CheckValidationLayerSupport() const;
		std::vector<const char*> GetRequiredExtensions() const;
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);
		void SetupDebugMessanger();
		VkResult CreateDebugUtilsMessengerEXT(
			VkInstance instance,
			const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator,
			VkDebugUtilsMessengerEXT* pDebugMessenger);
		void DestroyDebugUtilsMessengerEXT(
			VkInstance instance,
			VkDebugUtilsMessengerEXT debugMessenger,
			const VkAllocationCallbacks* pAllocator);
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		void CreateSurface(const NativeWindowHandle& handle);
		void DestroySurface();

		void CreateDevice();
		void DestroyDevice();

		void CreateSwapChain();
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

		void Clear(float r, float g, float b, float a) override;
		void SetWindowSpec(const WindowSpec& spec) override { m_Spec = spec; }

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
		VkSwapchainKHR m_SwapChain = nullptr;

		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;

		VkPhysicalDevices m_Devices;
		uint32_t m_QueueFamily;
		VkDevice m_Device;

		std::vector<VkCommandBuffer> m_CMDBuffers;
		VkCommandPool m_CMDBufPool;

		VKQueue m_Queue;
		uint32_t m_ImageIndex = 0;
	};
}