#pragma once
#include "VKCore.h"

namespace FINALITY
{
	struct PhysicalDevice
	{
		VkPhysicalDevice device;
		VkPhysicalDeviceProperties properties;
		std::vector<VkQueueFamilyProperties> qFamilyProps;
		std::vector<VkBool32> qSupportsPresent;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		VkPhysicalDeviceMemoryProperties memoryProperties;
		std::vector<VkPresentModeKHR> presentModes;
	};

	class VkPhysicalDevices
	{
	public:
		void Initialize(const VkInstance& instance, const VkSurfaceKHR& surface);
		uint32_t SelectDevice(VkQueueFlags requiredQueueTypes, bool supportsPresent);
		const PhysicalDevice& SelectedDevice() const;

	private:
		std::vector<PhysicalDevice> m_Devices;
		int m_DevIndex = -1;
	};
}