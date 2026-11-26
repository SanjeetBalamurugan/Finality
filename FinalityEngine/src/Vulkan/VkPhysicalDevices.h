/**
 * @file
 * @ingroup vulkan
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for VkPhysicalDevices.h.
 */
#pragma once
#include "VKCore.h"

namespace FINALITY
{
    /** @todo TODO: Document PhysicalDevice struct semantics, ownership and usage. */
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

    /** @todo TODO: Document VkPhysicalDevices class semantics, ownership and usage. */
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
} // namespace FINALITY