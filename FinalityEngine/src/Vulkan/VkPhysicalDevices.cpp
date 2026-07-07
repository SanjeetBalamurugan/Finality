#include "VkPhysicalDevices.h"

void FINALITY::VkPhysicalDevices::Initialize(const VkInstance& instance, const VkSurfaceKHR& surface)
{
	uint32_t numDevices = 0;

	VkResult res = vkEnumeratePhysicalDevices(instance, &numDevices, nullptr);
	CHECK_VK_RESULT(res, "vkEnumeratePhysicalDevices error");

	FI_CORE_INFO("Number of Devices: {}", numDevices);
	m_Devices.resize(numDevices);

	std::vector<VkPhysicalDevice> vkDevices;
	vkDevices.resize(numDevices);
	
	res = vkEnumeratePhysicalDevices(instance, &numDevices, vkDevices.data());
	CHECK_VK_RESULT(res, "vkEnumeratePhysicalDevices error");

	for (size_t i = 0; i < numDevices; i++)
	{
		VkPhysicalDevice device = vkDevices[i];
		m_Devices[i].device = device;

		vkGetPhysicalDeviceProperties(device, &m_Devices[i].properties);
		FI_CORE_INFO("Device Name: {}", m_Devices[i].properties.deviceName);

		uint32_t apiVersion = m_Devices[i].properties.apiVersion;
		FI_CORE_INFO("API VERSION: {}.{}.{}.{}",
			VK_API_VERSION_VARIANT(apiVersion),
			VK_API_VERSION_MAJOR(apiVersion),
			VK_API_VERSION_MINOR(apiVersion),
			VK_API_VERSION_PATCH(apiVersion));

		uint32_t numberOfQueueFamilies = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &numberOfQueueFamilies, nullptr);
		FI_CORE_INFO("Number Of Family Queues: {}", numberOfQueueFamilies);

		m_Devices[i].qFamilyProps.resize(numberOfQueueFamilies);
		m_Devices[i].qSupportsPresent.resize(numberOfQueueFamilies);

		vkGetPhysicalDeviceQueueFamilyProperties(device, &numberOfQueueFamilies, m_Devices[i].qFamilyProps.data());

		for (size_t q = 0; q < numberOfQueueFamilies; q++)
		{
			const VkQueueFamilyProperties& QFamilyProps = m_Devices[i].qFamilyProps[q];
			res = vkGetPhysicalDeviceSurfaceSupportKHR(device, q, surface, &m_Devices[i].qSupportsPresent.data()[q]);
			CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfaceSupportKHR error");
		}

		uint32_t numFormats = 0;
		res = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numFormats, nullptr);
		CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfaceSupportKHR error");
		assert(numFormats > 0);

		m_Devices[i].surfaceFormats.resize(numFormats);
		res = vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &numFormats, m_Devices[i].surfaceFormats.data());
		CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfaceSupportKHR error");

		res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &m_Devices[i].surfaceCapabilities);
		CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfaceSupportKHR error");

		uint32_t NumPresentModes = 0;

		res = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &NumPresentModes, NULL);
		CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfacePresentModesKHR (1) error\n");

		assert(NumPresentModes != 0);

		m_Devices[i].presentModes.resize(NumPresentModes);

		res = vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &NumPresentModes, m_Devices[i].presentModes.data());
		CHECK_VK_RESULT(res, "vkGetPhysicalDeviceSurfacePresentModesKHR (2) error\n");

		vkGetPhysicalDeviceMemoryProperties(device, &(m_Devices[i].memoryProperties));
	}
}

uint32_t FINALITY::VkPhysicalDevices::SelectDevice(VkQueueFlags requiredQueueTypes, bool supportsPresent)
{
	for (uint32_t i = 0; i < m_Devices.size(); i++) {

		for (uint32_t j = 0; j < m_Devices[i].qFamilyProps.size(); j++) {
			const VkQueueFamilyProperties& QFamilyProp = m_Devices[i].qFamilyProps[j];

			if ((QFamilyProp.queueFlags & requiredQueueTypes) && ((bool)m_Devices[i].qSupportsPresent[j] == supportsPresent)) {
				m_DevIndex = i;
				int QueueFamily = j;
				printf("Using GFX device %d and queue family %d\n", m_DevIndex, QueueFamily);
				return QueueFamily;
			}
		}
	}

	FI_CORE_ERROR("Required queue type {} and supports present {} not found\n", requiredQueueTypes, supportsPresent);

	return 0;
}

const FINALITY::PhysicalDevice& FINALITY::VkPhysicalDevices::SelectedDevice() const
{
	if (m_DevIndex < 0) {
		FI_CORE_ERROR("A physical device has not been selected\n");
	}

	return m_Devices[m_DevIndex];
}
