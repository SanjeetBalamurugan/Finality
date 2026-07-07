#pragma once

#include <Core/Core.h>

#include <Core/Window.h>
#include <Core/RenderDevice.h>
#include "VKCore.h"

#include <vector>

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
	public:
		void Initialize() override;
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
	};
}