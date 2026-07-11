#pragma once
#include "VKCore.h"
#include <vector>

namespace FINALITY
{
    class VKDescriptorAllocator
    {
    public:
        VKDescriptorAllocator() = default;
        ~VKDescriptorAllocator() { Shutdown(); }

        void Initialize(VkDevice device);
        void Shutdown();

        VkDescriptorSet Allocate(VkDescriptorSetLayout layout);
        void ResetPools();

    private:
        VkDescriptorPool CreatePoolPage();

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
        VkDescriptorPool m_CurrentPool = VK_NULL_HANDLE;

        std::vector<VkDescriptorPool> m_UsedPoolPages;
        std::vector<VkDescriptorPool> m_FreePoolPages;
    };
}
