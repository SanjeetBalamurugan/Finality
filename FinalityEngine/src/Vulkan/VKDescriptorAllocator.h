/**
 * @file
 * @ingroup vulkan
 * @todo TODO: Document responsibilities, ownership, public API contracts, failure behavior and usage for VKDescriptorAllocator.h.
 */
#pragma once
#include "VKCore.h"
#include <vector>
#include <unordered_map>

namespace FINALITY
{
    /** @todo TODO: Document VKDescriptorAllocator class semantics, ownership and usage. */
    class VKDescriptorAllocator
    {
    public:
        VKDescriptorAllocator() = default;
        ~VKDescriptorAllocator() { Shutdown(); }

        void Initialize(VkDevice device);
        void Shutdown();

        VkDescriptorSet Allocate(VkDescriptorSetLayout layout);
        void Free(VkDescriptorSet set);
        void ResetPools();

    private:
        VkDescriptorPool CreatePoolPage();

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkDescriptorPool m_CurrentPool = VK_NULL_HANDLE;
        std::unordered_map<VkDescriptorSet, VkDescriptorPool> m_AllocationPools;

        std::vector<VkDescriptorPool> m_UsedPoolPages;
        std::vector<VkDescriptorPool> m_FreePoolPages;
    };
}
