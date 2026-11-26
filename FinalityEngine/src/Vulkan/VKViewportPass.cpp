#include "VKViewportPass.h"
#include <stdexcept>

namespace FINALITY
{
    void VKViewportPass::Initialize(VkDevice device, VkPhysicalDevice physical, VkCommandPool commands, VkDescriptorSetLayout layout, bool timestamps)
    {
        VkCommandBufferAllocateInfo allocate{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        allocate.commandPool = commands;
        allocate.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate.commandBufferCount = 1;
        if (vkAllocateCommandBuffers(device, &allocate, &Command) != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot allocate viewport commands");
        }
        Camera.Initialize(device, physical, sizeof(glm::mat4) * 2, 1);
        VkDescriptorPoolSize size{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 };
        VkDescriptorPoolCreateInfo pool{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        pool.maxSets = 1;
        pool.poolSizeCount = 1;
        pool.pPoolSizes = &size;
        if (vkCreateDescriptorPool(device, &pool, nullptr, &Pool) != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot allocate viewport descriptors");
        }
        VkDescriptorSetAllocateInfo sets{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        sets.descriptorPool = Pool;
        sets.descriptorSetCount = 1;
        sets.pSetLayouts = &layout;
        if (vkAllocateDescriptorSets(device, &sets, &Globals) != VK_SUCCESS)
        {
            throw std::runtime_error("Cannot allocate viewport globals");
        }
        VkDescriptorBufferInfo buffer{ Camera.GetBuffer(0), 0, sizeof(glm::mat4) * 2 };
        VkWriteDescriptorSet write{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
        write.dstSet = Globals;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &buffer;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
        if (timestamps)
        {
            VkQueryPoolCreateInfo query{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
            query.queryType = VK_QUERY_TYPE_TIMESTAMP;
            query.queryCount = 2;
            if (vkCreateQueryPool(device, &query, nullptr, &Queries) != VK_SUCCESS)
            {
                throw std::runtime_error("Cannot allocate viewport timestamps");
            }
        }
    }

    void VKViewportPass::CollectTiming(VkDevice device, double period, uint32_t validBits)
    {
        if (Submitted && Queries)
        {
            uint64_t ticks[2]{};
            if (vkGetQueryPoolResults(device, Queries, 0, 2, sizeof(ticks), ticks, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS)
            {
                if (auto target = Target.lock())
                {
                    const uint64_t mask = validBits >= 64 ? ~uint64_t(0) : (uint64_t(1) << validBits) - 1;
                    target->Statistics.GpuMS = ((ticks[1] - ticks[0]) & mask) * period / 1000000.0;
                    target->Statistics.GpuValid = true;
                }
            }
        }
        Submitted = false;
    }

    void VKViewportPass::Destroy(VkDevice device, VkCommandPool commands)
    {
        if (Mapped)
        {
            vkUnmapMemory(device, InstanceMemory);
        }
        if (Instances)
        {
            vkDestroyBuffer(device, Instances, nullptr);
        }
        if (InstanceMemory)
        {
            vkFreeMemory(device, InstanceMemory, nullptr);
        }
        if (Pool)
        {
            vkDestroyDescriptorPool(device, Pool, nullptr);
        }
        Camera.Shutdown();
        if (Queries)
        {
            vkDestroyQueryPool(device, Queries, nullptr);
        }
        if (Command)
        {
            vkFreeCommandBuffers(device, commands, 1, &Command);
        }
    }
} // namespace FINALITY
