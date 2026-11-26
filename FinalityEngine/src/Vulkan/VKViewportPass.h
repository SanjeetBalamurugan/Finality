/**
 * @file
 * @ingroup vulkan
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for VKViewportPass.h.
 */
#pragma once
#include "VKUniformBuffer.h"
#include <Core/RenderTarget.h>

namespace FINALITY
{
    /** @todo TODO: Document VKViewportPass struct semantics, ownership and usage. */
    struct VKViewportPass
    {
        VkCommandBuffer Command = VK_NULL_HANDLE;
        VkDescriptorPool Pool = VK_NULL_HANDLE;
        VkDescriptorSet Globals = VK_NULL_HANDLE;
        VKUniformBuffer Camera;
        VkBuffer Instances = VK_NULL_HANDLE;
        VkDeviceMemory InstanceMemory = VK_NULL_HANDLE;
        uint8_t* Mapped = nullptr;
        VkDeviceSize Capacity = 0, Offset = 0;
        VkQueryPool Queries = VK_NULL_HANDLE;
        bool Submitted = false;
        std::weak_ptr<RenderTarget> Target;

        void Initialize(VkDevice device, VkPhysicalDevice physical, VkCommandPool commands, VkDescriptorSetLayout layout, bool timestamps);
        void Destroy(VkDevice device, VkCommandPool commands);
        void CollectTiming(VkDevice device, double period, uint32_t validBits);
    };
} // namespace FINALITY
