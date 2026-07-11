#include "VKDescriptorAllocator.h"
#include <stdexcept>

namespace FINALITY
{
    void VKDescriptorAllocator::Initialize(VkDevice device)
    {
        m_Device = device;
    }

    VkDescriptorPool VKDescriptorAllocator::CreatePoolPage()
    {
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 64 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 64 }
        };

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = 0;
        poolInfo.maxSets = 64; // Max 64 allocations per individual descriptor page
        poolInfo.poolSizeCount = 2;
        poolInfo.pPoolSizes = poolSizes;

        VkDescriptorPool pool;
        if (vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &pool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate background descriptor allocator page!");
        }

        return pool;
    }

    VkDescriptorSet VKDescriptorAllocator::Allocate(VkDescriptorSetLayout layout)
    {
        if (m_CurrentPool == VK_NULL_HANDLE)
        {
            m_CurrentPool = CreatePoolPage();
            m_UsedPoolPages.push_back(m_CurrentPool);
        }

        VkDescriptorSetLayout layouts[] = { layout };
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_CurrentPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = layouts;

        VkDescriptorSet descriptorSet;
        VkResult result = vkAllocateDescriptorSets(m_Device, &allocInfo, &descriptorSet);

        if (result == VK_ERROR_FRAGMENTED_POOL || result == VK_ERROR_OUT_OF_POOL_MEMORY)
        {
            m_CurrentPool = CreatePoolPage();
            m_UsedPoolPages.push_back(m_CurrentPool);

            allocInfo.descriptorPool = m_CurrentPool;
            result = vkAllocateDescriptorSets(m_Device, &allocInfo, &descriptorSet);
        }

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Critical Vulkan Error: Unable to allocate descriptor layout sets across paging pools!");
        }

        return descriptorSet;
    }

    void VKDescriptorAllocator::ResetPools()
    {
        for (auto pool : m_UsedPoolPages)
        {
            vkResetDescriptorPool(m_Device, pool, 0);
            m_FreePoolPages.push_back(pool);
        }

        m_UsedPoolPages.clear();
        m_CurrentPool = VK_NULL_HANDLE;
    }

    void VKDescriptorAllocator::Shutdown()
    {
        for (auto pool : m_UsedPoolPages) vkDestroyDescriptorPool(m_Device, pool, nullptr);
        for (auto pool : m_FreePoolPages) vkDestroyDescriptorPool(m_Device, pool, nullptr);

        m_UsedPoolPages.clear();
        m_FreePoolPages.clear();
        m_CurrentPool = VK_NULL_HANDLE;
    }
}
